#ifndef PLUGIN_H
#define PLUGIN_H
#endif

#include <windows.h>
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <mutex>

#include "Utils.h"
#include <RakHook/rakhook.hpp>

#include <d3d9.h>

#define D3D_VFUNCTIONS 	(119)
#define DEVICE_PTR 		(0xC97C28)
#define RESET_INDEX 	(16)
#define PRESENT_INDEX 	(17)

typedef HRESULT(__stdcall* _Present)(IDirect3DDevice9* pDevice, const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion);
typedef long(__stdcall* _Reset)(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pp);

enum class DXElementType : uint8_t {
	Rectangle = 1,
	Text = 2,
	Button = 3,
	Checkbox = 4,
	Input = 5,
	Image = 6,
	Line = 7,
	Circle = 8,
	Clip = 9,
	GradientRectangle = 10,
	RoundedRectangle = 11,
	Triangle = 12,
	Slider = 13,
	ComboBox = 14,
	ListView = 15,
	TabPanel = 16,
	Shadow = 17,
	CircularProgress = 18,
	Graph = 19,
	InventorySlot = 20,
	TexturedProgressBar = 21,
	RadialMenu = 22,
	ColorPicker = 23,
	ScrollContainer = 24,
	Icon = 25
};

struct DXAnimation {
	bool active = false;
	float startX = 0.0f;
	float startY = 0.0f;
	float targetX = 0.0f;
	float targetY = 0.0f;
	float startW = 0.0f;
	float startH = 0.0f;
	float targetW = 0.0f;
	float targetH = 0.0f;
	float startAlpha = 1.0f;
	float targetAlpha = 1.0f;
	DWORD startTime = 0;
	DWORD duration = 0;
	uint8_t easingType = 0;
};

struct DXElement {
	int id = 0;
	DXElementType type = DXElementType::Rectangle;
	float x = 0.0f;
	float y = 0.0f;
	float width = 0.0f;
	float height = 0.0f;
	DWORD color = 0xFFFFFFFF;
	float scale = 1.0f;
	std::string text;
	bool checked = false;
	std::string placeholder;
	std::string font;

	// New fields for advanced shapes
	DWORD colorTR = 0;
	DWORD colorBL = 0;
	DWORD colorBR = 0;
	float radius = 0.0f;
	float x2 = 0.0f;
	float y2 = 0.0f;
	float x3 = 0.0f;
	float y3 = 0.0f;

	// New fields for UI Widgets
	std::vector<std::string> options;
	int selectedIndex = 0;
	bool isDropped = false;
	int scrollOffset = 0;
	bool draggingScroll = false;

	// Drag & Drop support fields
	bool draggable = false;
	bool isDragging = false;
	float dragOffsetX = 0.0f;
	float dragOffsetY = 0.0f;

	// Premium Upgrade Kit fields
	int parentId = -1;
	DWORD creationTime = 0;
	bool isDestroying = false;
	DWORD destroyRequestedTime = 0;
	float hoverProgress = 0.0f;
	DWORD lastUpdateFrame = 0;
	float shadowSize = 0.0f;
	float shadowOffset = 0.0f;
	std::string tooltipText;
	bool isPassword = false;
	float progress = 0.0f;

	// New Premium features fields
	DXAnimation anim;
	std::string fillTextureUrl;
	std::vector<float> graphValues;
	std::vector<std::string> radialIcons;

	// Custom upgrade fields
	bool blurBehind = false;
	float contentHeight = 0.0f;
};

struct DXFont {
	IDirect3DTexture9* texture = nullptr;
	float charWidths[256] = { 0 };
	float charHeight = 32.0f;
};

struct DXImageTexture {
	IDirect3DTexture9* texture = nullptr;
	int width = 0;
	int height = 0;
};

// Thread-safe collection of DX elements and fonts
extern std::map<int, DXElement> g_dxElements;
extern std::mutex g_dxMutex;
extern std::map<std::string, DXFont> g_dxFonts;
extern std::mutex g_fontMutex;
extern std::map<std::string, DXImageTexture> g_dxImages;
extern std::mutex g_imageMutex;
extern WNDPROC oWndProc;
extern int g_focusedInputId;
extern int g_activeDragElementId;
extern float g_mouseX;
extern float g_mouseY;

void CleanupImageTextures();
unsigned int FNVHash(const std::string& str);

class c_plugin
{
	public:
		c_plugin(HMODULE hmodule);
		~c_plugin();

		static void everything();
		static void attach_console();

		static void game_loop();
		static c_hook<void(*)()> game_loop_hook;
	private:
		HMODULE hmodule;
};
inline c_hook<void(*)()> c_plugin::game_loop_hook = { 0x561B10 };
