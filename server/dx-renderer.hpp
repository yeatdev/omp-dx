#pragma once

#include <sdk.hpp>
#include <string>

void SendDXRectangle(IPlayer& player, int elementId, float x, float y, float w, float h, uint32_t color);
void SendDXText(IPlayer& player, int elementId, const std::string& text, float x, float y, uint32_t color, float scale, const std::string& font = "");
void SendDXDestroy(IPlayer& player, int elementId);
void SendDXClearAll(IPlayer& player);
void SendDXButton(IPlayer& player, int elementId, float x, float y, float w, float h, uint32_t color, float scale, const std::string& text, const std::string& font = "");
void SendDXCheckbox(IPlayer& player, int elementId, float x, float y, float w, float h, uint32_t color, bool checked, float scale, const std::string& label, const std::string& font = "");
void SendDXInput(IPlayer& player, int elementId, float x, float y, float w, float h, uint32_t color, float scale, const std::string& defaultText, const std::string& placeholder, const std::string& font = "");
void SendDXLoadFont(IPlayer& player, const std::string& fontFamily, const std::string& fileName);
void SendDXImage(IPlayer& player, int elementId, float x, float y, float w, float h, uint32_t color, const std::string& url);
void SendDXLine(IPlayer& player, int elementId, float x1, float y1, float x2, float y2, float thickness, uint32_t color);
void SendDXCircle(IPlayer& player, int elementId, float x, float y, float radius, uint32_t color, float thickness);
void SendDXClip(IPlayer& player, int elementId, float x, float y, float w, float h);
void SendDXGradientRectangle(IPlayer& player, int elementId, float x, float y, float w, float h, uint32_t colorTL, uint32_t colorTR, uint32_t colorBL, uint32_t colorBR);
void SendDXRoundedRectangle(IPlayer& player, int elementId, float x, float y, float w, float h, float radius, uint32_t color);
void SendDXTriangle(IPlayer& player, int elementId, float x1, float y1, float x2, float y2, float x3, float y3, uint32_t color);
void SendDXSlider(IPlayer& player, int elementId, float x, float y, float w, float h, uint32_t color, float value, const std::string& font = "");
void SendDXComboBox(IPlayer& player, int elementId, float x, float y, float w, float h, uint32_t color, int selectedIndex, const std::string& options, const std::string& font = "");
void SendDXListView(IPlayer& player, int elementId, float x, float y, float w, float h, uint32_t color, int selectedIndex, const std::string& items, const std::string& font = "");
void SendDXTabPanel(IPlayer& player, int elementId, float x, float y, float w, float h, uint32_t color, int selectedIndex, const std::string& tabs, const std::string& font = "");
void SendDXDraggable(IPlayer& player, int elementId, bool draggable);
void SendDXParent(IPlayer& player, int elementId, int parentId);
void SendDXShadow(IPlayer& player, int elementId, float x, float y, float w, float h, uint32_t color, float size, float offset);
void SendDXTooltip(IPlayer& player, int elementId, const std::string& tooltip);
void SendDXCircularProgress(IPlayer& player, int elementId, float x, float y, float radius, float progress, uint32_t color, float thickness);
void SendDXInputPassword(IPlayer& player, int elementId, bool enable);

void SendDXAnimate(IPlayer& player, int elementId, float targetX, float targetY, float targetW, float targetH, float targetAlpha, int durationMs, int easingType);
void SendDXGraph(IPlayer& player, int elementId, float x, float y, float w, float h, uint32_t color, const std::vector<float>& values, float maxVal);
void SendDXInventorySlot(IPlayer& player, int elementId, float x, float y, float w, float h, uint32_t color, const std::string& iconUrl, const std::string& label, int amount);
void SendDXTexturedProgressBar(IPlayer& player, int elementId, float x, float y, float w, float h, const std::string& bgUrl, const std::string& fillUrl, float progress, uint32_t color);
void SendDXRadialMenu(IPlayer& player, int elementId, float x, float y, float radius, uint32_t color, int selectedIndex, const std::string& items, const std::string& icons);
void SendDXBlurBehind(IPlayer& player, int elementId, bool enable);
void SendDXColorPicker(IPlayer& player, int elementId, float x, float y, float w, float h, uint32_t selectedColor);
void SendDXScrollContainer(IPlayer& player, int elementId, float x, float y, float w, float h, float contentHeight, float scrollVal, uint32_t color);
void SendDXIcon(IPlayer& player, int elementId, float x, float y, float size, const std::string& iconName, uint32_t color, const std::string& font = "");
void SendDXPlaySound(IPlayer& player, const std::string& url);

