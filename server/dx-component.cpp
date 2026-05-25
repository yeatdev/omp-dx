#include <Server/Components/Pawn/pawn_natives.hpp>
#include <Server/Components/Pawn/pawn_impl.hpp>
#include <bitstream.hpp>
#include "dx-component.hpp"

// Required component methods.
StringView DXComponent::componentName() const
{
	return "OMP-DX";
}

SemanticVersion DXComponent::componentVersion() const
{
	return SemanticVersion(1, 0, 0, 0);
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
			if (bs.readUINT16(len)) {
				std::string text;
				if (len > 0) {
					text.resize(len);
					Span<uint8_t> dataSpan(reinterpret_cast<uint8_t*>(&text[0]), len);
					bs.readArray(dataSpan);
				}
				extern void SetPlayerDXInputText(int playerId, int elementId, const std::string& text);
				SetPlayerDXInputText(playerId, elementId, text);
			}
		}
		else if (subtype == 4) { // Input Submit
			uint16_t len = 0;
			if (bs.readUINT16(len)) {
				std::string text;
				if (len > 0) {
					text.resize(len);
					Span<uint8_t> dataSpan(reinterpret_cast<uint8_t*>(&text[0]), len);
					bs.readArray(dataSpan);
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
			if (bs.readFLOAT(value)) {
				extern void SetPlayerDXSliderValue(int playerId, int elementId, float value);
				SetPlayerDXSliderValue(playerId, elementId, value);

				for (IPawnScript* script : pawnComponent->sideScripts()) {
					script->Call("OnPlayerChangeDXSlider", DefaultReturnValue_False, playerId, elementId, value);
				}
				if (auto script = pawnComponent->mainScript()) {
					script->Call("OnPlayerChangeDXSlider", DefaultReturnValue_False, playerId, elementId, value);
				}
			}
		}
		else if (subtype == 6) { // ComboBox selection
			int32_t index = 0;
			if (bs.readINT32(index)) {
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
			if (bs.readINT32(index)) {
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
			if (bs.readINT32(index)) {
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
			if (bs.readFLOAT(newX) && bs.readFLOAT(newY)) {
				for (IPawnScript* script : pawnComponent->sideScripts()) {
					script->Call("OnPlayerDragDX", DefaultReturnValue_False, playerId, elementId, newX, newY);
				}
				if (auto script = pawnComponent->mainScript()) {
					script->Call("OnPlayerDragDX", DefaultReturnValue_False, playerId, elementId, newX, newY);
				}
			}
		}
		else if (subtype == 10) { // Inventory swap
			int32_t targetId = 0;
			if (bs.readINT32(targetId)) {
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
			if (bs.readINT32(index)) {
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

				for (IPawnScript* script : pawnComponent->sideScripts()) {
					script->Call("OnPlayerSelectDXColor", DefaultReturnValue_False, playerId, elementId, static_cast<int>(selectedColor));
				}
				if (auto script = pawnComponent->mainScript()) {
					script->Call("OnPlayerSelectDXColor", DefaultReturnValue_False, playerId, elementId, static_cast<int>(selectedColor));
				}
			}
		}
		else if (subtype == 13) { // Scroll Container Change
			float ratio = 0.0f;
			if (bs.readFLOAT(ratio)) {
				extern void SetPlayerDXScrollVal(int playerId, int elementId, float value);
				SetPlayerDXScrollVal(playerId, elementId, ratio);

				for (IPawnScript* script : pawnComponent->sideScripts()) {
					script->Call("OnPlayerScrollDXContainer", DefaultReturnValue_False, playerId, elementId, ratio);
				}
				if (auto script = pawnComponent->mainScript()) {
					script->Call("OnPlayerScrollDXContainer", DefaultReturnValue_False, playerId, elementId, ratio);
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
	extern void RemovePlayerScreenSize(int playerId);
	RemovePlayerScreenSize(player.getID());
	extern void RemovePlayerDXStates(int playerId);
	RemovePlayerDXStates(player.getID());
}

bool DXComponent::onReceive(IPlayer& peer, NetworkBitStream& bs)
{
	float w = 0.0f;
	float h = 0.0f;
	if (bs.readFLOAT(w) && bs.readFLOAT(h))
	{
		extern void SetPlayerScreenSize(int playerId, float w, float h);
		SetPlayerScreenSize(peer.getID(), w, h);
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
