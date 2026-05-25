/*
 *  This Source Code Form is subject to the terms of the Mozilla Public License,
 *  v. 2.0. If a copy of the MPL was not distributed with this file, You can
 *  obtain one at http://mozilla.org/MPL/2.0/.
 *
 *  The original code is copyright (c) 2022, open.mp team and contributors.
 */

/// Required for most of open.mp.
#include <sdk.hpp>
#include <Server/Components/Pawn/pawn_natives.hpp>
#include "dx-renderer.hpp"
#include <map>
#include <mutex>

using namespace Impl;

struct ScreenSize {
	float width;
	float height;
};

static std::map<int, ScreenSize> g_playerScreenSizes;
static std::mutex g_screenSizeMutex;

void SetPlayerScreenSize(int playerId, float w, float h) {
	std::lock_guard<std::mutex> lock(g_screenSizeMutex);
	g_playerScreenSizes[playerId] = { w, h };
}

void RemovePlayerScreenSize(int playerId) {
	std::lock_guard<std::mutex> lock(g_screenSizeMutex);
	g_playerScreenSizes.erase(playerId);
}

SCRIPT_API(DX_GetScreenSize, bool(IPlayer& player, float& width, float& height))
{
	std::lock_guard<std::mutex> lock(g_screenSizeMutex);
	auto it = g_playerScreenSizes.find(player.getID());
	if (it != g_playerScreenSizes.end())
	{
		width = it->second.width;
		height = it->second.height;
		return true;
	}
	// Fallback to standard 1080p if client hasn't sent resolution yet
	width = 1920.0f;
	height = 1080.0f;
	return false;
}

bool IsPlayerDXReady(int playerId) {
	std::lock_guard<std::mutex> lock(g_screenSizeMutex);
	return g_playerScreenSizes.find(playerId) != g_playerScreenSizes.end();
}

SCRIPT_API(DX_IsReady, bool(IPlayer& player))
{
	return IsPlayerDXReady(player.getID());
}

SCRIPT_API(DX_DrawRectangle, bool(IPlayer& player, int elementId, float x, float y, float w, float h, int color))
{
	SendDXRectangle(player, elementId, x, y, w, h, static_cast<uint32_t>(color));
	return true;
}
SCRIPT_API(DX_Destroy, bool(IPlayer& player, int elementId))
{
	SendDXDestroy(player, elementId);
	return true;
}

SCRIPT_API(DX_ClearAll, bool(IPlayer& player))
{
	SendDXClearAll(player);
	return true;
}

struct DXElementState {
	bool checked = false;
	std::string text;
	float floatValue = 0.0f;
	int intValue = 0;
};

static std::map<int, std::map<int, DXElementState>> g_playerDXElementStates;
static std::mutex g_dxElementStatesMutex;

void SetPlayerDXCheckboxState(int playerId, int elementId, bool checked) {
	std::lock_guard<std::mutex> lock(g_dxElementStatesMutex);
	g_playerDXElementStates[playerId][elementId].checked = checked;
}

void SetPlayerDXInputText(int playerId, int elementId, const std::string& text) {
	std::lock_guard<std::mutex> lock(g_dxElementStatesMutex);
	g_playerDXElementStates[playerId][elementId].text = text;
}

void RemovePlayerDXStates(int playerId) {
	std::lock_guard<std::mutex> lock(g_dxElementStatesMutex);
	g_playerDXElementStates.erase(playerId);
}

void SetPlayerDXSliderValue(int playerId, int elementId, float value) {
	std::lock_guard<std::mutex> lock(g_dxElementStatesMutex);
	g_playerDXElementStates[playerId][elementId].floatValue = value;
}

void SetPlayerDXSelectionIndex(int playerId, int elementId, int index) {
	std::lock_guard<std::mutex> lock(g_dxElementStatesMutex);
	g_playerDXElementStates[playerId][elementId].intValue = index;
}

void SetPlayerDXColor(int playerId, int elementId, uint32_t color) {
	std::lock_guard<std::mutex> lock(g_dxElementStatesMutex);
	g_playerDXElementStates[playerId][elementId].intValue = static_cast<int>(color);
}

void SetPlayerDXScrollVal(int playerId, int elementId, float value) {
	std::lock_guard<std::mutex> lock(g_dxElementStatesMutex);
	g_playerDXElementStates[playerId][elementId].floatValue = value;
}

cell AMX_NATIVE_CALL DX_LoadFont_Native(AMX* amx, const cell* params)
{
	int playerId = static_cast<int>(params[1]);

	cell* addrFamily = nullptr;
	if (amx_GetAddr(amx, params[2], &addrFamily) != AMX_ERR_NONE) return 0;
	int lenFamily = 0;
	if (amx_StrLen(addrFamily, &lenFamily) != AMX_ERR_NONE) return 0;
	std::string fontFamily;
	if (lenFamily > 0) {
		fontFamily.resize(lenFamily);
		amx_GetString(&fontFamily[0], addrFamily, 0, lenFamily + 1);
	}

	cell* addrUrl = nullptr;
	if (amx_GetAddr(amx, params[3], &addrUrl) != AMX_ERR_NONE) return 0;
	int lenUrl = 0;
	if (amx_StrLen(addrUrl, &lenUrl) != AMX_ERR_NONE) return 0;
	std::string url;
	if (lenUrl > 0) {
		url.resize(lenUrl);
		amx_GetString(&url[0], addrUrl, 0, lenUrl + 1);
	}

	IPlayer* player = getAmxLookups()->players->get(playerId);
	if (player) {
		SendDXLoadFont(*player, fontFamily, url);
		return 1;
	}
	return 0;
}

cell AMX_NATIVE_CALL DX_DrawText_Native(AMX* amx, const cell* params)
{
	int playerId = static_cast<int>(params[1]);
	int elementId = static_cast<int>(params[2]);

	cell* addrText = nullptr;
	if (amx_GetAddr(amx, params[3], &addrText) != AMX_ERR_NONE) return 0;
	int lenText = 0;
	if (amx_StrLen(addrText, &lenText) != AMX_ERR_NONE) return 0;
	std::string text;
	if (lenText > 0) {
		text.resize(lenText);
		amx_GetString(&text[0], addrText, 0, lenText + 1);
	}

	float x = amx_ctof(params[4]);
	float y = amx_ctof(params[5]);
	uint32_t color = static_cast<uint32_t>(params[6]);
	float scale = amx_ctof(params[7]);

	std::string font;
	int numArgs = params[0] / sizeof(cell);
	if (numArgs >= 8) {
		cell* addrFont = nullptr;
		if (amx_GetAddr(amx, params[8], &addrFont) == AMX_ERR_NONE) {
			int lenFont = 0;
			if (amx_StrLen(addrFont, &lenFont) == AMX_ERR_NONE && lenFont > 0) {
				font.resize(lenFont);
				amx_GetString(&font[0], addrFont, 0, lenFont + 1);
			}
		}
	}

	IPlayer* player = getAmxLookups()->players->get(playerId);
	if (player) {
		SendDXText(*player, elementId, text, x, y, color, scale, font);
		return 1;
	}
	return 0;
}

cell AMX_NATIVE_CALL DX_DrawButton_Native(AMX* amx, const cell* params)
{
	int playerId = static_cast<int>(params[1]);
	int elementId = static_cast<int>(params[2]);
	float x = amx_ctof(params[3]);
	float y = amx_ctof(params[4]);
	float w = amx_ctof(params[5]);
	float h = amx_ctof(params[6]);
	uint32_t color = static_cast<uint32_t>(params[7]);
	float scale = amx_ctof(params[8]);

	cell* addr = nullptr;
	if (amx_GetAddr(amx, params[9], &addr) != AMX_ERR_NONE) {
		return 0;
	}
	int len = 0;
	if (amx_StrLen(addr, &len) != AMX_ERR_NONE) {
		return 0;
	}
	std::string text;
	if (len > 0) {
		text.resize(len);
		amx_GetString(&text[0], addr, 0, len + 1);
	}

	std::string font;
	int numArgs = params[0] / sizeof(cell);
	if (numArgs >= 10) {
		cell* addrFont = nullptr;
		if (amx_GetAddr(amx, params[10], &addrFont) == AMX_ERR_NONE) {
			int lenFont = 0;
			if (amx_StrLen(addrFont, &lenFont) == AMX_ERR_NONE && lenFont > 0) {
				font.resize(lenFont);
				amx_GetString(&font[0], addrFont, 0, lenFont + 1);
			}
		}
	}

	IPlayer* player = getAmxLookups()->players->get(playerId);
	if (player) {
		SendDXButton(*player, elementId, x, y, w, h, color, scale, text, font);
		return 1;
	}
	return 0;
}

cell AMX_NATIVE_CALL DX_DrawCheckbox_Native(AMX* amx, const cell* params)
{
	int playerId = static_cast<int>(params[1]);
	int elementId = static_cast<int>(params[2]);
	float x = amx_ctof(params[3]);
	float y = amx_ctof(params[4]);
	float w = amx_ctof(params[5]);
	float h = amx_ctof(params[6]);
	uint32_t color = static_cast<uint32_t>(params[7]);
	bool checked = (params[8] != 0);
	float scale = amx_ctof(params[9]);

	SetPlayerDXCheckboxState(playerId, elementId, checked);

	cell* addr = nullptr;
	if (amx_GetAddr(amx, params[10], &addr) != AMX_ERR_NONE) {
		return 0;
	}
	int len = 0;
	if (amx_StrLen(addr, &len) != AMX_ERR_NONE) {
		return 0;
	}
	std::string label;
	if (len > 0) {
		label.resize(len);
		amx_GetString(&label[0], addr, 0, len + 1);
	}

	std::string font;
	int numArgs = params[0] / sizeof(cell);
	if (numArgs >= 11) {
		cell* addrFont = nullptr;
		if (amx_GetAddr(amx, params[11], &addrFont) == AMX_ERR_NONE) {
			int lenFont = 0;
			if (amx_StrLen(addrFont, &lenFont) == AMX_ERR_NONE && lenFont > 0) {
				font.resize(lenFont);
				amx_GetString(&font[0], addrFont, 0, lenFont + 1);
			}
		}
	}

	IPlayer* player = getAmxLookups()->players->get(playerId);
	if (player) {
		SendDXCheckbox(*player, elementId, x, y, w, h, color, checked, scale, label, font);
		return 1;
	}
	return 0;
}

cell AMX_NATIVE_CALL DX_DrawInput_Native(AMX* amx, const cell* params)
{
	int playerId = static_cast<int>(params[1]);
	int elementId = static_cast<int>(params[2]);
	float x = amx_ctof(params[3]);
	float y = amx_ctof(params[4]);
	float w = amx_ctof(params[5]);
	float h = amx_ctof(params[6]);
	uint32_t color = static_cast<uint32_t>(params[7]);
	float scale = amx_ctof(params[8]);

	cell* addrDefault = nullptr;
	if (amx_GetAddr(amx, params[9], &addrDefault) != AMX_ERR_NONE) {
		return 0;
	}
	int lenDefault = 0;
	if (amx_StrLen(addrDefault, &lenDefault) != AMX_ERR_NONE) {
		return 0;
	}
	std::string defaultText;
	if (lenDefault > 0) {
		defaultText.resize(lenDefault);
		amx_GetString(&defaultText[0], addrDefault, 0, lenDefault + 1);
	}

	SetPlayerDXInputText(playerId, elementId, defaultText);

	cell* addrPlaceholder = nullptr;
	if (amx_GetAddr(amx, params[10], &addrPlaceholder) != AMX_ERR_NONE) {
		return 0;
	}
	int lenPlaceholder = 0;
	if (amx_StrLen(addrPlaceholder, &lenPlaceholder) != AMX_ERR_NONE) {
		return 0;
	}
	std::string placeholder;
	if (lenPlaceholder > 0) {
		placeholder.resize(lenPlaceholder);
		amx_GetString(&placeholder[0], addrPlaceholder, 0, lenPlaceholder + 1);
	}

	std::string font;
	int numArgs = params[0] / sizeof(cell);
	if (numArgs >= 11) {
		cell* addrFont = nullptr;
		if (amx_GetAddr(amx, params[11], &addrFont) == AMX_ERR_NONE) {
			int lenFont = 0;
			if (amx_StrLen(addrFont, &lenFont) == AMX_ERR_NONE && lenFont > 0) {
				font.resize(lenFont);
				amx_GetString(&font[0], addrFont, 0, lenFont + 1);
			}
		}
	}

	IPlayer* player = getAmxLookups()->players->get(playerId);
	if (player) {
		SendDXInput(*player, elementId, x, y, w, h, color, scale, defaultText, placeholder, font);
		return 1;
	}
	return 0;
}

SCRIPT_API(DX_GetCheckboxState, bool(IPlayer& player, int elementId, bool& checked))
{
	std::lock_guard<std::mutex> lock(g_dxElementStatesMutex);
	auto itPlayer = g_playerDXElementStates.find(player.getID());
	if (itPlayer != g_playerDXElementStates.end()) {
		auto itElem = itPlayer->second.find(elementId);
		if (itElem != itPlayer->second.end()) {
			checked = itElem->second.checked;
			return true;
		}
	}
	checked = false;
	return false;
}

SCRIPT_API(DX_GetInputText, bool(IPlayer& player, int elementId, OutputOnlyString& text))
{
	std::lock_guard<std::mutex> lock(g_dxElementStatesMutex);
	auto itPlayer = g_playerDXElementStates.find(player.getID());
	if (itPlayer != g_playerDXElementStates.end()) {
		auto itElem = itPlayer->second.find(elementId);
		if (itElem != itPlayer->second.end()) {
			text = StringView(itElem->second.text);
			return true;
		}
	}
	text = StringView("");
	return false;
}

cell AMX_NATIVE_CALL DX_DrawImage_Native(AMX* amx, const cell* params)
{
	int playerId = static_cast<int>(params[1]);
	int elementId = static_cast<int>(params[2]);

	cell* addrUrl = nullptr;
	if (amx_GetAddr(amx, params[3], &addrUrl) != AMX_ERR_NONE) return 0;
	int lenUrl = 0;
	if (amx_StrLen(addrUrl, &lenUrl) != AMX_ERR_NONE) return 0;
	std::string url;
	if (lenUrl > 0) {
		url.resize(lenUrl);
		amx_GetString(&url[0], addrUrl, 0, lenUrl + 1);
	}

	float x = amx_ctof(params[4]);
	float y = amx_ctof(params[5]);
	float w = amx_ctof(params[6]);
	float h = amx_ctof(params[7]);

	uint32_t color = 0xFFFFFFFF;
	int numArgs = params[0] / sizeof(cell);
	if (numArgs >= 8) {
		color = static_cast<uint32_t>(params[8]);
	}

	IPlayer* player = getAmxLookups()->players->get(playerId);
	if (player) {
		SendDXImage(*player, elementId, x, y, w, h, color, url);
		return 1;
	}
	return 0;
}

SCRIPT_API(DX_DrawLine, bool(IPlayer& player, int elementId, float x1, float y1, float x2, float y2, float thickness, int color))
{
	SendDXLine(player, elementId, x1, y1, x2, y2, thickness, static_cast<uint32_t>(color));
	return true;
}

SCRIPT_API(DX_DrawCircle, bool(IPlayer& player, int elementId, float x, float y, float radius, int color, float thickness))
{
	SendDXCircle(player, elementId, x, y, radius, static_cast<uint32_t>(color), thickness);
	return true;
}

SCRIPT_API(DX_SetClipArea, bool(IPlayer& player, int elementId, float x, float y, float w, float h))
{
	SendDXClip(player, elementId, x, y, w, h);
	return true;
}

SCRIPT_API(DX_DrawGradientRectangle, bool(IPlayer& player, int elementId, float x, float y, float w, float h, int colorTL, int colorTR, int colorBL, int colorBR))
{
	SendDXGradientRectangle(player, elementId, x, y, w, h, static_cast<uint32_t>(colorTL), static_cast<uint32_t>(colorTR), static_cast<uint32_t>(colorBL), static_cast<uint32_t>(colorBR));
	return true;
}

SCRIPT_API(DX_DrawRoundedRectangle, bool(IPlayer& player, int elementId, float x, float y, float w, float h, float radius, int color))
{
	SendDXRoundedRectangle(player, elementId, x, y, w, h, radius, static_cast<uint32_t>(color));
	return true;
}

SCRIPT_API(DX_DrawTriangle, bool(IPlayer& player, int elementId, float x1, float y1, float x2, float y2, float x3, float y3, int color))
{
	SendDXTriangle(player, elementId, x1, y1, x2, y2, x3, y3, static_cast<uint32_t>(color));
	return true;
}

SCRIPT_API(DX_GetSliderValue, bool(IPlayer& player, int elementId, float& value))
{
	std::lock_guard<std::mutex> lock(g_dxElementStatesMutex);
	auto itPlayer = g_playerDXElementStates.find(player.getID());
	if (itPlayer != g_playerDXElementStates.end()) {
		auto itElem = itPlayer->second.find(elementId);
		if (itElem != itPlayer->second.end()) {
			value = itElem->second.floatValue;
			return true;
		}
	}
	value = 0.0f;
	return false;
}

SCRIPT_API(DX_GetComboBoxIndex, bool(IPlayer& player, int elementId, int& index))
{
	std::lock_guard<std::mutex> lock(g_dxElementStatesMutex);
	auto itPlayer = g_playerDXElementStates.find(player.getID());
	if (itPlayer != g_playerDXElementStates.end()) {
		auto itElem = itPlayer->second.find(elementId);
		if (itElem != itPlayer->second.end()) {
			index = itElem->second.intValue;
			return true;
		}
	}
	index = 0;
	return false;
}

SCRIPT_API(DX_GetListViewIndex, bool(IPlayer& player, int elementId, int& index))
{
	std::lock_guard<std::mutex> lock(g_dxElementStatesMutex);
	auto itPlayer = g_playerDXElementStates.find(player.getID());
	if (itPlayer != g_playerDXElementStates.end()) {
		auto itElem = itPlayer->second.find(elementId);
		if (itElem != itPlayer->second.end()) {
			index = itElem->second.intValue;
			return true;
		}
	}
	index = 0;
	return false;
}

SCRIPT_API(DX_GetTabActive, bool(IPlayer& player, int elementId, int& index))
{
	std::lock_guard<std::mutex> lock(g_dxElementStatesMutex);
	auto itPlayer = g_playerDXElementStates.find(player.getID());
	if (itPlayer != g_playerDXElementStates.end()) {
		auto itElem = itPlayer->second.find(elementId);
		if (itElem != itPlayer->second.end()) {
			index = itElem->second.intValue;
			return true;
		}
	}
	index = 0;
	return false;
}

SCRIPT_API(DX_SetDraggable, bool(IPlayer& player, int elementId, bool draggable))
{
	SendDXDraggable(player, elementId, draggable);
	return true;
}

cell AMX_NATIVE_CALL DX_DrawSlider_Native(AMX* amx, const cell* params)
{
	int playerId = static_cast<int>(params[1]);
	int elementId = static_cast<int>(params[2]);
	float x = amx_ctof(params[3]);
	float y = amx_ctof(params[4]);
	float w = amx_ctof(params[5]);
	float h = amx_ctof(params[6]);
	uint32_t color = static_cast<uint32_t>(params[7]);
	float value = amx_ctof(params[8]);

	SetPlayerDXSliderValue(playerId, elementId, value);

	std::string font;
	int numArgs = params[0] / sizeof(cell);
	if (numArgs >= 9) {
		cell* addrFont = nullptr;
		if (amx_GetAddr(amx, params[9], &addrFont) == AMX_ERR_NONE) {
			int lenFont = 0;
			if (amx_StrLen(addrFont, &lenFont) == AMX_ERR_NONE && lenFont > 0) {
				font.resize(lenFont);
				amx_GetString(&font[0], addrFont, 0, lenFont + 1);
			}
		}
	}

	IPlayer* player = getAmxLookups()->players->get(playerId);
	if (player) {
		SendDXSlider(*player, elementId, x, y, w, h, color, value, font);
		return 1;
	}
	return 0;
}

cell AMX_NATIVE_CALL DX_DrawComboBox_Native(AMX* amx, const cell* params)
{
	int playerId = static_cast<int>(params[1]);
	int elementId = static_cast<int>(params[2]);
	float x = amx_ctof(params[3]);
	float y = amx_ctof(params[4]);
	float w = amx_ctof(params[5]);
	float h = amx_ctof(params[6]);
	uint32_t color = static_cast<uint32_t>(params[7]);
	int selectedIndex = static_cast<int>(params[8]);

	SetPlayerDXSelectionIndex(playerId, elementId, selectedIndex);

	cell* addrOpts = nullptr;
	if (amx_GetAddr(amx, params[9], &addrOpts) != AMX_ERR_NONE) return 0;
	int lenOpts = 0;
	if (amx_StrLen(addrOpts, &lenOpts) != AMX_ERR_NONE) return 0;
	std::string options;
	if (lenOpts > 0) {
		options.resize(lenOpts);
		amx_GetString(&options[0], addrOpts, 0, lenOpts + 1);
	}

	std::string font;
	int numArgs = params[0] / sizeof(cell);
	if (numArgs >= 10) {
		cell* addrFont = nullptr;
		if (amx_GetAddr(amx, params[10], &addrFont) == AMX_ERR_NONE) {
			int lenFont = 0;
			if (amx_StrLen(addrFont, &lenFont) == AMX_ERR_NONE && lenFont > 0) {
				font.resize(lenFont);
				amx_GetString(&font[0], addrFont, 0, lenFont + 1);
			}
		}
	}

	IPlayer* player = getAmxLookups()->players->get(playerId);
	if (player) {
		SendDXComboBox(*player, elementId, x, y, w, h, color, selectedIndex, options, font);
		return 1;
	}
	return 0;
}

cell AMX_NATIVE_CALL DX_DrawListView_Native(AMX* amx, const cell* params)
{
	int playerId = static_cast<int>(params[1]);
	int elementId = static_cast<int>(params[2]);
	float x = amx_ctof(params[3]);
	float y = amx_ctof(params[4]);
	float w = amx_ctof(params[5]);
	float h = amx_ctof(params[6]);
	uint32_t color = static_cast<uint32_t>(params[7]);
	int selectedIndex = static_cast<int>(params[8]);

	SetPlayerDXSelectionIndex(playerId, elementId, selectedIndex);

	cell* addrItems = nullptr;
	if (amx_GetAddr(amx, params[9], &addrItems) != AMX_ERR_NONE) return 0;
	int lenItems = 0;
	if (amx_StrLen(addrItems, &lenItems) != AMX_ERR_NONE) return 0;
	std::string items;
	if (lenItems > 0) {
		items.resize(lenItems);
		amx_GetString(&items[0], addrItems, 0, lenItems + 1);
	}

	std::string font;
	int numArgs = params[0] / sizeof(cell);
	if (numArgs >= 10) {
		cell* addrFont = nullptr;
		if (amx_GetAddr(amx, params[10], &addrFont) == AMX_ERR_NONE) {
			int lenFont = 0;
			if (amx_StrLen(addrFont, &lenFont) == AMX_ERR_NONE && lenFont > 0) {
				font.resize(lenFont);
				amx_GetString(&font[0], addrFont, 0, lenFont + 1);
			}
		}
	}

	IPlayer* player = getAmxLookups()->players->get(playerId);
	if (player) {
		SendDXListView(*player, elementId, x, y, w, h, color, selectedIndex, items, font);
		return 1;
	}
	return 0;
}

cell AMX_NATIVE_CALL DX_DrawTabPanel_Native(AMX* amx, const cell* params)
{
	int playerId = static_cast<int>(params[1]);
	int elementId = static_cast<int>(params[2]);
	float x = amx_ctof(params[3]);
	float y = amx_ctof(params[4]);
	float w = amx_ctof(params[5]);
	float h = amx_ctof(params[6]);
	uint32_t color = static_cast<uint32_t>(params[7]);
	int selectedIndex = static_cast<int>(params[8]);

	SetPlayerDXSelectionIndex(playerId, elementId, selectedIndex);

	cell* addrTabs = nullptr;
	if (amx_GetAddr(amx, params[9], &addrTabs) != AMX_ERR_NONE) return 0;
	int lenTabs = 0;
	if (amx_StrLen(addrTabs, &lenTabs) != AMX_ERR_NONE) return 0;
	std::string tabs;
	if (lenTabs > 0) {
		tabs.resize(lenTabs);
		amx_GetString(&tabs[0], addrTabs, 0, lenTabs + 1);
	}

	std::string font;
	int numArgs = params[0] / sizeof(cell);
	if (numArgs >= 10) {
		cell* addrFont = nullptr;
		if (amx_GetAddr(amx, params[10], &addrFont) == AMX_ERR_NONE) {
			int lenFont = 0;
			if (amx_StrLen(addrFont, &lenFont) == AMX_ERR_NONE && lenFont > 0) {
				font.resize(lenFont);
				amx_GetString(&font[0], addrFont, 0, lenFont + 1);
			}
		}
	}

	IPlayer* player = getAmxLookups()->players->get(playerId);
	if (player) {
		SendDXTabPanel(*player, elementId, x, y, w, h, color, selectedIndex, tabs, font);
		return 1;
	}
	return 0;
}

SCRIPT_API(DX_SetParent, bool(IPlayer& player, int elementId, int parentId))
{
	SendDXParent(player, elementId, parentId);
	return true;
}

SCRIPT_API(DX_DrawShadow, bool(IPlayer& player, int elementId, float x, float y, float w, float h, uint32_t color, float size, float offset))
{
	SendDXShadow(player, elementId, x, y, w, h, color, size, offset);
	return true;
}

SCRIPT_API(DX_SetTooltip, bool(IPlayer& player, int elementId, const std::string& tooltipText))
{
	SendDXTooltip(player, elementId, tooltipText);
	return true;
}

SCRIPT_API(DX_DrawCircularProgress, bool(IPlayer& player, int elementId, float x, float y, float radius, float progress, uint32_t color, float thickness))
{
	SendDXCircularProgress(player, elementId, x, y, radius, progress, color, thickness);
	return true;
}

SCRIPT_API(DX_SetInputPassword, bool(IPlayer& player, int elementId, bool enable))
{
	SendDXInputPassword(player, elementId, enable);
	return true;
}

SCRIPT_API(DX_Animate, bool(IPlayer& player, int elementId, float targetX, float targetY, float targetW, float targetH, float targetAlpha, int durationMs, int easingType))
{
	SendDXAnimate(player, elementId, targetX, targetY, targetW, targetH, targetAlpha, durationMs, easingType);
	return true;
}

SCRIPT_API(DX_SetBlurBehind, bool(IPlayer& player, int elementId, bool enable))
{
	SendDXBlurBehind(player, elementId, enable);
	return true;
}

SCRIPT_API(DX_DrawColorPicker, bool(IPlayer& player, int elementId, float x, float y, float w, float h, int selectedColor))
{
	SendDXColorPicker(player, elementId, x, y, w, h, static_cast<uint32_t>(selectedColor));
	return true;
}

SCRIPT_API(DX_DrawScrollContainer, bool(IPlayer& player, int elementId, float x, float y, float w, float h, float contentHeight, float scrollVal, int color))
{
	SendDXScrollContainer(player, elementId, x, y, w, h, contentHeight, scrollVal, static_cast<uint32_t>(color));
	return true;
}

cell AMX_NATIVE_CALL DX_DrawIcon_Native(AMX* amx, const cell* params)
{
	int playerId = static_cast<int>(params[1]);
	int elementId = static_cast<int>(params[2]);
	float x = amx_ctof(params[3]);
	float y = amx_ctof(params[4]);
	float size = amx_ctof(params[5]);

	cell* addrName = nullptr;
	if (amx_GetAddr(amx, params[6], &addrName) != AMX_ERR_NONE) return 0;
	int lenName = 0;
	if (amx_StrLen(addrName, &lenName) != AMX_ERR_NONE) return 0;
	std::string iconName;
	if (lenName > 0) {
		iconName.resize(lenName);
		amx_GetString(&iconName[0], addrName, 0, lenName + 1);
	}

	uint32_t color = static_cast<uint32_t>(params[7]);

	std::string font = "FontAwesome";
	int numArgs = params[0] / sizeof(cell);
	if (numArgs >= 8) {
		cell* addrFont = nullptr;
		if (amx_GetAddr(amx, params[8], &addrFont) == AMX_ERR_NONE) {
			int lenFont = 0;
			if (amx_StrLen(addrFont, &lenFont) == AMX_ERR_NONE && lenFont > 0) {
				font.resize(lenFont);
				amx_GetString(&font[0], addrFont, 0, lenFont + 1);
			}
		}
	}

	IPlayer* player = getAmxLookups()->players->get(playerId);
	if (player) {
		SendDXIcon(*player, elementId, x, y, size, iconName, color, font);
		return 1;
	}
	return 0;
}

SCRIPT_API(DX_PlaySound, bool(IPlayer& player, const std::string& url))
{
	SendDXPlaySound(player, url);
	return true;
}

SCRIPT_API(DX_GetColorPickerColor, bool(IPlayer& player, int elementId, int& color))
{
	std::lock_guard<std::mutex> lock(g_dxElementStatesMutex);
	auto itPlayer = g_playerDXElementStates.find(player.getID());
	if (itPlayer != g_playerDXElementStates.end()) {
		auto itElem = itPlayer->second.find(elementId);
		if (itElem != itPlayer->second.end()) {
			color = itElem->second.intValue;
			return true;
		}
	}
	color = 0xFFFFFFFF;
	return false;
}

SCRIPT_API(DX_GetScrollContainerVal, bool(IPlayer& player, int elementId, float& value))
{
	std::lock_guard<std::mutex> lock(g_dxElementStatesMutex);
	auto itPlayer = g_playerDXElementStates.find(player.getID());
	if (itPlayer != g_playerDXElementStates.end()) {
		auto itElem = itPlayer->second.find(elementId);
		if (itElem != itPlayer->second.end()) {
			value = itElem->second.floatValue;
			return true;
		}
	}
	value = 0.0f;
	return false;
}

cell AMX_NATIVE_CALL DX_DrawGraph_Native(AMX* amx, const cell* params)
{
	int playerId = static_cast<int>(params[1]);
	int elementId = static_cast<int>(params[2]);
	float x = amx_ctof(params[3]);
	float y = amx_ctof(params[4]);
	float w = amx_ctof(params[5]);
	float h = amx_ctof(params[6]);
	uint32_t color = static_cast<uint32_t>(params[7]);

	cell* addrValues = nullptr;
	if (amx_GetAddr(amx, params[8], &addrValues) != AMX_ERR_NONE) return 0;
	int numValues = static_cast<int>(params[9]);
	float maxVal = amx_ctof(params[10]);

	std::vector<float> values;
	for (int i = 0; i < numValues; i++) {
		values.push_back(amx_ctof(addrValues[i]));
	}

	IPlayer* player = getAmxLookups()->players->get(playerId);
	if (player) {
		SendDXGraph(*player, elementId, x, y, w, h, color, values, maxVal);
		return 1;
	}
	return 0;
}

cell AMX_NATIVE_CALL DX_DrawInventorySlot_Native(AMX* amx, const cell* params)
{
	int playerId = static_cast<int>(params[1]);
	int elementId = static_cast<int>(params[2]);
	float x = amx_ctof(params[3]);
	float y = amx_ctof(params[4]);
	float w = amx_ctof(params[5]);
	float h = amx_ctof(params[6]);
	uint32_t color = static_cast<uint32_t>(params[7]);

	cell* addrIcon = nullptr;
	if (amx_GetAddr(amx, params[8], &addrIcon) != AMX_ERR_NONE) return 0;
	int lenIcon = 0;
	if (amx_StrLen(addrIcon, &lenIcon) != AMX_ERR_NONE) return 0;
	std::string iconUrl;
	if (lenIcon > 0) {
		iconUrl.resize(lenIcon);
		amx_GetString(&iconUrl[0], addrIcon, 0, lenIcon + 1);
	}

	cell* addrLabel = nullptr;
	if (amx_GetAddr(amx, params[9], &addrLabel) != AMX_ERR_NONE) return 0;
	int lenLabel = 0;
	if (amx_StrLen(addrLabel, &lenLabel) != AMX_ERR_NONE) return 0;
	std::string label;
	if (lenLabel > 0) {
		label.resize(lenLabel);
		amx_GetString(&label[0], addrLabel, 0, lenLabel + 1);
	}

	int amount = static_cast<int>(params[10]);

	IPlayer* player = getAmxLookups()->players->get(playerId);
	if (player) {
		SendDXInventorySlot(*player, elementId, x, y, w, h, color, iconUrl, label, amount);
		return 1;
	}
	return 0;
}

cell AMX_NATIVE_CALL DX_DrawTexturedProgressBar_Native(AMX* amx, const cell* params)
{
	int playerId = static_cast<int>(params[1]);
	int elementId = static_cast<int>(params[2]);
	float x = amx_ctof(params[3]);
	float y = amx_ctof(params[4]);
	float w = amx_ctof(params[5]);
	float h = amx_ctof(params[6]);

	cell* addrBg = nullptr;
	if (amx_GetAddr(amx, params[7], &addrBg) != AMX_ERR_NONE) return 0;
	int lenBg = 0;
	if (amx_StrLen(addrBg, &lenBg) != AMX_ERR_NONE) return 0;
	std::string bgUrl;
	if (lenBg > 0) {
		bgUrl.resize(lenBg);
		amx_GetString(&bgUrl[0], addrBg, 0, lenBg + 1);
	}

	cell* addrFill = nullptr;
	if (amx_GetAddr(amx, params[8], &addrFill) != AMX_ERR_NONE) return 0;
	int lenFill = 0;
	if (amx_StrLen(addrFill, &lenFill) != AMX_ERR_NONE) return 0;
	std::string fillUrl;
	if (lenFill > 0) {
		fillUrl.resize(lenFill);
		amx_GetString(&fillUrl[0], addrFill, 0, lenFill + 1);
	}

	float progress = amx_ctof(params[9]);
	uint32_t color = static_cast<uint32_t>(params[10]);

	IPlayer* player = getAmxLookups()->players->get(playerId);
	if (player) {
		SendDXTexturedProgressBar(*player, elementId, x, y, w, h, bgUrl, fillUrl, progress, color);
		return 1;
	}
	return 0;
}

cell AMX_NATIVE_CALL DX_DrawRadialMenu_Native(AMX* amx, const cell* params)
{
	int playerId = static_cast<int>(params[1]);
	int elementId = static_cast<int>(params[2]);
	float x = amx_ctof(params[3]);
	float y = amx_ctof(params[4]);
	float radius = amx_ctof(params[5]);
	uint32_t color = static_cast<uint32_t>(params[6]);
	int selectedIndex = static_cast<int>(params[7]);

	cell* addrItems = nullptr;
	if (amx_GetAddr(amx, params[8], &addrItems) != AMX_ERR_NONE) return 0;
	int lenItems = 0;
	if (amx_StrLen(addrItems, &lenItems) != AMX_ERR_NONE) return 0;
	std::string items;
	if (lenItems > 0) {
		items.resize(lenItems);
		amx_GetString(&items[0], addrItems, 0, lenItems + 1);
	}

	cell* addrIcons = nullptr;
	if (amx_GetAddr(amx, params[9], &addrIcons) != AMX_ERR_NONE) return 0;
	int lenIcons = 0;
	if (amx_StrLen(addrIcons, &lenIcons) != AMX_ERR_NONE) return 0;
	std::string icons;
	if (lenIcons > 0) {
		icons.resize(lenIcons);
		amx_GetString(&icons[0], addrIcons, 0, lenIcons + 1);
	}

	IPlayer* player = getAmxLookups()->players->get(playerId);
	if (player) {
		SendDXRadialMenu(*player, elementId, x, y, radius, color, selectedIndex, items, icons);
		return 1;
	}
	return 0;
}


