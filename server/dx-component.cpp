#include <Server/Components/Pawn/pawn_natives.hpp>
#include <Server/Components/Pawn/pawn_impl.hpp>
#include <bitstream.hpp>
#include "dx-component.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <mutex>
#include <string>
#include <unordered_map>

void LogServerToFile(const std::string& text);

namespace {
constexpr uint16_t MaxInputLength = 1024;
constexpr double EventRatePerSecond = 120.0;
constexpr double EventBurst = 240.0;
constexpr double ScreenUpdateRatePerSecond = 2.0;
constexpr double ScreenUpdateBurst = 4.0;

struct EventRateState {
	double tokens = EventBurst;
	std::chrono::steady_clock::time_point last = std::chrono::steady_clock::now();
};

std::mutex g_eventRateMutex;
std::unordered_map<int, EventRateState> g_eventRates;
std::unordered_map<uint64_t, EventRateState> g_elementEventRates;
std::unordered_map<int, EventRateState> g_screenUpdateRates;

struct RejectLogState {
	std::chrono::steady_clock::time_point last = std::chrono::steady_clock::now() - std::chrono::seconds(10);
	uint32_t suppressed = 0;
};

std::mutex g_rejectLogMutex;
std::unordered_map<uint64_t, RejectLogState> g_rejectLogs;

void LogRejectedDXEvent(int playerId, int elementId, uint8_t subtype, uint8_t reason)
{
	const auto now = std::chrono::steady_clock::now();
	uint32_t suppressed = 0;
	bool shouldLog = false;
	{
		std::lock_guard<std::mutex> lock(g_rejectLogMutex);
		const uint64_t key = (static_cast<uint64_t>(static_cast<uint32_t>(playerId)) << 32) |
			(static_cast<uint64_t>(reason) << 24) |
			(static_cast<uint64_t>(subtype) << 16);
		auto& state = g_rejectLogs[key];
		if (std::chrono::duration_cast<std::chrono::milliseconds>(now - state.last).count() >= 1000) {
			shouldLog = true;
			suppressed = state.suppressed;
			state.suppressed = 0;
			state.last = now;
		} else {
			++state.suppressed;
		}
	}

	if (!shouldLog) {
		return;
	}

	const char* reasonText = "unknown";
	if (reason == 1) {
		reasonText = "global-rate-limit";
	} else if (reason == 2) {
		reasonText = "unknown-or-disallowed-element";
	} else if (reason == 3) {
		reasonText = "element-event-rate-limit";
	}
	LogServerToFile("Rejected DX event: player=" + std::to_string(playerId) +
		", subtype=" + std::to_string(static_cast<int>(subtype)) +
		", elementId=" + std::to_string(elementId) +
		", reason=" + reasonText +
		", suppressed=" + std::to_string(suppressed));
}

bool ConsumeEventToken(int playerId)
{
	std::lock_guard<std::mutex> lock(g_eventRateMutex);
	auto& state = g_eventRates[playerId];
	const auto now = std::chrono::steady_clock::now();
	const double elapsed = std::chrono::duration<double>(now - state.last).count();
	state.last = now;
	state.tokens = std::min(EventBurst, state.tokens + elapsed * EventRatePerSecond);
	if (state.tokens < 1.0) {
		return false;
	}
	state.tokens -= 1.0;
	return true;
}

bool ConsumeScreenUpdateToken(int playerId)
{
	std::lock_guard<std::mutex> lock(g_eventRateMutex);
	auto& state = g_screenUpdateRates[playerId];
	const auto now = std::chrono::steady_clock::now();
	const double elapsed = std::chrono::duration<double>(now - state.last).count();
	state.last = now;
	state.tokens = std::min(ScreenUpdateBurst, state.tokens + elapsed * ScreenUpdateRatePerSecond);
	if (state.tokens > ScreenUpdateBurst) {
		state.tokens = ScreenUpdateBurst;
	}
	if (state.tokens < 1.0) {
		return false;
	}
	state.tokens -= 1.0;
	return true;
}

uint64_t MakeElementEventRateKey(int playerId, int elementId, uint8_t subtype)
{
	return (static_cast<uint64_t>(static_cast<uint16_t>(playerId)) << 48) |
		(static_cast<uint64_t>(subtype) << 40) |
		static_cast<uint64_t>(static_cast<uint32_t>(elementId));
}

void GetElementEventRatePolicy(uint8_t subtype, double& rate, double& burst, bool& stateful)
{
	stateful = false;
	switch (subtype) {
		case 1: rate = 5.0; burst = 10.0; return;  // button click
		case 2: rate = 8.0; burst = 12.0; return;  // checkbox toggle
		case 4: rate = 2.0; burst = 4.0; return;   // input submit
		case 6:
		case 7:
		case 8:
		case 11: rate = 8.0; burst = 12.0; return; // selections
		case 10: rate = 5.0; burst = 8.0; return;  // inventory swap
		case 5:
		case 9:
		case 13: stateful = true; rate = 30.0; burst = 45.0; return; // slider, drag, scroll
		case 12: stateful = true; rate = 20.0; burst = 30.0; return; // color picker
		default: rate = 20.0; burst = 30.0; return;
	}
}

bool ConsumeElementEventToken(int playerId, int elementId, uint8_t subtype, bool& stateful)
{
	double rate = 0.0;
	double burst = 0.0;
	GetElementEventRatePolicy(subtype, rate, burst, stateful);

	std::lock_guard<std::mutex> lock(g_eventRateMutex);
	auto& state = g_elementEventRates[MakeElementEventRateKey(playerId, elementId, subtype)];
	const auto now = std::chrono::steady_clock::now();
	const double elapsed = std::chrono::duration<double>(now - state.last).count();
	state.last = now;
	state.tokens = std::min(burst, state.tokens + elapsed * rate);
	if (state.tokens > burst) {
		state.tokens = burst;
	}
	if (state.tokens < 1.0) {
		return false;
	}
	state.tokens -= 1.0;
	return true;
}
}

// Required component methods.
StringView DXComponent::componentName() const
{
	return "OMP-DX";
}

SemanticVersion DXComponent::componentVersion() const
{
	return SemanticVersion(0, 0, 1, 0);
}

class RPC192Handler final : public SingleNetworkInEventHandler {
public:
	bool onReceive(IPlayer& peer, NetworkBitStream& bs) override {
		uint8_t subtype = 0;
		int32_t elementId = 0;
		if (!bs.readUINT8(subtype) || !bs.readINT32(elementId)) {
			return false;
		}

		auto pawnComponent = DXComponent::getInstance()->getPawnComponent();
		if (!pawnComponent) return false;

		int playerId = peer.getID();
		extern bool IsPlayerDXElementEventAllowed(int playerId, int elementId, uint8_t eventSubtype);
		if (!ConsumeEventToken(playerId)) {
			LogRejectedDXEvent(playerId, elementId, subtype, 1);
			return false;
		}
		if (!IsPlayerDXElementEventAllowed(playerId, elementId, subtype)) {
			LogRejectedDXEvent(playerId, elementId, subtype, 2);
			return false;
		}
		bool statefulEvent = false;
		const bool callbackAllowed = ConsumeElementEventToken(playerId, elementId, subtype, statefulEvent);
		if (!callbackAllowed && !statefulEvent) {
			LogRejectedDXEvent(playerId, elementId, subtype, 3);
			return false;
		}
		if (!callbackAllowed) {
			LogRejectedDXEvent(playerId, elementId, subtype, 3);
		}

		if (subtype == 1) { // Button Click
			for (IPawnScript* script : pawnComponent->sideScripts()) {
				script->Call("OnPlayerClickDX", DefaultReturnValue_False, playerId, elementId);
			}
			if (auto script = pawnComponent->mainScript()) {
				script->Call("OnPlayerClickDX", DefaultReturnValue_False, playerId, elementId);
			}
		}
		else if (subtype == 2) { // Checkbox Toggle
			bool checked = false;
			if (bs.readBIT(checked)) {
				extern void SetPlayerDXCheckboxState(int playerId, int elementId, bool checked);
				SetPlayerDXCheckboxState(playerId, elementId, checked);

				for (IPawnScript* script : pawnComponent->sideScripts()) {
					script->Call("OnPlayerToggleDXCheckbox", DefaultReturnValue_False, playerId, elementId, checked ? 1 : 0);
				}
				if (auto script = pawnComponent->mainScript()) {
					script->Call("OnPlayerToggleDXCheckbox", DefaultReturnValue_False, playerId, elementId, checked ? 1 : 0);
				}
			}
		}
		else if (subtype == 3) { // Input Text Change
			uint16_t len = 0;
			if (bs.readUINT16(len) && len <= MaxInputLength) {
				std::string text;
				if (len > 0) {
					text.resize(len);
					Span<uint8_t> dataSpan(reinterpret_cast<uint8_t*>(&text[0]), len);
					if (!bs.readArray(dataSpan)) return false;
				}
				extern void SetPlayerDXInputText(int playerId, int elementId, const std::string& text);
				SetPlayerDXInputText(playerId, elementId, text);
			}
		}
		else if (subtype == 4) { // Input Submit
			uint16_t len = 0;
			if (bs.readUINT16(len) && len <= MaxInputLength) {
				std::string text;
				if (len > 0) {
					text.resize(len);
					Span<uint8_t> dataSpan(reinterpret_cast<uint8_t*>(&text[0]), len);
					if (!bs.readArray(dataSpan)) return false;
				}
				extern void SetPlayerDXInputText(int playerId, int elementId, const std::string& text);
				SetPlayerDXInputText(playerId, elementId, text);

				for (IPawnScript* script : pawnComponent->sideScripts()) {
					script->Call("OnPlayerDXInputSubmit", DefaultReturnValue_False, playerId, elementId, StringView(text));
				}
				if (auto script = pawnComponent->mainScript()) {
					script->Call("OnPlayerDXInputSubmit", DefaultReturnValue_False, playerId, elementId, StringView(text));
				}
			}
		}
		else if (subtype == 5) { // Slider value change
			float value = 0.0f;
			if (bs.readFLOAT(value) && std::isfinite(value) && value >= 0.0f && value <= 1.0f) {
				extern void SetPlayerDXSliderValue(int playerId, int elementId, float value);
				SetPlayerDXSliderValue(playerId, elementId, value);

				if (callbackAllowed) {
					for (IPawnScript* script : pawnComponent->sideScripts()) {
						script->Call("OnPlayerChangeDXSlider", DefaultReturnValue_False, playerId, elementId, value);
					}
					if (auto script = pawnComponent->mainScript()) {
						script->Call("OnPlayerChangeDXSlider", DefaultReturnValue_False, playerId, elementId, value);
					}
				}
			}
		}
		else if (subtype == 6) { // ComboBox selection
			int32_t index = 0;
			if (bs.readINT32(index) && index >= -1 && index <= 100000) {
				extern void SetPlayerDXSelectionIndex(int playerId, int elementId, int index);
				SetPlayerDXSelectionIndex(playerId, elementId, index);

				for (IPawnScript* script : pawnComponent->sideScripts()) {
					script->Call("OnPlayerSelectDXComboBox", DefaultReturnValue_False, playerId, elementId, index);
				}
				if (auto script = pawnComponent->mainScript()) {
					script->Call("OnPlayerSelectDXComboBox", DefaultReturnValue_False, playerId, elementId, index);
				}
			}
		}
		else if (subtype == 7) { // ListView selection
			int32_t index = 0;
			if (bs.readINT32(index) && index >= -1 && index <= 100000) {
				extern void SetPlayerDXSelectionIndex(int playerId, int elementId, int index);
				SetPlayerDXSelectionIndex(playerId, elementId, index);

				for (IPawnScript* script : pawnComponent->sideScripts()) {
					script->Call("OnPlayerSelectDXListView", DefaultReturnValue_False, playerId, elementId, index);
				}
				if (auto script = pawnComponent->mainScript()) {
					script->Call("OnPlayerSelectDXListView", DefaultReturnValue_False, playerId, elementId, index);
				}
			}
		}
		else if (subtype == 8) { // Tab selection
			int32_t index = 0;
			if (bs.readINT32(index) && index >= -1 && index <= 100000) {
				extern void SetPlayerDXSelectionIndex(int playerId, int elementId, int index);
				SetPlayerDXSelectionIndex(playerId, elementId, index);

				for (IPawnScript* script : pawnComponent->sideScripts()) {
					script->Call("OnPlayerSelectDXTab", DefaultReturnValue_False, playerId, elementId, index);
				}
				if (auto script = pawnComponent->mainScript()) {
					script->Call("OnPlayerSelectDXTab", DefaultReturnValue_False, playerId, elementId, index);
				}
			}
		}
		else if (subtype == 9) { // Drag & Drop coordinate update
			float newX = 0.0f;
			float newY = 0.0f;
			if (bs.readFLOAT(newX) && bs.readFLOAT(newY) &&
				std::isfinite(newX) && std::isfinite(newY) &&
				std::abs(newX) <= 100000.0f && std::abs(newY) <= 100000.0f) {
				if (callbackAllowed) {
					for (IPawnScript* script : pawnComponent->sideScripts()) {
						script->Call("OnPlayerDragDX", DefaultReturnValue_False, playerId, elementId, newX, newY);
					}
					if (auto script = pawnComponent->mainScript()) {
						script->Call("OnPlayerDragDX", DefaultReturnValue_False, playerId, elementId, newX, newY);
					}
				}
			}
		}
		else if (subtype == 10) { // Inventory swap
			int32_t targetId = 0;
			if (bs.readINT32(targetId) &&
				IsPlayerDXElementEventAllowed(playerId, targetId, subtype)) {
				for (IPawnScript* script : pawnComponent->sideScripts()) {
					script->Call("OnPlayerSwapDXSlots", DefaultReturnValue_False, playerId, elementId, targetId);
				}
				if (auto script = pawnComponent->mainScript()) {
					script->Call("OnPlayerSwapDXSlots", DefaultReturnValue_False, playerId, elementId, targetId);
				}
			}
		}
		else if (subtype == 11) { // Radial Select
			int32_t index = 0;
			if (bs.readINT32(index) && index >= -1 && index <= 100000) {
				extern void SetPlayerDXSelectionIndex(int playerId, int elementId, int index);
				SetPlayerDXSelectionIndex(playerId, elementId, index);

				for (IPawnScript* script : pawnComponent->sideScripts()) {
					script->Call("OnPlayerSelectRadialItem", DefaultReturnValue_False, playerId, elementId, index);
				}
				if (auto script = pawnComponent->mainScript()) {
					script->Call("OnPlayerSelectRadialItem", DefaultReturnValue_False, playerId, elementId, index);
				}
			}
		}
		else if (subtype == 12) { // Color Select
			uint32_t selectedColor = 0;
			if (bs.readUINT32(selectedColor)) {
				extern void SetPlayerDXColor(int playerId, int elementId, uint32_t color);
				SetPlayerDXColor(playerId, elementId, selectedColor);

				if (callbackAllowed) {
					for (IPawnScript* script : pawnComponent->sideScripts()) {
						script->Call("OnPlayerSelectDXColor", DefaultReturnValue_False, playerId, elementId, static_cast<int>(selectedColor));
					}
					if (auto script = pawnComponent->mainScript()) {
						script->Call("OnPlayerSelectDXColor", DefaultReturnValue_False, playerId, elementId, static_cast<int>(selectedColor));
					}
				}
			}
		}
		else if (subtype == 13) { // Scroll Container Change
			float ratio = 0.0f;
			if (bs.readFLOAT(ratio) && std::isfinite(ratio) && ratio >= 0.0f && ratio <= 1.0f) {
				extern void SetPlayerDXScrollVal(int playerId, int elementId, float value);
				SetPlayerDXScrollVal(playerId, elementId, ratio);

				if (callbackAllowed) {
					for (IPawnScript* script : pawnComponent->sideScripts()) {
						script->Call("OnPlayerScrollDXContainer", DefaultReturnValue_False, playerId, elementId, ratio);
					}
					if (auto script = pawnComponent->mainScript()) {
						script->Call("OnPlayerScrollDXContainer", DefaultReturnValue_False, playerId, elementId, ratio);
					}
				}
			}
		}
		return false;
	}
} g_rpc192Handler;

void DXComponent::onLoad(ICore* c)
{
	core_ = c;
	core_->getPlayers().getPlayerConnectDispatcher().addEventHandler(this);
	
	// Listen to RPC 191
	core_->addPerRPCInEventHandler<191>(this);
	// Listen to RPC 192
	core_->addPerRPCInEventHandler<192>(&g_rpc192Handler);
	
	setAmxLookups(core_);
	core_->printLn("OMP-DX component loaded.");
}

void DXComponent::onInit(IComponentList* components)
{
	pawn_ = components->queryComponent<IPawnComponent>();
	if (pawn_)
	{
		setAmxFunctions(pawn_->getAmxFunctions());
		setAmxLookups(components);
		pawn_->getEventDispatcher().addEventHandler(this);
	}
}

void DXComponent::onReady()
{
}

void DXComponent::onFree(IComponent* component)
{
	if (component == pawn_)
	{
		pawn_ = nullptr;
		setAmxFunctions();
		setAmxLookups();
	}
}

void DXComponent::free()
{
	delete this;
}

void DXComponent::reset()
{
}

void DXComponent::onPlayerConnect(IPlayer& player)
{
}

void DXComponent::onPlayerDisconnect(IPlayer& player, PeerDisconnectReason reason)
{
	{
		std::lock_guard<std::mutex> lock(g_eventRateMutex);
		g_eventRates.erase(player.getID());
		g_screenUpdateRates.erase(player.getID());
		const uint64_t playerPrefix = static_cast<uint64_t>(static_cast<uint16_t>(player.getID())) << 48;
		for (auto it = g_elementEventRates.begin(); it != g_elementEventRates.end(); ) {
			if ((it->first & 0xFFFF000000000000ull) == playerPrefix) {
				it = g_elementEventRates.erase(it);
			} else {
				++it;
			}
		}
	}
	{
		std::lock_guard<std::mutex> lock(g_rejectLogMutex);
		const uint64_t playerPrefix = static_cast<uint64_t>(static_cast<uint32_t>(player.getID())) << 32;
		for (auto it = g_rejectLogs.begin(); it != g_rejectLogs.end(); ) {
			if ((it->first & 0xFFFFFFFF00000000ull) == playerPrefix) {
				it = g_rejectLogs.erase(it);
			} else {
				++it;
			}
		}
	}
	extern void RemovePlayerScreenSize(int playerId);
	RemovePlayerScreenSize(player.getID());
	extern void RemovePlayerDXStates(int playerId);
	RemovePlayerDXStates(player.getID());
}

bool DXComponent::onReceive(IPlayer& peer, NetworkBitStream& bs)
{
	float w = 0.0f;
	float h = 0.0f;
	if (bs.readFLOAT(w) && bs.readFLOAT(h) &&
		std::isfinite(w) && std::isfinite(h) &&
		w >= 320.0f && w <= 16384.0f && h >= 200.0f && h <= 16384.0f)
	{
		int playerId = peer.getID();
		if (!ConsumeScreenUpdateToken(playerId)) {
			return false;
		}
		extern bool IsPlayerDXReady(int playerId);
		bool wasReady = IsPlayerDXReady(playerId);

		extern void SetPlayerScreenSize(int playerId, float w, float h);
		SetPlayerScreenSize(playerId, w, h);

		if (!wasReady)
		{
			auto pawnComponent = DXComponent::getInstance()->getPawnComponent();
			if (pawnComponent)
			{
				for (IPawnScript* script : pawnComponent->sideScripts())
				{
					script->Call("OnPlayerDXReady", DefaultReturnValue_False, playerId);
				}
				if (auto script = pawnComponent->mainScript())
				{
					script->Call("OnPlayerDXReady", DefaultReturnValue_False, playerId);
				}
			}
		}
	}
	return false;
}

extern cell AMX_NATIVE_CALL DX_DrawButton_Native(AMX* amx, const cell* params);
extern cell AMX_NATIVE_CALL DX_DrawCheckbox_Native(AMX* amx, const cell* params);
extern cell AMX_NATIVE_CALL DX_DrawInput_Native(AMX* amx, const cell* params);
extern cell AMX_NATIVE_CALL DX_LoadFont_Native(AMX* amx, const cell* params);
extern cell AMX_NATIVE_CALL DX_DrawText_Native(AMX* amx, const cell* params);
extern cell AMX_NATIVE_CALL DX_DrawImage_Native(AMX* amx, const cell* params);
extern cell AMX_NATIVE_CALL DX_DrawSlider_Native(AMX* amx, const cell* params);
extern cell AMX_NATIVE_CALL DX_DrawComboBox_Native(AMX* amx, const cell* params);
extern cell AMX_NATIVE_CALL DX_DrawListView_Native(AMX* amx, const cell* params);
extern cell AMX_NATIVE_CALL DX_DrawTabPanel_Native(AMX* amx, const cell* params);
extern cell AMX_NATIVE_CALL DX_DrawGraph_Native(AMX* amx, const cell* params);
extern cell AMX_NATIVE_CALL DX_DrawInventorySlot_Native(AMX* amx, const cell* params);
extern cell AMX_NATIVE_CALL DX_DrawTexturedProgressBar_Native(AMX* amx, const cell* params);
extern cell AMX_NATIVE_CALL DX_DrawRadialMenu_Native(AMX* amx, const cell* params);
extern cell AMX_NATIVE_CALL DX_DrawIcon_Native(AMX* amx, const cell* params);

void DXComponent::onAmxLoad(IPawnScript& script)
{
	pawn_natives::AmxLoad(script.GetAMX());

	const AMX_NATIVE_INFO natives[] = {
		{"DX_DrawButton", DX_DrawButton_Native},
		{"DX_DrawCheckbox", DX_DrawCheckbox_Native},
		{"DX_DrawInput", DX_DrawInput_Native},
		{"DX_LoadFont", DX_LoadFont_Native},
		{"DX_DrawText", DX_DrawText_Native},
		{"DX_DrawImage", DX_DrawImage_Native},
		{"DX_DrawSlider", DX_DrawSlider_Native},
		{"DX_DrawComboBox", DX_DrawComboBox_Native},
		{"DX_DrawListView", DX_DrawListView_Native},
		{"DX_DrawTabPanel", DX_DrawTabPanel_Native},
		{"DX_DrawGraph", DX_DrawGraph_Native},
		{"DX_DrawInventorySlot", DX_DrawInventorySlot_Native},
		{"DX_DrawTexturedProgressBar", DX_DrawTexturedProgressBar_Native},
		{"DX_DrawRadialMenu", DX_DrawRadialMenu_Native},
		{"DX_DrawIcon", DX_DrawIcon_Native},
		{nullptr, nullptr}
	};
	amx_Register(script.GetAMX(), natives, -1);
}

void DXComponent::onAmxUnload(IPawnScript& script)
{
}

DXComponent* DXComponent::getInstance()
{
	if (instance_ == nullptr)
	{
		instance_ = new DXComponent();
	}
	return instance_;
}

DXComponent::~DXComponent()
{
	if (pawn_)
	{
		pawn_->getEventDispatcher().removeEventHandler(this);
	}
	if (core_)
	{
		core_->removePerRPCInEventHandler<191>(this);
		core_->removePerRPCInEventHandler<192>(&g_rpc192Handler);
		core_->getPlayers().getPlayerConnectDispatcher().removeEventHandler(this);
	}
}
