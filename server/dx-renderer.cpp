#include "dx-renderer.hpp"
#include <bitstream.hpp>
#include <fstream>
#include <chrono>
#include <iomanip>

void LogServerToFile(const std::string& text) {
	std::ofstream logFile("dx_server_log.txt", std::ios::app);
	if (logFile.is_open()) {
		auto now = std::chrono::system_clock::now();
		auto time = std::chrono::system_clock::to_time_t(now);
		struct tm buf;
		localtime_s(&buf, &time);
		logFile << std::put_time(&buf, "[%H:%M:%S] ") << text << std::endl;
	}
}

std::string GetServerHexStr(const uint8_t* pData, int numBytes) {
	std::string hexStr;
	for (int i = 0; i < numBytes; i++) {
		char tmp[8];
		sprintf_s(tmp, "%02X ", pData[i]);
		hexStr += tmp;
	}
	return hexStr;
}

void SendDXRectangle(IPlayer& player, int elementId, float x, float y, float w, float h, uint32_t color) {
	LogServerToFile("SendDXRectangle - Player: " + std::to_string(player.getID()) + 
		", ElementID: " + std::to_string(elementId) + 
		", X: " + std::to_string(x) + ", Y: " + std::to_string(y) + 
		", W: " + std::to_string(w) + ", H: " + std::to_string(h) + 
		", Color: 0x" + std::to_string(color));

	NetworkBitStream bs;
	bs.writeUINT8(1); // Subtype 1: Create/Update Rectangle
	bs.writeINT32(elementId);
	bs.writeFLOAT(x);
	bs.writeFLOAT(y);
	bs.writeFLOAT(w);
	bs.writeFLOAT(h);
	bs.writeUINT32(color);

	int numBytes = bs.GetNumberOfBytesUsed();
	LogServerToFile("SendDXRectangle: Bytes=" + std::to_string(numBytes) + ", Hex: " + GetServerHexStr(reinterpret_cast<uint8_t*>(bs.GetData()), numBytes));

	Span<uint8_t> dataSpan(reinterpret_cast<uint8_t*>(bs.GetData()), bs.GetNumberOfBitsUsed());
	player.sendRPC(190, dataSpan, 2);
}

void SendDXText(IPlayer& player, int elementId, const std::string& text, float x, float y, uint32_t color, float scale, const std::string& font) {
	LogServerToFile("SendDXText - Player: " + std::to_string(player.getID()) + 
		", ElementID: " + std::to_string(elementId) + 
		", Text: '" + text + "', X: " + std::to_string(x) + ", Y: " + std::to_string(y) + 
		", Color: 0x" + std::to_string(color) + ", Scale: " + std::to_string(scale) + ", Font: '" + font + "'");

	NetworkBitStream bs;
	bs.writeUINT8(2); // Subtype 2: Create/Update Text
	bs.writeINT32(elementId);
	bs.writeFLOAT(x);
	bs.writeFLOAT(y);
	bs.writeUINT32(color);
	bs.writeFLOAT(scale);
	bs.writeDynStr16(text);
	bs.writeDynStr16(font);

	int numBytes = bs.GetNumberOfBytesUsed();
	LogServerToFile("SendDXText: Bytes=" + std::to_string(numBytes) + ", Hex: " + GetServerHexStr(reinterpret_cast<uint8_t*>(bs.GetData()), numBytes));

	Span<uint8_t> dataSpan(reinterpret_cast<uint8_t*>(bs.GetData()), bs.GetNumberOfBitsUsed());
	player.sendRPC(190, dataSpan, 2);
}

void SendDXDestroy(IPlayer& player, int elementId) {
	LogServerToFile("SendDXDestroy - Player: " + std::to_string(player.getID()) + 
		", ElementID: " + std::to_string(elementId));

	NetworkBitStream bs;
	bs.writeUINT8(3); // Subtype 3: Destroy
	bs.writeINT32(elementId);

	Span<uint8_t> dataSpan(reinterpret_cast<uint8_t*>(bs.GetData()), bs.GetNumberOfBitsUsed());
	player.sendRPC(190, dataSpan, 2);
}

void SendDXClearAll(IPlayer& player) {
	LogServerToFile("SendDXClearAll - Player: " + std::to_string(player.getID()));

	NetworkBitStream bs;
	bs.writeUINT8(4); // Subtype 4: ClearAll

	Span<uint8_t> dataSpan(reinterpret_cast<uint8_t*>(bs.GetData()), bs.GetNumberOfBitsUsed());
	player.sendRPC(190, dataSpan, 2);
}

void SendDXButton(IPlayer& player, int elementId, float x, float y, float w, float h, uint32_t color, float scale, const std::string& text, const std::string& font) {
	LogServerToFile("SendDXButton - Player: " + std::to_string(player.getID()) + 
		", ElementID: " + std::to_string(elementId) + 
		", X: " + std::to_string(x) + ", Y: " + std::to_string(y) + 
		", W: " + std::to_string(w) + ", H: " + std::to_string(h) + 
		", Color: 0x" + std::to_string(color) + ", Scale: " + std::to_string(scale) +
		", Text: '" + text + "', Font: '" + font + "'");

	NetworkBitStream bs;
	bs.writeUINT8(5); // Subtype 5: Create/Update Button
	bs.writeINT32(elementId);
	bs.writeFLOAT(x);
	bs.writeFLOAT(y);
	bs.writeFLOAT(w);
	bs.writeFLOAT(h);
	bs.writeUINT32(color);
	bs.writeFLOAT(scale);
	bs.writeDynStr16(text);
	bs.writeDynStr16(font);

	Span<uint8_t> dataSpan(reinterpret_cast<uint8_t*>(bs.GetData()), bs.GetNumberOfBitsUsed());
	player.sendRPC(190, dataSpan, 2);
}

void SendDXCheckbox(IPlayer& player, int elementId, float x, float y, float w, float h, uint32_t color, bool checked, float scale, const std::string& label, const std::string& font) {
	LogServerToFile("SendDXCheckbox - Player: " + std::to_string(player.getID()) + 
		", ElementID: " + std::to_string(elementId) + 
		", X: " + std::to_string(x) + ", Y: " + std::to_string(y) + 
		", W: " + std::to_string(w) + ", H: " + std::to_string(h) + 
		", Color: 0x" + std::to_string(color) + ", Checked: " + std::to_string(checked) +
		", Scale: " + std::to_string(scale) + ", Label: '" + label + "', Font: '" + font + "'");

	NetworkBitStream bs;
	bs.writeUINT8(6); // Subtype 6: Create/Update Checkbox
	bs.writeINT32(elementId);
	bs.writeFLOAT(x);
	bs.writeFLOAT(y);
	bs.writeFLOAT(w);
	bs.writeFLOAT(h);
	bs.writeUINT32(color);
	bs.writeBIT(checked);
	bs.writeFLOAT(scale);
	bs.writeDynStr16(label);
	bs.writeDynStr16(font);

	Span<uint8_t> dataSpan(reinterpret_cast<uint8_t*>(bs.GetData()), bs.GetNumberOfBitsUsed());
	player.sendRPC(190, dataSpan, 2);
}

void SendDXInput(IPlayer& player, int elementId, float x, float y, float w, float h, uint32_t color, float scale, const std::string& defaultText, const std::string& placeholder, const std::string& font) {
	LogServerToFile("SendDXInput - Player: " + std::to_string(player.getID()) + 
		", ElementID: " + std::to_string(elementId) + 
		", X: " + std::to_string(x) + ", Y: " + std::to_string(y) + 
		", W: " + std::to_string(w) + ", H: " + std::to_string(h) + 
		", Color: 0x" + std::to_string(color) + ", Scale: " + std::to_string(scale) +
		", Text: '" + defaultText + "', Placeholder: '" + placeholder + "', Font: '" + font + "'");

	NetworkBitStream bs;
	bs.writeUINT8(7); // Subtype 7: Create/Update Input
	bs.writeINT32(elementId);
	bs.writeFLOAT(x);
	bs.writeFLOAT(y);
	bs.writeFLOAT(w);
	bs.writeFLOAT(h);
	bs.writeUINT32(color);
	bs.writeFLOAT(scale);
	bs.writeDynStr16(defaultText);
	bs.writeDynStr16(placeholder);
	bs.writeDynStr16(font);

	Span<uint8_t> dataSpan(reinterpret_cast<uint8_t*>(bs.GetData()), bs.GetNumberOfBitsUsed());
	player.sendRPC(190, dataSpan, 2);
}

void SendDXLoadFont(IPlayer& player, const std::string& fontFamily, const std::string& url) {
	LogServerToFile("SendDXLoadFont - Player: " + std::to_string(player.getID()) + 
		", FontFamily: '" + fontFamily + "', URL: '" + url + "'");

	NetworkBitStream bs;
	bs.writeUINT8(8); // Subtype 8: Load Font
	bs.writeDynStr16(fontFamily);
	bs.writeDynStr16(url);

	Span<uint8_t> dataSpan(reinterpret_cast<uint8_t*>(bs.GetData()), bs.GetNumberOfBitsUsed());
	player.sendRPC(190, dataSpan, 2);
}

void SendDXImage(IPlayer& player, int elementId, float x, float y, float w, float h, uint32_t color, const std::string& url) {
	LogServerToFile("SendDXImage - Player: " + std::to_string(player.getID()) + 
		", ElementID: " + std::to_string(elementId) + 
		", X: " + std::to_string(x) + ", Y: " + std::to_string(y) + 
		", W: " + std::to_string(w) + ", H: " + std::to_string(h) + 
		", Color: 0x" + std::to_string(color) + ", URL: '" + url + "'");

	NetworkBitStream bs;
	bs.writeUINT8(9); // Subtype 9: Create/Update Image
	bs.writeINT32(elementId);
	bs.writeFLOAT(x);
	bs.writeFLOAT(y);
	bs.writeFLOAT(w);
	bs.writeFLOAT(h);
	bs.writeUINT32(color);
	bs.writeDynStr16(url);

	Span<uint8_t> dataSpan(reinterpret_cast<uint8_t*>(bs.GetData()), bs.GetNumberOfBitsUsed());
	player.sendRPC(190, dataSpan, 2);
}

void SendDXLine(IPlayer& player, int elementId, float x1, float y1, float x2, float y2, float thickness, uint32_t color) {
	LogServerToFile("SendDXLine - Player: " + std::to_string(player.getID()) + 
		", ElementID: " + std::to_string(elementId) + 
		", X1: " + std::to_string(x1) + ", Y1: " + std::to_string(y1) + 
		", X2: " + std::to_string(x2) + ", Y2: " + std::to_string(y2) + 
		", Thickness: " + std::to_string(thickness) + ", Color: 0x" + std::to_string(color));

	NetworkBitStream bs;
	bs.writeUINT8(10); // Subtype 10: Create/Update Line
	bs.writeINT32(elementId);
	bs.writeFLOAT(x1);
	bs.writeFLOAT(y1);
	bs.writeFLOAT(x2);
	bs.writeFLOAT(y2);
	bs.writeFLOAT(thickness);
	bs.writeUINT32(color);

	Span<uint8_t> dataSpan(reinterpret_cast<uint8_t*>(bs.GetData()), bs.GetNumberOfBitsUsed());
	player.sendRPC(190, dataSpan, 2);
}

void SendDXCircle(IPlayer& player, int elementId, float x, float y, float radius, uint32_t color, float thickness) {
	LogServerToFile("SendDXCircle - Player: " + std::to_string(player.getID()) + 
		", ElementID: " + std::to_string(elementId) + 
		", X: " + std::to_string(x) + ", Y: " + std::to_string(y) + 
		", Radius: " + std::to_string(radius) + ", Color: 0x" + std::to_string(color) + 
		", Thickness: " + std::to_string(thickness));

	NetworkBitStream bs;
	bs.writeUINT8(11); // Subtype 11: Create/Update Circle
	bs.writeINT32(elementId);
	bs.writeFLOAT(x);
	bs.writeFLOAT(y);
	bs.writeFLOAT(radius);
	bs.writeUINT32(color);
	bs.writeFLOAT(thickness);

	Span<uint8_t> dataSpan(reinterpret_cast<uint8_t*>(bs.GetData()), bs.GetNumberOfBitsUsed());
	player.sendRPC(190, dataSpan, 2);
}

void SendDXClip(IPlayer& player, int elementId, float x, float y, float w, float h) {
	LogServerToFile("SendDXClip - Player: " + std::to_string(player.getID()) + 
		", ElementID: " + std::to_string(elementId) + 
		", X: " + std::to_string(x) + ", Y: " + std::to_string(y) + 
		", W: " + std::to_string(w) + ", H: " + std::to_string(h));

	NetworkBitStream bs;
	bs.writeUINT8(12); // Subtype 12: Create/Update Clip
	bs.writeINT32(elementId);
	bs.writeFLOAT(x);
	bs.writeFLOAT(y);
	bs.writeFLOAT(w);
	bs.writeFLOAT(h);

	Span<uint8_t> dataSpan(reinterpret_cast<uint8_t*>(bs.GetData()), bs.GetNumberOfBitsUsed());
	player.sendRPC(190, dataSpan, 2);
}

void SendDXGradientRectangle(IPlayer& player, int elementId, float x, float y, float w, float h, uint32_t colorTL, uint32_t colorTR, uint32_t colorBL, uint32_t colorBR) {
	LogServerToFile("SendDXGradientRectangle - Player: " + std::to_string(player.getID()) + 
		", ElementID: " + std::to_string(elementId) + 
		", X: " + std::to_string(x) + ", Y: " + std::to_string(y) + 
		", W: " + std::to_string(w) + ", H: " + std::to_string(h));

	NetworkBitStream bs;
	bs.writeUINT8(13); // Subtype 13: Create/Update Gradient Rectangle
	bs.writeINT32(elementId);
	bs.writeFLOAT(x);
	bs.writeFLOAT(y);
	bs.writeFLOAT(w);
	bs.writeFLOAT(h);
	bs.writeUINT32(colorTL);
	bs.writeUINT32(colorTR);
	bs.writeUINT32(colorBL);
	bs.writeUINT32(colorBR);

	Span<uint8_t> dataSpan(reinterpret_cast<uint8_t*>(bs.GetData()), bs.GetNumberOfBitsUsed());
	player.sendRPC(190, dataSpan, 2);
}

void SendDXRoundedRectangle(IPlayer& player, int elementId, float x, float y, float w, float h, float radius, uint32_t color) {
	LogServerToFile("SendDXRoundedRectangle - Player: " + std::to_string(player.getID()) + 
		", ElementID: " + std::to_string(elementId) + 
		", X: " + std::to_string(x) + ", Y: " + std::to_string(y) + 
		", W: " + std::to_string(w) + ", H: " + std::to_string(h) + 
		", Radius: " + std::to_string(radius));

	NetworkBitStream bs;
	bs.writeUINT8(14); // Subtype 14: Create/Update Rounded Rectangle
	bs.writeINT32(elementId);
	bs.writeFLOAT(x);
	bs.writeFLOAT(y);
	bs.writeFLOAT(w);
	bs.writeFLOAT(h);
	bs.writeFLOAT(radius);
	bs.writeUINT32(color);

	Span<uint8_t> dataSpan(reinterpret_cast<uint8_t*>(bs.GetData()), bs.GetNumberOfBitsUsed());
	player.sendRPC(190, dataSpan, 2);
}

void SendDXTriangle(IPlayer& player, int elementId, float x1, float y1, float x2, float y2, float x3, float y3, uint32_t color) {
	LogServerToFile("SendDXTriangle - Player: " + std::to_string(player.getID()) + 
		", ElementID: " + std::to_string(elementId) + 
		", X1: " + std::to_string(x1) + ", Y1: " + std::to_string(y1) + 
		", X2: " + std::to_string(x2) + ", Y2: " + std::to_string(y2) + 
		", X3: " + std::to_string(x3) + ", Y3: " + std::to_string(y3));

	NetworkBitStream bs;
	bs.writeUINT8(15); // Subtype 15: Create/Update Triangle
	bs.writeINT32(elementId);
	bs.writeFLOAT(x1);
	bs.writeFLOAT(y1);
	bs.writeFLOAT(x2);
	bs.writeFLOAT(y2);
	bs.writeFLOAT(x3);
	bs.writeFLOAT(y3);
	bs.writeUINT32(color);

	Span<uint8_t> dataSpan(reinterpret_cast<uint8_t*>(bs.GetData()), bs.GetNumberOfBitsUsed());
	player.sendRPC(190, dataSpan, 2);
}

void SendDXSlider(IPlayer& player, int elementId, float x, float y, float w, float h, uint32_t color, float value, const std::string& font) {
	LogServerToFile("SendDXSlider - Player: " + std::to_string(player.getID()) + 
		", ElementID: " + std::to_string(elementId) + 
		", X: " + std::to_string(x) + ", Y: " + std::to_string(y) + 
		", W: " + std::to_string(w) + ", H: " + std::to_string(h) + 
		", Color: 0x" + std::to_string(color) + ", Value: " + std::to_string(value) +
		", Font: '" + font + "'");

	NetworkBitStream bs;
	bs.writeUINT8(16); // Subtype 16: Create/Update Slider
	bs.writeINT32(elementId);
	bs.writeFLOAT(x);
	bs.writeFLOAT(y);
	bs.writeFLOAT(w);
	bs.writeFLOAT(h);
	bs.writeUINT32(color);
	bs.writeFLOAT(value);
	bs.writeDynStr16(font);

	Span<uint8_t> dataSpan(reinterpret_cast<uint8_t*>(bs.GetData()), bs.GetNumberOfBitsUsed());
	player.sendRPC(190, dataSpan, 2);
}

void SendDXComboBox(IPlayer& player, int elementId, float x, float y, float w, float h, uint32_t color, int selectedIndex, const std::string& options, const std::string& font) {
	LogServerToFile("SendDXComboBox - Player: " + std::to_string(player.getID()) + 
		", ElementID: " + std::to_string(elementId) + 
		", X: " + std::to_string(x) + ", Y: " + std::to_string(y) + 
		", W: " + std::to_string(w) + ", H: " + std::to_string(h) + 
		", Color: 0x" + std::to_string(color) + ", SelectedIndex: " + std::to_string(selectedIndex) +
		", Options: '" + options + "', Font: '" + font + "'");

	NetworkBitStream bs;
	bs.writeUINT8(17); // Subtype 17: Create/Update ComboBox
	bs.writeINT32(elementId);
	bs.writeFLOAT(x);
	bs.writeFLOAT(y);
	bs.writeFLOAT(w);
	bs.writeFLOAT(h);
	bs.writeUINT32(color);
	bs.writeINT32(selectedIndex);
	bs.writeDynStr16(options);
	bs.writeDynStr16(font);

	Span<uint8_t> dataSpan(reinterpret_cast<uint8_t*>(bs.GetData()), bs.GetNumberOfBitsUsed());
	player.sendRPC(190, dataSpan, 2);
}

void SendDXListView(IPlayer& player, int elementId, float x, float y, float w, float h, uint32_t color, int selectedIndex, const std::string& items, const std::string& font) {
	LogServerToFile("SendDXListView - Player: " + std::to_string(player.getID()) + 
		", ElementID: " + std::to_string(elementId) + 
		", X: " + std::to_string(x) + ", Y: " + std::to_string(y) + 
		", W: " + std::to_string(w) + ", H: " + std::to_string(h) + 
		", Color: 0x" + std::to_string(color) + ", SelectedIndex: " + std::to_string(selectedIndex) +
		", Items: '" + items + "', Font: '" + font + "'");

	NetworkBitStream bs;
	bs.writeUINT8(18); // Subtype 18: Create/Update ListView
	bs.writeINT32(elementId);
	bs.writeFLOAT(x);
	bs.writeFLOAT(y);
	bs.writeFLOAT(w);
	bs.writeFLOAT(h);
	bs.writeUINT32(color);
	bs.writeINT32(selectedIndex);
	bs.writeDynStr16(items);
	bs.writeDynStr16(font);

	Span<uint8_t> dataSpan(reinterpret_cast<uint8_t*>(bs.GetData()), bs.GetNumberOfBitsUsed());
	player.sendRPC(190, dataSpan, 2);
}

void SendDXTabPanel(IPlayer& player, int elementId, float x, float y, float w, float h, uint32_t color, int selectedIndex, const std::string& tabs, const std::string& font) {
	LogServerToFile("SendDXTabPanel - Player: " + std::to_string(player.getID()) + 
		", ElementID: " + std::to_string(elementId) + 
		", X: " + std::to_string(x) + ", Y: " + std::to_string(y) + 
		", W: " + std::to_string(w) + ", H: " + std::to_string(h) + 
		", Color: 0x" + std::to_string(color) + ", SelectedIndex: " + std::to_string(selectedIndex) +
		", Tabs: '" + tabs + "', Font: '" + font + "'");

	NetworkBitStream bs;
	bs.writeUINT8(19); // Subtype 19: Create/Update TabPanel
	bs.writeINT32(elementId);
	bs.writeFLOAT(x);
	bs.writeFLOAT(y);
	bs.writeFLOAT(w);
	bs.writeFLOAT(h);
	bs.writeUINT32(color);
	bs.writeINT32(selectedIndex);
	bs.writeDynStr16(tabs);
	bs.writeDynStr16(font);

	Span<uint8_t> dataSpan(reinterpret_cast<uint8_t*>(bs.GetData()), bs.GetNumberOfBitsUsed());
	player.sendRPC(190, dataSpan, 2);
}

void SendDXDraggable(IPlayer& player, int elementId, bool draggable) {
	LogServerToFile("SendDXDraggable - Player: " + std::to_string(player.getID()) + 
		", ElementID: " + std::to_string(elementId) + 
		", Draggable: " + std::to_string(draggable));

	NetworkBitStream bs;
	bs.writeUINT8(20); // Subtype 20: SetDraggable
	bs.writeINT32(elementId);
	bs.writeBIT(draggable);

	Span<uint8_t> dataSpan(reinterpret_cast<uint8_t*>(bs.GetData()), bs.GetNumberOfBitsUsed());
	player.sendRPC(190, dataSpan, 2);
}

void SendDXParent(IPlayer& player, int elementId, int parentId) {
	LogServerToFile("SendDXParent - Player: " + std::to_string(player.getID()) + 
		", ElementID: " + std::to_string(elementId) + 
		", ParentID: " + std::to_string(parentId));

	NetworkBitStream bs;
	bs.writeUINT8(21); // Subtype 21: SetParent
	bs.writeINT32(elementId);
	bs.writeINT32(parentId);

	Span<uint8_t> dataSpan(reinterpret_cast<uint8_t*>(bs.GetData()), bs.GetNumberOfBitsUsed());
	player.sendRPC(190, dataSpan, 2);
}

void SendDXShadow(IPlayer& player, int elementId, float x, float y, float w, float h, uint32_t color, float size, float offset) {
	LogServerToFile("SendDXShadow - Player: " + std::to_string(player.getID()) + 
		", ElementID: " + std::to_string(elementId) + 
		", X: " + std::to_string(x) + ", Y: " + std::to_string(y) + 
		", W: " + std::to_string(w) + ", H: " + std::to_string(h) + 
		", Color: 0x" + std::to_string(color) + ", Size: " + std::to_string(size) + 
		", Offset: " + std::to_string(offset));

	NetworkBitStream bs;
	bs.writeUINT8(22); // Subtype 22: DrawShadow
	bs.writeINT32(elementId);
	bs.writeFLOAT(x);
	bs.writeFLOAT(y);
	bs.writeFLOAT(w);
	bs.writeFLOAT(h);
	bs.writeUINT32(color);
	bs.writeFLOAT(size);
	bs.writeFLOAT(offset);

	Span<uint8_t> dataSpan(reinterpret_cast<uint8_t*>(bs.GetData()), bs.GetNumberOfBitsUsed());
	player.sendRPC(190, dataSpan, 2);
}

void SendDXTooltip(IPlayer& player, int elementId, const std::string& tooltip) {
	LogServerToFile("SendDXTooltip - Player: " + std::to_string(player.getID()) + 
		", ElementID: " + std::to_string(elementId) + 
		", Tooltip: '" + tooltip + "'");

	NetworkBitStream bs;
	bs.writeUINT8(23); // Subtype 23: SetTooltip
	bs.writeINT32(elementId);
	bs.writeDynStr16(tooltip);

	Span<uint8_t> dataSpan(reinterpret_cast<uint8_t*>(bs.GetData()), bs.GetNumberOfBitsUsed());
	player.sendRPC(190, dataSpan, 2);
}

void SendDXCircularProgress(IPlayer& player, int elementId, float x, float y, float radius, float progress, uint32_t color, float thickness) {
	LogServerToFile("SendDXCircularProgress - Player: " + std::to_string(player.getID()) + 
		", ElementID: " + std::to_string(elementId) + 
		", X: " + std::to_string(x) + ", Y: " + std::to_string(y) + 
		", Radius: " + std::to_string(radius) + ", Progress: " + std::to_string(progress) +
		", Color: 0x" + std::to_string(color) + ", Thickness: " + std::to_string(thickness));

	NetworkBitStream bs;
	bs.writeUINT8(24); // Subtype 24: DrawCircularProgress
	bs.writeINT32(elementId);
	bs.writeFLOAT(x);
	bs.writeFLOAT(y);
	bs.writeFLOAT(radius);
	bs.writeFLOAT(progress);
	bs.writeUINT32(color);
	bs.writeFLOAT(thickness);

	Span<uint8_t> dataSpan(reinterpret_cast<uint8_t*>(bs.GetData()), bs.GetNumberOfBitsUsed());
	player.sendRPC(190, dataSpan, 2);
}

void SendDXInputPassword(IPlayer& player, int elementId, bool enable) {
	LogServerToFile("SendDXInputPassword - Player: " + std::to_string(player.getID()) + 
		", ElementID: " + std::to_string(elementId) + 
		", Enable: " + std::to_string(enable));

	NetworkBitStream bs;
	bs.writeUINT8(25); // Subtype 25: SetInputPassword
	bs.writeINT32(elementId);
	bs.writeBIT(enable);

	Span<uint8_t> dataSpan(reinterpret_cast<uint8_t*>(bs.GetData()), bs.GetNumberOfBitsUsed());
	player.sendRPC(190, dataSpan, 2);
}

void SendDXAnimate(IPlayer& player, int elementId, float targetX, float targetY, float targetW, float targetH, float targetAlpha, int durationMs, int easingType) {
	LogServerToFile("SendDXAnimate - Player: " + std::to_string(player.getID()) + 
		", ElementID: " + std::to_string(elementId) + 
		", TargetX: " + std::to_string(targetX) + ", TargetY: " + std::to_string(targetY) +
		", TargetW: " + std::to_string(targetW) + ", TargetH: " + std::to_string(targetH) +
		", TargetAlpha: " + std::to_string(targetAlpha) + ", Duration: " + std::to_string(durationMs) +
		", Easing: " + std::to_string(easingType));

	NetworkBitStream bs;
	bs.writeUINT8(26);
	bs.writeINT32(elementId);
	bs.writeFLOAT(targetX);
	bs.writeFLOAT(targetY);
	bs.writeFLOAT(targetW);
	bs.writeFLOAT(targetH);
	bs.writeFLOAT(targetAlpha);
	bs.writeINT32(durationMs);
	bs.writeUINT8(static_cast<uint8_t>(easingType));

	Span<uint8_t> dataSpan(reinterpret_cast<uint8_t*>(bs.GetData()), bs.GetNumberOfBitsUsed());
	player.sendRPC(190, dataSpan, 2);
}

void SendDXGraph(IPlayer& player, int elementId, float x, float y, float w, float h, uint32_t color, const std::vector<float>& values, float maxVal) {
	LogServerToFile("SendDXGraph - Player: " + std::to_string(player.getID()) + 
		", ElementID: " + std::to_string(elementId) + 
		", X: " + std::to_string(x) + ", Y: " + std::to_string(y) +
		", W: " + std::to_string(w) + ", H: " + std::to_string(h) +
		", NumValues: " + std::to_string(values.size()));

	NetworkBitStream bs;
	bs.writeUINT8(27);
	bs.writeINT32(elementId);
	bs.writeFLOAT(x);
	bs.writeFLOAT(y);
	bs.writeFLOAT(w);
	bs.writeFLOAT(h);
	bs.writeUINT32(color);
	bs.writeINT32(static_cast<int32_t>(values.size()));
	bs.writeFLOAT(maxVal);
	for (float val : values) {
		bs.writeFLOAT(val);
	}

	Span<uint8_t> dataSpan(reinterpret_cast<uint8_t*>(bs.GetData()), bs.GetNumberOfBitsUsed());
	player.sendRPC(190, dataSpan, 2);
}

void SendDXInventorySlot(IPlayer& player, int elementId, float x, float y, float w, float h, uint32_t color, const std::string& iconUrl, const std::string& label, int amount) {
	LogServerToFile("SendDXInventorySlot - Player: " + std::to_string(player.getID()) + 
		", ElementID: " + std::to_string(elementId) + 
		", X: " + std::to_string(x) + ", Y: " + std::to_string(y) +
		", W: " + std::to_string(w) + ", H: " + std::to_string(h) +
		", Label: '" + label + "', Amount: " + std::to_string(amount));

	NetworkBitStream bs;
	bs.writeUINT8(28);
	bs.writeINT32(elementId);
	bs.writeFLOAT(x);
	bs.writeFLOAT(y);
	bs.writeFLOAT(w);
	bs.writeFLOAT(h);
	bs.writeUINT32(color);
	bs.writeINT32(amount);
	bs.writeDynStr16(iconUrl);
	bs.writeDynStr16(label);

	Span<uint8_t> dataSpan(reinterpret_cast<uint8_t*>(bs.GetData()), bs.GetNumberOfBitsUsed());
	player.sendRPC(190, dataSpan, 2);
}

void SendDXTexturedProgressBar(IPlayer& player, int elementId, float x, float y, float w, float h, const std::string& bgUrl, const std::string& fillUrl, float progress, uint32_t color) {
	LogServerToFile("SendDXTexturedProgressBar - Player: " + std::to_string(player.getID()) + 
		", ElementID: " + std::to_string(elementId) + 
		", X: " + std::to_string(x) + ", Y: " + std::to_string(y) +
		", W: " + std::to_string(w) + ", H: " + std::to_string(h) +
		", Progress: " + std::to_string(progress));

	NetworkBitStream bs;
	bs.writeUINT8(29);
	bs.writeINT32(elementId);
	bs.writeFLOAT(x);
	bs.writeFLOAT(y);
	bs.writeFLOAT(w);
	bs.writeFLOAT(h);
	bs.writeFLOAT(progress);
	bs.writeUINT32(color);
	bs.writeDynStr16(bgUrl);
	bs.writeDynStr16(fillUrl);

	Span<uint8_t> dataSpan(reinterpret_cast<uint8_t*>(bs.GetData()), bs.GetNumberOfBitsUsed());
	player.sendRPC(190, dataSpan, 2);
}

void SendDXRadialMenu(IPlayer& player, int elementId, float x, float y, float radius, uint32_t color, int selectedIndex, const std::string& items, const std::string& icons) {
	LogServerToFile("SendDXRadialMenu - Player: " + std::to_string(player.getID()) + 
		", ElementID: " + std::to_string(elementId) + 
		", X: " + std::to_string(x) + ", Y: " + std::to_string(y) +
		", Radius: " + std::to_string(radius) + ", SelectedIndex: " + std::to_string(selectedIndex));

	NetworkBitStream bs;
	bs.writeUINT8(30);
	bs.writeINT32(elementId);
	bs.writeFLOAT(x);
	bs.writeFLOAT(y);
	bs.writeFLOAT(radius);
	bs.writeUINT32(color);
	bs.writeINT32(selectedIndex);
	bs.writeDynStr16(items);
	bs.writeDynStr16(icons);

	Span<uint8_t> dataSpan(reinterpret_cast<uint8_t*>(bs.GetData()), bs.GetNumberOfBitsUsed());
	player.sendRPC(190, dataSpan, 2);
}

void SendDXBlurBehind(IPlayer& player, int elementId, bool enable) {
	LogServerToFile("SendDXBlurBehind - Player: " + std::to_string(player.getID()) + 
		", ElementID: " + std::to_string(elementId) + 
		", Enable: " + std::to_string(enable));

	NetworkBitStream bs;
	bs.writeUINT8(31);
	bs.writeINT32(elementId);
	bs.writeBIT(enable);

	Span<uint8_t> dataSpan(reinterpret_cast<uint8_t*>(bs.GetData()), bs.GetNumberOfBitsUsed());
	player.sendRPC(190, dataSpan, 2);
}

void SendDXColorPicker(IPlayer& player, int elementId, float x, float y, float w, float h, uint32_t selectedColor) {
	LogServerToFile("SendDXColorPicker - Player: " + std::to_string(player.getID()) + 
		", ElementID: " + std::to_string(elementId) + 
		", X: " + std::to_string(x) + ", Y: " + std::to_string(y) +
		", W: " + std::to_string(w) + ", H: " + std::to_string(h));

	NetworkBitStream bs;
	bs.writeUINT8(32);
	bs.writeINT32(elementId);
	bs.writeFLOAT(x);
	bs.writeFLOAT(y);
	bs.writeFLOAT(w);
	bs.writeFLOAT(h);
	bs.writeUINT32(selectedColor);

	Span<uint8_t> dataSpan(reinterpret_cast<uint8_t*>(bs.GetData()), bs.GetNumberOfBitsUsed());
	player.sendRPC(190, dataSpan, 2);
}

void SendDXScrollContainer(IPlayer& player, int elementId, float x, float y, float w, float h, float contentHeight, float scrollVal, uint32_t color) {
	LogServerToFile("SendDXScrollContainer - Player: " + std::to_string(player.getID()) + 
		", ElementID: " + std::to_string(elementId) + 
		", X: " + std::to_string(x) + ", Y: " + std::to_string(y) +
		", W: " + std::to_string(w) + ", H: " + std::to_string(h) +
		", ContentHeight: " + std::to_string(contentHeight) + ", ScrollVal: " + std::to_string(scrollVal));

	NetworkBitStream bs;
	bs.writeUINT8(33);
	bs.writeINT32(elementId);
	bs.writeFLOAT(x);
	bs.writeFLOAT(y);
	bs.writeFLOAT(w);
	bs.writeFLOAT(h);
	bs.writeFLOAT(contentHeight);
	bs.writeFLOAT(scrollVal);
	bs.writeUINT32(color);

	Span<uint8_t> dataSpan(reinterpret_cast<uint8_t*>(bs.GetData()), bs.GetNumberOfBitsUsed());
	player.sendRPC(190, dataSpan, 2);
}

void SendDXIcon(IPlayer& player, int elementId, float x, float y, float size, const std::string& iconName, uint32_t color, const std::string& font) {
	LogServerToFile("SendDXIcon - Player: " + std::to_string(player.getID()) + 
		", ElementID: " + std::to_string(elementId) + 
		", X: " + std::to_string(x) + ", Y: " + std::to_string(y) +
		", Size: " + std::to_string(size) + ", IconName: '" + iconName + "', Font: '" + font + "'");

	NetworkBitStream bs;
	bs.writeUINT8(35);
	bs.writeINT32(elementId);
	bs.writeFLOAT(x);
	bs.writeFLOAT(y);
	bs.writeFLOAT(size);
	bs.writeUINT32(color);
	bs.writeDynStr16(iconName);
	bs.writeDynStr16(font);

	Span<uint8_t> dataSpan(reinterpret_cast<uint8_t*>(bs.GetData()), bs.GetNumberOfBitsUsed());
	player.sendRPC(190, dataSpan, 2);
}

void SendDXPlaySound(IPlayer& player, const std::string& url) {
	LogServerToFile("SendDXPlaySound - Player: " + std::to_string(player.getID()) + 
		", URL: '" + url + "'");

	NetworkBitStream bs;
	bs.writeUINT8(36);
	bs.writeDynStr16(url);

	Span<uint8_t> dataSpan(reinterpret_cast<uint8_t*>(bs.GetData()), bs.GetNumberOfBitsUsed());
	player.sendRPC(190, dataSpan, 2);
}


