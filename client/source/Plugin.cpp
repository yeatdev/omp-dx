#include "Plugin.h"

#include <sampapi/CChat.h>
#include <RakNet/PacketEnumerations.h>
#include <RakNet/StringCompressor.h>
#include <RakNet/BitStream.h>

#include <fstream>
#include <iomanip>
#include <sstream>

namespace samp = sampapi::v03dl;

_Present oPresent = nullptr;
_Reset oReset = nullptr;

bool g_bwasInitialized = false;
DWORD procID;
HANDLE handle;
HWND hWnd;

float g_screenWidth = 0.0f;
float g_screenHeight = 0.0f;
DWORD g_lastScreenSizeSendTime = 0;

std::map<int, DXElement> g_dxElements;
std::mutex g_dxMutex;

WNDPROC oWndProc = nullptr;
int g_focusedInputId = -1;
float g_mouseX = 0.0f;
float g_mouseY = 0.0f;
int g_activeSliderDragId = -1;
int g_activeListViewScrollDragId = -1;
int g_activeDragElementId = -1;
int g_activeScrollContainerDragId = -1;
int g_activeInventoryDragId = -1;
int g_activeColorPickerDragId = -1;


std::map<std::string, DXFont> g_dxFonts;
std::mutex g_fontMutex;
std::vector<std::string> g_loadedFontPaths;

#include <wininet.h>
#pragma comment(lib, "wininet.lib")
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")
ULONG_PTR g_gdiplusToken = 0;
std::map<std::string, DXImageTexture> g_dxImages;
std::mutex g_imageMutex;

IDirect3DTexture9* g_pBlurTexture1 = nullptr;
IDirect3DTexture9* g_pBlurTexture2 = nullptr;
int g_blurTexW = 0;
int g_blurTexH = 0;

void LogToFile(const std::string& text);
void GetElementShift(const DXElement& elem, float& shiftX, float& shiftY);
float GetElementAlphaMultiplier(const DXElement& elem, DWORD now);
DWORD ModulateAlpha(DWORD color, float multiplier);
void CleanupBlurTextures();
DWORD GetHueColor(float rx);

bool DownloadFileWinINet(const std::string& url, const std::string& localPath) {
	LogToFile("DownloadFileWinINet: Starting download from: " + url + " to: " + localPath);
	HINTERNET hSession = InternetOpenA("Mozilla/5.0 (Windows NT 10.0; Win64; x64) omp-dx/1.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if (!hSession) {
		LogToFile("DownloadFileWinINet: InternetOpenA failed. Error: " + std::to_string(GetLastError()));
		return false;
	}

	HINTERNET hUrl = InternetOpenUrlA(hSession, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD | INTERNET_FLAG_SECURE, 0);
	if (!hUrl) {
		LogToFile("DownloadFileWinINet: InternetOpenUrlA failed. Error: " + std::to_string(GetLastError()));
		InternetCloseHandle(hSession);
		return false;
	}

	std::ofstream outFile(localPath, std::ios::binary);
	if (!outFile.is_open()) {
		LogToFile("DownloadFileWinINet: Failed to open local file for writing: " + localPath);
		InternetCloseHandle(hUrl);
		InternetCloseHandle(hSession);
		return false;
	}

	char buffer[8192];
	DWORD bytesRead = 0;
	while (InternetReadFile(hUrl, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
		outFile.write(buffer, bytesRead);
	}

	outFile.close();
	InternetCloseHandle(hUrl);
	InternetCloseHandle(hSession);
	LogToFile("DownloadFileWinINet: Download completed successfully.");
	return true;
}

struct Vertex2D {
	float x, y, z, rhw;
	DWORD color;
};

struct FontVertex {
	float x, y, z, rhw;
	DWORD color;
	float u, v;
};

void LogToFile(const std::string& text) {
	static std::mutex logMutex;
	std::lock_guard<std::mutex> lock(logMutex);
	std::ofstream logFile("dx_client_log.txt", std::ios::app);
	if (logFile.is_open()) {
		SYSTEMTIME lt;
		GetLocalTime(&lt);
		char timeStr[64];
		sprintf_s(timeStr, "[%02d:%02d:%02d.%03d] ", lt.wHour, lt.wMinute, lt.wSecond, lt.wMilliseconds);
		logFile << timeStr << text << std::endl;
	}
}

void ReleaseFontTextures() {
	LogToFile("ReleaseFontTextures called.");
	std::lock_guard<std::mutex> lock(g_fontMutex);
	for (auto& [name, font] : g_dxFonts) {
		if (font.texture) {
			font.texture->Release();
		}
	}
	g_dxFonts.clear();
	LogToFile("All font textures released.");
}

void UnregisterFontResources() {
	LogToFile("UnregisterFontResources called.");
	for (const auto& path : g_loadedFontPaths) {
		RemoveFontResourceExA(path.c_str(), FR_PRIVATE, NULL);
		LogToFile("Removed font resource from Windows: " + path);
	}
	g_loadedFontPaths.clear();
	LogToFile("All Windows font resources unregistered.");
}

void CleanupImageTextures() {
	LogToFile("CleanupImageTextures called.");
	std::lock_guard<std::mutex> lock(g_imageMutex);
	for (auto& [path, imgTex] : g_dxImages) {
		if (imgTex.texture) {
			imgTex.texture->Release();
		}
	}
	g_dxImages.clear();
	LogToFile("All image textures released.");
}

unsigned int FNVHash(const std::string& str) {
	unsigned int hash = 2166136261u;
	for (char c : str) {
		hash ^= (unsigned char)c;
		hash *= 16777619u;
	}
	return hash;
}

IDirect3DTexture9* LoadTextureFromFile(IDirect3DDevice9* pDevice, const std::string& path, int& outWidth, int& outHeight) {
	int len = MultiByteToWideChar(CP_ACP, 0, path.c_str(), -1, NULL, 0);
	if (len <= 0) return nullptr;
	std::wstring wpath(len, 0);
	MultiByteToWideChar(CP_ACP, 0, path.c_str(), -1, &wpath[0], len);
	if (!wpath.empty() && wpath.back() == L'\0') {
		wpath.pop_back();
	}

	Gdiplus::Bitmap* bitmap = Gdiplus::Bitmap::FromFile(wpath.c_str());
	if (!bitmap || bitmap->GetLastStatus() != Gdiplus::Ok) {
		if (bitmap) delete bitmap;
		return nullptr;
	}

	unsigned int width = bitmap->GetWidth();
	unsigned int height = bitmap->GetHeight();
	outWidth = (int)width;
	outHeight = (int)height;

	IDirect3DTexture9* pTexture = nullptr;
	HRESULT hr = pDevice->CreateTexture(width, height, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &pTexture, NULL);
	if (FAILED(hr)) {
		delete bitmap;
		return nullptr;
	}

	D3DLOCKED_RECT rect;
	if (SUCCEEDED(pTexture->LockRect(0, &rect, NULL, 0))) {
		Gdiplus::Rect rectGDIP(0, 0, width, height);
		Gdiplus::BitmapData bmpData;
		if (bitmap->LockBits(&rectGDIP, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bmpData) == Gdiplus::Ok) {
			unsigned char* pDest = (unsigned char*)rect.pBits;
			unsigned char* pSrc = (unsigned char*)bmpData.Scan0;
			for (unsigned int y = 0; y < height; y++) {
				memcpy(pDest + y * rect.Pitch, pSrc + y * bmpData.Stride, width * 4);
			}
			bitmap->UnlockBits(&bmpData);
		}
		pTexture->UnlockRect(0);
	}

	delete bitmap;
	return pTexture;
}

void DownloadImage(std::string url, std::string localPath) {
	LogToFile("DownloadImage: Starting background download from: " + url + " to: " + localPath);
	if (DownloadFileWinINet(url, localPath)) {
		LogToFile("DownloadImage: Download completed successfully for image: " + url);
	} else {
		LogToFile("DownloadImage: FAILED to download image: " + url);
	}
}

const DXImageTexture* GetImageOrCreate(IDirect3DDevice9* pDevice, const std::string& pathOrUrl) {
	if (pathOrUrl.empty()) return nullptr;

	{
		std::lock_guard<std::mutex> lock(g_imageMutex);
		auto it = g_dxImages.find(pathOrUrl);
		if (it != g_dxImages.end()) {
			return &it->second;
		}
	}

	std::string localPath;
	if (pathOrUrl.rfind("http://", 0) == 0 || pathOrUrl.rfind("https://", 0) == 0) {
		unsigned int hash = FNVHash(pathOrUrl);
		localPath = "omp-dx\\images\\" + std::to_string(hash) + ".dat";
	} else {
		localPath = pathOrUrl;
	}

	DWORD fileAttr = GetFileAttributesA(localPath.c_str());
	if (fileAttr == INVALID_FILE_ATTRIBUTES || (fileAttr & FILE_ATTRIBUTE_DIRECTORY)) {
		if (pathOrUrl.rfind("http://", 0) == 0 || pathOrUrl.rfind("https://", 0) == 0) {
			CreateDirectoryA("omp-dx", NULL);
			CreateDirectoryA("omp-dx\\images", NULL);
			static std::map<std::string, bool> downloadingImages;
			static std::mutex downloadingMutex;
			std::lock_guard<std::mutex> dlLock(downloadingMutex);
			if (downloadingImages.find(pathOrUrl) == downloadingImages.end()) {
				downloadingImages[pathOrUrl] = true;
				std::thread dlThread([pathOrUrl, localPath]() {
					DownloadImage(pathOrUrl, localPath);
					std::lock_guard<std::mutex> dlLock2(downloadingMutex);
					downloadingImages.erase(pathOrUrl);
				});
				dlThread.detach();
			}
		}
		return nullptr;
	}

	int w = 0, h = 0;
	IDirect3DTexture9* pTexture = LoadTextureFromFile(pDevice, localPath, w, h);
	if (!pTexture) {
		LogToFile("GetImageOrCreate: FAILED to load texture from path: " + localPath);
		return nullptr;
	}

	DXImageTexture imgTex;
	imgTex.texture = pTexture;
	imgTex.width = w;
	imgTex.height = h;

	LogToFile("GetImageOrCreate: Successfully loaded texture: " + pathOrUrl + " (Size: " + std::to_string(w) + "x" + std::to_string(h) + ")");

	std::lock_guard<std::mutex> lock(g_imageMutex);
	g_dxImages[pathOrUrl] = imgTex;
	return &g_dxImages[pathOrUrl];
}

struct ImageVertex {
	float x, y, z, rhw;
	DWORD color;
	float u, v;
};

void DrawDXImage(IDirect3DDevice9* pDevice, float x, float y, float w, float h, DWORD color, const std::string& pathOrUrl) {
	const DXImageTexture* pImg = GetImageOrCreate(pDevice, pathOrUrl);
	if (!pImg || !pImg->texture) return;

	float drawW = (w > 0.0f) ? w : (float)pImg->width;
	float drawH = (h > 0.0f) ? h : (float)pImg->height;

	ImageVertex vertices[4] = {
		{ x,         y,         0.0f, 1.0f, color, 0.0f, 0.0f },
		{ x + drawW, y,         0.0f, 1.0f, color, 1.0f, 0.0f },
		{ x,         y + drawH, 0.0f, 1.0f, color, 0.0f, 1.0f },
		{ x + drawW, y + drawH, 0.0f, 1.0f, color, 1.0f, 1.0f }
	};

	pDevice->SetTexture(0, pImg->texture);
	pDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	pDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	pDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	pDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	pDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
	pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
	pDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	pDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

	pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

	pDevice->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1);
	pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(ImageVertex));
}

void DrawDXLine(IDirect3DDevice9* pDevice, float x1, float y1, float x2, float y2, float thickness, DWORD color) {
	if (thickness <= 0.0f) thickness = 1.0f;
	float dx = x2 - x1;
	float dy = y2 - y1;
	float len = sqrtf(dx * dx + dy * dy);
	if (len == 0.0f) return;

	float nx = -dy / len;
	float ny = dx / len;
	float ox = nx * (thickness * 0.5f);
	float oy = ny * (thickness * 0.5f);

	Vertex2D vertices[4] = {
		{ x1 - ox, y1 - oy, 0.0f, 1.0f, color },
		{ x1 + ox, y1 + oy, 0.0f, 1.0f, color },
		{ x2 - ox, y2 - oy, 0.0f, 1.0f, color },
		{ x2 + ox, y2 + oy, 0.0f, 1.0f, color }
	};

	pDevice->SetTexture(0, NULL);
	pDevice->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
	pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(Vertex2D));
}

void DrawDXCircle(IDirect3DDevice9* pDevice, float x, float y, float r, DWORD color, float thickness) {
	const int segments = 64;
	const float PI = 3.14159265f;

	pDevice->SetTexture(0, NULL);
	pDevice->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);

	if (thickness <= 0.0f) {
		std::vector<Vertex2D> vertices(segments + 2);
		vertices[0] = { x, y, 0.0f, 1.0f, color };
		for (int i = 0; i <= segments; i++) {
			float theta = (i * 2.0f * PI) / segments;
			vertices[i + 1] = { x + r * cosf(theta), y + r * sinf(theta), 0.0f, 1.0f, color };
		}
		pDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, segments, vertices.data(), sizeof(Vertex2D));
	} else {
		float rInner = r - thickness * 0.5f;
		float rOuter = r + thickness * 0.5f;
		if (rInner < 0.0f) rInner = 0.0f;

		std::vector<Vertex2D> vertices(2 * (segments + 1));
		for (int i = 0; i <= segments; i++) {
			float theta = (i * 2.0f * PI) / segments;
			float cosT = cosf(theta);
			float sinT = sinf(theta);

			vertices[2 * i] = { x + rOuter * cosT, y + rOuter * sinT, 0.0f, 1.0f, color };
			vertices[2 * i + 1] = { x + rInner * cosT, y + rInner * sinT, 0.0f, 1.0f, color };
		}
		pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2 * segments, vertices.data(), sizeof(Vertex2D));
	}
}

void DrawDXCircularProgress(IDirect3DDevice9* pDevice, float x, float y, float r, float progress, DWORD color, float thickness, float multiplier) {
	if (progress <= 0.0f) return;
	if (progress > 1.0f) progress = 1.0f;
	if (thickness <= 0.0f) thickness = 1.0f;

	const int segments = 64;
	const float PI = 3.14159265f;

	float rInner = r - thickness * 0.5f;
	float rOuter = r + thickness * 0.5f;
	if (rInner < 0.0f) rInner = 0.0f;

	pDevice->SetTexture(0, NULL);
	pDevice->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);

	int activeSegments = (int)(progress * segments);
	if (activeSegments < 1) activeSegments = 1;

	std::vector<Vertex2D> vertices(2 * (activeSegments + 1));
	float startAngle = -PI * 0.5f; // Top center
	float totalAngle = progress * 2.0f * PI;

	DWORD modulatedColor = ModulateAlpha(color, multiplier);

	for (int i = 0; i <= activeSegments; i++) {
		float theta = startAngle + (i * totalAngle) / activeSegments;
		float cosT = cosf(theta);
		float sinT = sinf(theta);

		vertices[2 * i] = { x + rOuter * cosT, y + rOuter * sinT, 0.0f, 1.0f, modulatedColor };
		vertices[2 * i + 1] = { x + rInner * cosT, y + rInner * sinT, 0.0f, 1.0f, modulatedColor };
	}
	pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2 * activeSegments, vertices.data(), sizeof(Vertex2D));
}

float GetEasedValue(uint8_t easingType, float t) {
	if (t <= 0.0f) return 0.0f;
	if (t >= 1.0f) return 1.0f;
	switch (easingType) {
		case 1: return t * t; // InQuad
		case 2: return t * (2.0f - t); // OutQuad
		case 3: return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t; // InOutQuad
		case 4: return 1.0f - GetEasedValue(5, 1.0f - t); // InBounce
		case 5: { // OutBounce
			if (t < (1.0f / 2.75f)) {
				return 7.5625f * t * t;
			} else if (t < (2.0f / 2.75f)) {
				t -= (1.5f / 2.75f);
				return 7.5625f * t * t + 0.75f;
			} else if (t < (2.5f / 2.75f)) {
				t -= (2.25f / 2.75f);
				return 7.5625f * t * t + 0.9375f;
			} else {
				t -= (2.625f / 2.75f);
				return 7.5625f * t * t + 0.984375f;
			}
		}
		case 6: return t < 0.5f ? GetEasedValue(4, t * 2.0f) * 0.5f : GetEasedValue(5, t * 2.0f - 1.0f) * 0.5f + 0.5f; // InOutBounce
		case 7: return 1.0f - GetEasedValue(8, 1.0f - t); // InElastic
		case 8: { // OutElastic
			if (t == 0.0f || t == 1.0f) return t;
			float p = 0.3f;
			return powf(2.0f, -10.0f * t) * sinf((t - p / 4.0f) * (2.0f * 3.14159f) / p) + 1.0f;
		}
		case 9: return t < 0.5f ? GetEasedValue(7, t * 2.0f) * 0.5f : GetEasedValue(8, t * 2.0f - 1.0f) * 0.5f + 0.5f; // InOutElastic
		case 10: return 1.0f - cosf(t * 3.14159f * 0.5f); // InSine
		case 11: return sinf(t * 3.14159f * 0.5f); // OutSine
		case 12: return 0.5f * (1.0f - cosf(t * 3.14159f)); // InOutSine
		default: return t; // Linear
	}
}

void DrawDXGraph(IDirect3DDevice9* pDevice, float x, float y, float w, float h, DWORD color, const std::vector<float>& values, float maxVal, float multiplier) {
	if (values.size() < 2) return;

	// Background semi-transparent box
	void DrawDXRectangle(IDirect3DDevice9* pDevice, float x, float y, float w, float h, DWORD color);
	DrawDXRectangle(pDevice, x, y, w, h, ModulateAlpha(0x99000000, multiplier));
	// Border
	DrawDXRectangle(pDevice, x, y, w, 1.0f, ModulateAlpha(0xFF555555, multiplier));
	DrawDXRectangle(pDevice, x, y + h - 1.0f, w, 1.0f, ModulateAlpha(0xFF555555, multiplier));
	DrawDXRectangle(pDevice, x, y, 1.0f, h, ModulateAlpha(0xFF555555, multiplier));
	DrawDXRectangle(pDevice, x + w - 1.0f, y, 1.0f, h, ModulateAlpha(0xFF555555, multiplier));

	float xStep = w / (float)(values.size() - 1);
	std::vector<POINT> points;
	for (size_t i = 0; i < values.size(); i++) {
		float val = (maxVal > 0.0f) ? (values[i] / maxVal) : 0.0f;
		if (val < 0.0f) val = 0.0f;
		if (val > 1.0f) val = 1.0f;
		float vx = x + i * xStep;
		float vy = y + h - val * h;
		points.push_back({ (long)vx, (long)vy });
	}

	// 1. Draw gradient filled area under graph (triangle strip)
	pDevice->SetTexture(0, NULL);
	pDevice->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
	std::vector<Vertex2D> fillVertices(2 * values.size());
	for (size_t i = 0; i < values.size(); i++) {
		float vx = (float)points[i].x;
		float vy = (float)points[i].y;

		// Bottom vertex (transparent)
		fillVertices[2 * i] = { vx, y + h, 0.0f, 1.0f, ModulateAlpha(color & 0x00FFFFFF, multiplier * 0.1f) };
		// Top vertex (solid color)
		fillVertices[2 * i + 1] = { vx, vy, 0.0f, 1.0f, ModulateAlpha(color, multiplier * 0.4f) };
	}
	pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2 * (values.size() - 1), fillVertices.data(), sizeof(Vertex2D));

	// 2. Draw graph line
	void DrawDXLine(IDirect3DDevice9* pDevice, float x1, float y1, float x2, float y2, float thickness, DWORD color);
	for (size_t i = 0; i < values.size() - 1; i++) {
		DrawDXLine(pDevice, (float)points[i].x, (float)points[i].y, (float)points[i + 1].x, (float)points[i + 1].y, 2.0f, ModulateAlpha(color, multiplier));
	}
}

void DrawDXInventorySlot(IDirect3DDevice9* pDevice, float x, float y, float w, float h, DWORD color, const std::string& iconUrl, const std::string& label, int amount, bool isHovered, float multiplier) {
	// Background slot box
	void DrawDXRectangle(IDirect3DDevice9* pDevice, float x, float y, float w, float h, DWORD color);
	DWORD slotBgColor = isHovered ? 0xFF2A2A33 : 0xFF14141E;
	DrawDXRectangle(pDevice, x, y, w, h, ModulateAlpha(slotBgColor, multiplier));
	// Border
	DWORD borderCol = isHovered ? color : 0xFF33333F;
	float borderThickness = isHovered ? 1.5f : 1.0f;
	DrawDXRectangle(pDevice, x, y, w, borderThickness, ModulateAlpha(borderCol, multiplier));
	DrawDXRectangle(pDevice, x, y + h - borderThickness, w, borderThickness, ModulateAlpha(borderCol, multiplier));
	DrawDXRectangle(pDevice, x, y, borderThickness, h, ModulateAlpha(borderCol, multiplier));
	DrawDXRectangle(pDevice, x + w - borderThickness, y, borderThickness, h, ModulateAlpha(borderCol, multiplier));

	// Draw Icon (if present)
	if (!iconUrl.empty()) {
		void DrawDXImage(IDirect3DDevice9* pDevice, float x, float y, float w, float h, DWORD color, const std::string& pathOrUrl);
		DrawDXImage(pDevice, x + 10.0f, y + 10.0f, w - 20.0f, h - 30.0f, ModulateAlpha(0xFFFFFFFF, multiplier), iconUrl);
	}

	// Draw Label
	if (!label.empty()) {
		void DrawDXText(IDirect3DDevice9* pDevice, const std::string& text, float x, float y, DWORD color, float scale, const std::string& fontName);
		DrawDXText(pDevice, label, x + 5.0f, y + h - 18.0f, ModulateAlpha(0xFF8E8E93, multiplier), 0.45f, "Outfit-Bold");
	}

	// Draw Amount Badge (if > 1)
	if (amount > 1) {
		float badgeW = 20.0f;
		float badgeH = 14.0f;
		float bx = x + w - badgeW - 4.0f;
		float by = y + 4.0f;
		DrawDXRectangle(pDevice, bx, by, badgeW, badgeH, ModulateAlpha(0xFF33333F, multiplier));
		
		std::string amtStr = std::to_string(amount);
		void DrawDXText(IDirect3DDevice9* pDevice, const std::string& text, float x, float y, DWORD color, float scale, const std::string& fontName);
		float textScale = 0.4f;
		DrawDXText(pDevice, amtStr, bx + 3.0f, by + 1.0f, ModulateAlpha(0xFFFFFFFF, multiplier), textScale, "Outfit-Bold");
	}
}

void DrawDXTexturedProgressBar(IDirect3DDevice9* pDevice, float x, float y, float w, float h, const std::string& bgUrl, const std::string& fillUrl, float progress, DWORD color, float multiplier) {
	if (progress < 0.0f) progress = 0.0f;
	if (progress > 1.0f) progress = 1.0f;

	void DrawDXImage(IDirect3DDevice9* pDevice, float x, float y, float w, float h, DWORD color, const std::string& pathOrUrl);
	void DrawDXRectangle(IDirect3DDevice9* pDevice, float x, float y, float w, float h, DWORD color);
	
	// 1. Draw Background Texture across whole width
	if (!bgUrl.empty()) {
		DrawDXImage(pDevice, x, y, w, h, ModulateAlpha(0xFFFFFFFF, multiplier), bgUrl);
	} else {
		DrawDXRectangle(pDevice, x, y, w, h, ModulateAlpha(0xFF1E1E24, multiplier));
	}

	// 2. Draw Fill Texture using texture coordinate clipping (Sub-rectangle mapping)
	if (!fillUrl.empty() && progress > 0.0f) {
		extern std::map<std::string, DXImageTexture> g_dxImages;
		extern std::mutex g_imageMutex;
		
		std::lock_guard<std::mutex> lock(g_imageMutex);
		auto it = g_dxImages.find(fillUrl);
		if (it != g_dxImages.end() && it->second.texture) {
			pDevice->SetTexture(0, it->second.texture);
			
			// Set texture stages for alpha modulation
			pDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
			pDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
			pDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
			pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
			pDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
			pDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
			
			pDevice->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1);
			
			// Vertex struct matching FVF
			struct TexVertex {
				float x, y, z, rhw;
				DWORD color;
				float u, v;
			};
			
			float fillW = w * progress;
			DWORD modulatedColor = ModulateAlpha(color, multiplier);
			
			TexVertex vertices[4] = {
				{ x,         y,     0.0f, 1.0f, modulatedColor, 0.0f,     0.0f },
				{ x + fillW, y,     0.0f, 1.0f, modulatedColor, progress, 0.0f },
				{ x,         y + h, 0.0f, 1.0f, modulatedColor, 0.0f,     1.0f },
				{ x + fillW, y + h, 0.0f, 1.0f, modulatedColor, progress, 1.0f }
			};
			
			pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(TexVertex));
			pDevice->SetTexture(0, NULL);
		} else {
			// Fallback to solid color bar if texture is loading
			DrawDXRectangle(pDevice, x, y, w * progress, h, ModulateAlpha(color, multiplier));
			// Proactively trigger async download
			GetImageOrCreate(pDevice, fillUrl);
		}
	}
}

void DrawDXRadialMenu(IDirect3DDevice9* pDevice, float x, float y, float radius, DWORD color, const std::vector<std::string>& items, const std::vector<std::string>& icons, int hoveredIndex, float multiplier) {
	if (items.empty()) return;

	int N = (int)items.size();
	float step = 360.0f / N;
	float rInner = radius * 0.4f;
	float rOuter = radius;
	const float PI = 3.14159265f;

	void DrawDXCircle(IDirect3DDevice9* pDevice, float x, float y, float r, DWORD color, float thickness);
	// Center overlay circle
	DrawDXCircle(pDevice, x, y, rInner, ModulateAlpha(0xFF111116, multiplier), 0.0f);
	DrawDXCircle(pDevice, x, y, rInner, ModulateAlpha(0xFF33333F, multiplier), 1.5f);

	for (int i = 0; i < N; i++) {
		bool isHovered = (i == hoveredIndex);
		float startAngleDeg = i * step - 90.0f; // offset by -90 to start at top center
		float endAngleDeg = (i + 1) * step - 90.0f;

		// Draw sector slice using Triangle Strip
		int segments = 16;
		std::vector<Vertex2D> vertices(2 * (segments + 1));
		
		DWORD sliceCol = isHovered ? color : 0xAA141724;
		DWORD modulatedCol = ModulateAlpha(sliceCol, multiplier);

		for (int j = 0; j <= segments; j++) {
			float theta = (startAngleDeg + (j / (float)segments) * step) * PI / 180.0f;
			float cosT = cosf(theta);
			float sinT = sinf(theta);

			float curROuter = isHovered ? (rOuter + 5.0f) : rOuter;
			float curRInner = rInner;

			vertices[2 * j] = { x + curROuter * cosT, y + curROuter * sinT, 0.0f, 1.0f, modulatedCol };
			vertices[2 * j + 1] = { x + curRInner * cosT, y + curRInner * sinT, 0.0f, 1.0f, modulatedCol };
		}

		pDevice->SetTexture(0, NULL);
		pDevice->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
		pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2 * segments, vertices.data(), sizeof(Vertex2D));

		// Draw sector separator lines
		float thetaStart = startAngleDeg * PI / 180.0f;
		float curROuter = isHovered ? (rOuter + 5.0f) : rOuter;
		void DrawDXLine(IDirect3DDevice9* pDevice, float x1, float y1, float x2, float y2, float thickness, DWORD color);
		DrawDXLine(pDevice, x + rInner * cosf(thetaStart), y + rInner * sinf(thetaStart), x + curROuter * cosf(thetaStart), y + curROuter * sinf(thetaStart), 1.5f, ModulateAlpha(0xFF33333F, multiplier));

		// Draw Item Text & Icon
		float midAngleDeg = startAngleDeg + step * 0.5f;
		float thetaMid = midAngleDeg * PI / 180.0f;
		float rCenter = (rInner + curROuter) * 0.55f;
		float tx = x + rCenter * cosf(thetaMid);
		float ty = y + rCenter * sinf(thetaMid);

		float textScale = isHovered ? 0.55f : 0.5f;
		DWORD textCol = isHovered ? 0xFFFFFFFF : 0xFF8E8E93;
		
		void DrawDXText(IDirect3DDevice9* pDevice, const std::string& text, float x, float y, DWORD color, float scale, const std::string& fontName);
		
		// Draw Icon if present
		if (i < (int)icons.size() && !icons[i].empty()) {
			void DrawDXImage(IDirect3DDevice9* pDevice, float x, float y, float w, float h, DWORD color, const std::string& pathOrUrl);
			DrawDXImage(pDevice, tx - 12.0f, ty - 18.0f, 24.0f, 24.0f, ModulateAlpha(0xFFFFFFFF, multiplier), icons[i]);
			DrawDXText(pDevice, items[i], tx - 20.0f, ty + 10.0f, ModulateAlpha(textCol, multiplier), textScale, "Outfit-Bold");
		} else {
			DrawDXText(pDevice, items[i], tx - 20.0f, ty - 5.0f, ModulateAlpha(textCol, multiplier), textScale, "Outfit-Bold");
		}
	}
}

void DrawDXRectangle(IDirect3DDevice9* pDevice, float x, float y, float w, float h, DWORD color) {
	Vertex2D vertices[4] = {
		{ x,     y,     0.0f, 1.0f, color },
		{ x + w, y,     0.0f, 1.0f, color },
		{ x,     y + h, 0.0f, 1.0f, color },
		{ x + w, y + h, 0.0f, 1.0f, color }
	};
	pDevice->SetTexture(0, NULL);
	pDevice->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
	pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(Vertex2D));
}

void DrawDXGradientRectangle(IDirect3DDevice9* pDevice, float x, float y, float w, float h, DWORD colorTL, DWORD colorTR, DWORD colorBL, DWORD colorBR) {
	Vertex2D vertices[4] = {
		{ x,     y,     0.0f, 1.0f, colorTL },
		{ x + w, y,     0.0f, 1.0f, colorTR },
		{ x,     y + h, 0.0f, 1.0f, colorBL },
		{ x + w, y + h, 0.0f, 1.0f, colorBR }
	};
	pDevice->SetTexture(0, NULL);
	pDevice->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
	pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(Vertex2D));
}

void DrawDXRoundedRectangle(IDirect3DDevice9* pDevice, float x, float y, float w, float h, float r, DWORD color) {
	if (r <= 0.0f) {
		DrawDXRectangle(pDevice, x, y, w, h, color);
		return;
	}

	if (r > w * 0.5f) r = w * 0.5f;
	if (r > h * 0.5f) r = h * 0.5f;

	const int S = 8;
	const float PI = 3.14159265f;
	std::vector<Vertex2D> vertices;
	vertices.push_back({ x + w * 0.5f, y + h * 0.5f, 0.0f, 1.0f, color });

	for (int i = 0; i <= S; i++) {
		float theta = (i * PI * 0.5f) / S;
		float vx = (x + w - r) + r * cosf(theta);
		float vy = (y + r) - r * sinf(theta);
		vertices.push_back({ vx, vy, 0.0f, 1.0f, color });
	}
	for (int i = 0; i <= S; i++) {
		float theta = PI * 0.5f + (i * PI * 0.5f) / S;
		float vx = (x + r) + r * cosf(theta);
		float vy = (y + r) - r * sinf(theta);
		vertices.push_back({ vx, vy, 0.0f, 1.0f, color });
	}
	for (int i = 0; i <= S; i++) {
		float theta = PI + (i * PI * 0.5f) / S;
		float vx = (x + r) + r * cosf(theta);
		float vy = (y + h - r) - r * sinf(theta);
		vertices.push_back({ vx, vy, 0.0f, 1.0f, color });
	}
	for (int i = 0; i <= S; i++) {
		float theta = PI * 1.5f + (i * PI * 0.5f) / S;
		float vx = (x + w - r) + r * cosf(theta);
		float vy = (y + h - r) - r * sinf(theta);
		vertices.push_back({ vx, vy, 0.0f, 1.0f, color });
	}

	vertices.push_back(vertices[1]);

	pDevice->SetTexture(0, NULL);
	pDevice->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
	pDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, vertices.size() - 2, vertices.data(), sizeof(Vertex2D));
}
void DrawDXTriangle(IDirect3DDevice9* pDevice, float x1, float y1, float x2, float y2, float x3, float y3, DWORD color) {
	Vertex2D vertices[3] = {
		{ x1, y1, 0.0f, 1.0f, color },
		{ x2, y2, 0.0f, 1.0f, color },
		{ x3, y3, 0.0f, 1.0f, color }
	};
	pDevice->SetTexture(0, NULL);
	pDevice->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
	pDevice->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 1, vertices, sizeof(Vertex2D));
}

std::vector<std::string> SplitString(const std::string& str, char delimiter) {
	std::vector<std::string> result;
	std::stringstream ss(str);
	std::string item;
	while (std::getline(ss, item, delimiter)) {
		result.push_back(item);
	}
	return result;
}

// Forward declarations for functions used by widget rendering helpers
void DrawDXBorder(IDirect3DDevice9* pDevice, float x, float y, float w, float h, float thickness, DWORD color);
const DXFont* GetFontOrCreate(IDirect3DDevice9* pDevice, const std::string& fontName);
void DrawDXText(IDirect3DDevice9* pDevice, const std::string& text, float x, float y, DWORD color, float scale, const std::string& fontName);

void DrawDXSlider(IDirect3DDevice9* pDevice, float x, float y, float w, float h, DWORD color, float value, float multiplier) {
	float trackH = 4.0f;
	float trackY = y + (h - trackH) * 0.5f;
	DrawDXRectangle(pDevice, x, trackY, w, trackH, ModulateAlpha(0xFF33333D, multiplier));
	
	float progressW = value * w;
	DrawDXRectangle(pDevice, x, trackY, progressW, trackH, color);

	float knobRadius = h * 0.4f;
	if (knobRadius < 6.0f) knobRadius = 6.0f;
	float knobX = x + progressW;
	float knobY = y + h * 0.5f;
	DrawDXCircle(pDevice, knobX, knobY, knobRadius, color, 0.0f);
	DrawDXCircle(pDevice, knobX, knobY, knobRadius, ModulateAlpha(0xFFFFFFFF, multiplier), 1.5f);
}

void DrawDXComboBox(IDirect3DDevice9* pDevice, float x, float y, float w, float h, DWORD color, int selectedIndex, const std::vector<std::string>& options, bool isDropped, const std::string& fontName, float scale, float multiplier) {
	scale = 0.55f; // Lock widget text scale to 0.55f to prevent giant overflow
	DrawDXRectangle(pDevice, x, y, w, h, ModulateAlpha(0xFF1E1E24, multiplier));
	DrawDXBorder(pDevice, x, y, w, h, 1.5f, color);

	std::string selectedText = "Select...";
	if (selectedIndex >= 0 && selectedIndex < (int)options.size()) {
		selectedText = options[selectedIndex];
	}
	float charHeight = 32.0f;
	const DXFont* pFont = GetFontOrCreate(pDevice, fontName);
	if (pFont) charHeight = pFont->charHeight;
	float textY = y + (h - charHeight * scale) * 0.5f;
	DrawDXText(pDevice, selectedText, x + 10.0f, textY, ModulateAlpha(0xFFFFFFFF, multiplier), scale, fontName);

	float arrowSize = h * 0.2f;
	if (arrowSize < 5.0f) arrowSize = 5.0f;
	float ax = x + w - 20.0f;
	float ay = y + h * 0.5f;
	if (isDropped) {
		DrawDXTriangle(pDevice, ax, ay - arrowSize * 0.5f, ax - arrowSize, ay + arrowSize * 0.5f, ax + arrowSize, ay + arrowSize * 0.5f, ModulateAlpha(0xFFFFFFFF, multiplier));
	} else {
		DrawDXTriangle(pDevice, ax, ay + arrowSize * 0.5f, ax - arrowSize, ay - arrowSize * 0.5f, ax + arrowSize, ay - arrowSize * 0.5f, ModulateAlpha(0xFFFFFFFF, multiplier));
	}
}

void DrawDXComboBoxDropdownList(IDirect3DDevice9* pDevice, const DXElement& elem) {
	float x = elem.x;
	float y = elem.y + elem.height;
	float w = elem.width;
	float h = elem.height;
	float scale = 0.55f; // Lock widget text scale to 0.55f to prevent giant overflow

	const DXFont* pFont = GetFontOrCreate(pDevice, elem.font);
	float charHeight = 32.0f;
	if (pFont) charHeight = pFont->charHeight;

	float multiplier = GetElementAlphaMultiplier(elem, GetTickCount());

	int numOptions = (int)elem.options.size();
	for (int i = 0; i < numOptions; i++) {
		float rowY = y + i * h;
		DWORD rowColor = 0xFF14141A;
		bool hovered = (g_mouseX >= x && g_mouseX <= x + w && g_mouseY >= rowY && g_mouseY <= rowY + h);
		if (hovered) {
			rowColor = 0xAA3498DB;
		} else if (i == elem.selectedIndex) {
			rowColor = 0x881E1E24;
		}
		
		rowColor = ModulateAlpha(rowColor, multiplier);
		DWORD borderColor = ModulateAlpha(0xFF2A2A33, multiplier);
		DWORD textColor = ModulateAlpha(0xFFFFFFFF, multiplier);

		DrawDXRectangle(pDevice, x, rowY, w, h, rowColor);
		DrawDXBorder(pDevice, x, rowY, w, h, 1.0f, borderColor);

		float textY = rowY + (h - charHeight * scale) * 0.5f;
		DrawDXText(pDevice, elem.options[i], x + 10.0f, textY, textColor, scale, elem.font);
	}
}

void DrawDXListView(IDirect3DDevice9* pDevice, float x, float y, float w, float h, DWORD color, int selectedIndex, const std::vector<std::string>& options, int scrollOffset, const std::string& fontName, float scale, bool draggingScroll, float multiplier) {
	scale = 0.55f; // Lock widget text scale to 0.55f to prevent giant overflow
	DrawDXRectangle(pDevice, x, y, w, h, ModulateAlpha(0xFF0E0E12, multiplier));
	DrawDXBorder(pDevice, x, y, w, h, 1.5f, color);

	float rowHeight = 30.0f;
	int maxVisibleRows = (int)(h / rowHeight);
	int numItems = (int)options.size();

	float scrollbarW = 12.0f;
	bool hasScrollbar = (numItems > maxVisibleRows);
	float listW = hasScrollbar ? (w - scrollbarW) : w;

	const DXFont* pFont = GetFontOrCreate(pDevice, fontName);
	float charHeight = 32.0f;
	if (pFont) charHeight = pFont->charHeight;

	for (int i = 0; i < maxVisibleRows; i++) {
		int itemIdx = scrollOffset + i;
		if (itemIdx >= numItems) break;

		float rowY = y + i * rowHeight;
		DWORD rowColor = (itemIdx % 2 == 0) ? 0x0AFFFFFF : 0x00FFFFFF;

		bool hovered = (g_mouseX >= x && g_mouseX <= x + listW && g_mouseY >= rowY && g_mouseY <= rowY + rowHeight);
		if (hovered) {
			rowColor = 0x223498DB;
		}
		if (itemIdx == selectedIndex) {
			rowColor = 0x663498DB;
		}

		DrawDXRectangle(pDevice, x, rowY, listW, rowHeight, ModulateAlpha(rowColor, multiplier));

		float textY = rowY + (rowHeight - charHeight * scale) * 0.5f;
		DrawDXText(pDevice, options[itemIdx], x + 8.0f, textY, ModulateAlpha(0xFFFFFFFF, multiplier), scale, fontName);
	}

	if (hasScrollbar) {
		float sx = x + w - scrollbarW;
		DrawDXRectangle(pDevice, sx, y, scrollbarW, h, ModulateAlpha(0xFF14141A, multiplier));
		DrawDXBorder(pDevice, sx, y, scrollbarW, h, 1.0f, ModulateAlpha(0xFF2A2A33, multiplier));

		float visibleRatio = (float)maxVisibleRows / numItems;
		float thumbH = h * visibleRatio;
		if (thumbH < 20.0f) thumbH = 20.0f;

		float maxScrollOffset = (float)(numItems - maxVisibleRows);
		float scrollRatio = (maxScrollOffset > 0) ? (float)scrollOffset / maxScrollOffset : 0.0f;
		float thumbY = y + scrollRatio * (h - thumbH);

		DWORD thumbColor = 0xFF555565;
		bool thumbHovered = (g_mouseX >= sx && g_mouseX <= sx + scrollbarW && g_mouseY >= thumbY && g_mouseY <= thumbY + thumbH);
		if (thumbHovered || draggingScroll) {
			thumbColor = 0xFF3498DB;
		}
		DrawDXRectangle(pDevice, sx + 2.0f, thumbY + 2.0f, scrollbarW - 4.0f, thumbH - 4.0f, ModulateAlpha(thumbColor, multiplier));
	}
}

void DrawDXTabPanel(IDirect3DDevice9* pDevice, float x, float y, float w, float h, DWORD color, int selectedIndex, const std::vector<std::string>& options, const std::string& fontName, float scale, float multiplier) {
	scale = 0.55f; // Lock widget text scale to 0.55f to prevent giant overflow
	DrawDXRectangle(pDevice, x, y, w, h, ModulateAlpha(0xFF141724, multiplier));
	DrawDXBorder(pDevice, x, y, w, h, 1.5f, color);

	int numTabs = (int)options.size();
	if (numTabs <= 0) return;

	float tabHeight = 35.0f;
	float tabW = w / numTabs;

	const DXFont* pFont = GetFontOrCreate(pDevice, fontName);
	float charHeight = 32.0f;
	if (pFont) charHeight = pFont->charHeight;

	for (int i = 0; i < numTabs; i++) {
		float tx = x + i * tabW;
		bool isActive = (i == selectedIndex);

		DWORD tabBg = isActive ? 0xFF1E2235 : 0xFF0E101A;
		DrawDXRectangle(pDevice, tx, y, tabW, tabHeight, ModulateAlpha(tabBg, multiplier));
		DrawDXBorder(pDevice, tx, y, tabW, tabHeight, 1.0f, ModulateAlpha(0xFF2D324F, multiplier));

		float textWidth = 0.0f;
		if (pFont) {
			for (char c : options[i]) {
				unsigned char uc = (unsigned char)c;
				if (uc >= 32) textWidth += pFont->charWidths[uc] * scale;
			}
		}
		float textX = tx + (tabW - textWidth) * 0.5f;
		float textY = y + (tabHeight - charHeight * scale) * 0.5f;
		
		DWORD textColor = isActive ? 0xFFFFFFFF : 0xFF9E9E9E;
		DrawDXText(pDevice, options[i], textX, textY, ModulateAlpha(textColor, multiplier), scale, fontName);

		if (isActive) {
			DrawDXRectangle(pDevice, tx, y + tabHeight - 3.0f, tabW, 3.0f, color);
		}
	}
}

void DrawDXBorder(IDirect3DDevice9* pDevice, float x, float y, float w, float h, float thickness, DWORD color) {
	DrawDXRectangle(pDevice, x, y, w, thickness, color); // Top
	DrawDXRectangle(pDevice, x, y + h - thickness, w, thickness, color); // Bottom
	DrawDXRectangle(pDevice, x, y, thickness, h, color); // Left
	DrawDXRectangle(pDevice, x + w - thickness, y, thickness, h, color); // Right
}

LRESULT CALLBACK hkWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (uMsg == WM_MOUSEMOVE || uMsg == WM_LBUTTONDOWN || uMsg == WM_LBUTTONUP) {
		POINT pt;
		if (GetCursorPos(&pt)) {
			if (ScreenToClient(hWnd, &pt)) {
				RECT clientRect;
				if (GetClientRect(hWnd, &clientRect)) {
					float clientW = (float)(clientRect.right - clientRect.left);
					float clientH = (float)(clientRect.bottom - clientRect.top);
					if (clientW > 0.0f && clientH > 0.0f && g_screenWidth > 0.0f && g_screenHeight > 0.0f) {
						g_mouseX = ((float)pt.x / clientW) * g_screenWidth;
						g_mouseY = ((float)pt.y / clientH) * g_screenHeight;
					} else {
						g_mouseX = (float)pt.x;
						g_mouseY = (float)pt.y;
					}
				} else {
					g_mouseX = (float)pt.x;
					g_mouseY = (float)pt.y;
				}
			}
		}
	}

	if (uMsg == WM_MOUSEMOVE) {
		std::lock_guard<std::mutex> lock(g_dxMutex);
		if (g_activeSliderDragId != -1) {
			auto it = g_dxElements.find(g_activeSliderDragId);
			if (it != g_dxElements.end() && it->second.type == DXElementType::Slider) {
				auto& elem = it->second;
				float shiftX = 0.0f, shiftY = 0.0f;
				GetElementShift(elem, shiftX, shiftY);
				float absX = elem.x + shiftX;
				float val = (g_mouseX - absX) / elem.width;
				if (val < 0.0f) val = 0.0f;
				if (val > 1.0f) val = 1.0f;
				elem.scale = val; // Update local scale immediately for buttery smoothness

				// Throttle network updates to saniyede maks ~60 kez to prevent flooding and lag
				static DWORD lastSliderSendTime = 0;
				DWORD now = GetTickCount();
				if (now - lastSliderSendTime > 16) {
					lastSliderSendTime = now;
					RakNet::BitStream bs;
					bs.Write((uint8_t)5); // Slider change
					bs.Write(elem.id);
					bs.Write(val);
					rakhook::send_rpc(192, &bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, false);
				}
			}
		}
		else if (g_activeListViewScrollDragId != -1) {
			auto it = g_dxElements.find(g_activeListViewScrollDragId);
			if (it != g_dxElements.end() && it->second.type == DXElementType::ListView) {
				auto& elem = it->second;
				float shiftX = 0.0f, shiftY = 0.0f;
				GetElementShift(elem, shiftX, shiftY);
				float absY = elem.y + shiftY;
				
				float rowHeight = 30.0f;
				int maxVisibleRows = (int)(elem.height / rowHeight);
				int numItems = (int)elem.options.size();
				if (numItems > maxVisibleRows) {
					float visibleRatio = (float)maxVisibleRows / numItems;
					float thumbH = elem.height * visibleRatio;
					if (thumbH < 20.0f) thumbH = 20.0f;
					
					float ratio = (g_mouseY - (absY + thumbH * 0.5f)) / (elem.height - thumbH);
					if (ratio < 0.0f) ratio = 0.0f;
					if (ratio > 1.0f) ratio = 1.0f;
					int scrollOffset = (int)(ratio * (numItems - maxVisibleRows));
					if (scrollOffset < 0) scrollOffset = 0;
					if (scrollOffset > numItems - maxVisibleRows) scrollOffset = numItems - maxVisibleRows;
					elem.scrollOffset = scrollOffset;
				}
			}
		}
		else if (g_activeScrollContainerDragId != -1) {
			auto it = g_dxElements.find(g_activeScrollContainerDragId);
			if (it != g_dxElements.end() && it->second.type == DXElementType::ScrollContainer) {
				auto& elem = it->second;
				float shiftX = 0.0f, shiftY = 0.0f;
				GetElementShift(elem, shiftX, shiftY);
				float absY = elem.y + shiftY;
				
				float visibleRatio = elem.height / elem.contentHeight;
				float thumbH = elem.height * visibleRatio;
				if (thumbH < 20.0f) thumbH = 20.0f;
				
				float ratio = (g_mouseY - (absY + thumbH * 0.5f)) / (elem.height - thumbH);
				if (ratio < 0.0f) ratio = 0.0f;
				if (ratio > 1.0f) ratio = 1.0f;
				elem.progress = ratio;
				
				static DWORD lastScrollSendTime = 0;
				DWORD now = GetTickCount();
				if (now - lastScrollSendTime > 16) {
					lastScrollSendTime = now;
					RakNet::BitStream bs;
					bs.Write((uint8_t)13);
					bs.Write(elem.id);
					bs.Write(ratio);
					rakhook::send_rpc(192, &bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, false);
				}
			}
		}
		else if (g_activeColorPickerDragId != -1) {
			auto it = g_dxElements.find(g_activeColorPickerDragId);
			if (it != g_dxElements.end() && it->second.type == DXElementType::ColorPicker) {
				auto& elem = it->second;
				float shiftX = 0.0f, shiftY = 0.0f;
				GetElementShift(elem, shiftX, shiftY);
				float absX = elem.x + shiftX;
				float absY = elem.y + shiftY;
				
				float rx = (g_mouseX - absX) / elem.width;
				float ry = (g_mouseY - absY) / elem.height;
				if (rx < 0.0f) rx = 0.0f; if (rx > 1.0f) rx = 1.0f;
				if (ry < 0.0f) ry = 0.0f; if (ry > 1.0f) ry = 1.0f;
				
				DWORD baseColor = GetHueColor(rx);
				DWORD selectedColor = 0;
				if (ry < 0.5f) {
					float t = ry * 2.0f;
					BYTE r = (BYTE)(255 * (1.0f - t) + ((baseColor >> 16) & 0xFF) * t);
					BYTE g = (BYTE)(255 * (1.0f - t) + ((baseColor >> 8) & 0xFF) * t);
					BYTE b = (BYTE)(255 * (1.0f - t) + (baseColor & 0xFF) * t);
					selectedColor = 0xFF000000 | (r << 16) | (g << 8) | b;
				} else {
					float t = (ry - 0.5f) * 2.0f;
					BYTE r = (BYTE)(((baseColor >> 16) & 0xFF) * (1.0f - t));
					BYTE g = (BYTE)(((baseColor >> 8) & 0xFF) * (1.0f - t));
					BYTE b = (BYTE)((baseColor & 0xFF) * (1.0f - t));
					selectedColor = 0xFF000000 | (r << 16) | (g << 8) | b;
				}
				
				elem.color = selectedColor;
				
				static DWORD lastColorSendTime = 0;
				DWORD now = GetTickCount();
				if (now - lastColorSendTime > 20) {
					lastColorSendTime = now;
					RakNet::BitStream bs;
					bs.Write((uint8_t)12);
					bs.Write(elem.id);
					bs.Write((uint32_t)selectedColor);
					rakhook::send_rpc(192, &bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, false);
				}
			}
		}
	}
	else if (uMsg == WM_LBUTTONDOWN) {
		std::lock_guard<std::mutex> lock(g_dxMutex);
		int clickedInputId = -1;
		bool clickConsumed = false;

		// 1. Check ComboBox active dropdown items first
		for (auto& [id, elem] : g_dxElements) {
			if (elem.isDestroying) continue;
			if (elem.type == DXElementType::ComboBox && elem.isDropped) {
				float shiftX = 0.0f, shiftY = 0.0f;
				GetElementShift(elem, shiftX, shiftY);
				float absX = elem.x + shiftX;
				float absY = elem.y + shiftY;

				int numOptions = (int)elem.options.size();
				float dropY = absY + elem.height;
				float dropH = elem.height * numOptions;
				if (g_mouseX >= absX && g_mouseX <= absX + elem.width &&
					g_mouseY >= dropY && g_mouseY <= dropY + dropH) {
					
					int idx = (int)((g_mouseY - dropY) / elem.height);
					if (idx >= 0 && idx < numOptions) {
						elem.selectedIndex = idx;
						elem.isDropped = false;
						
						RakNet::BitStream bs;
						bs.Write((uint8_t)6); // ComboBox select
						bs.Write(elem.id);
						bs.Write(idx);
						rakhook::send_rpc(192, &bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, false);
						LogToFile("Interaction: ComboBox Select, ID=" + std::to_string(elem.id) + ", Index=" + std::to_string(idx));
					}
					clickConsumed = true;
					break;
				}
			}
		}

		// Close dropdowns if clicked outside
		if (!clickConsumed) {
			for (auto& [id, elem] : g_dxElements) {
				if (elem.type == DXElementType::ComboBox) {
					elem.isDropped = false;
				}
			}
		}

		// 2. Process other elements if click not consumed
		if (!clickConsumed) {
			for (auto& [id, elem] : g_dxElements) {
				if (elem.isDestroying) continue;

				float shiftX = 0.0f, shiftY = 0.0f;
				GetElementShift(elem, shiftX, shiftY);
				float absX = elem.x + shiftX;
				float absY = elem.y + shiftY;

				// Skip non-interactive static drawing elements
				if (elem.type == DXElementType::Rectangle ||
					elem.type == DXElementType::Text ||
					elem.type == DXElementType::Image ||
					elem.type == DXElementType::Line ||
					elem.type == DXElementType::Circle ||
					elem.type == DXElementType::Clip ||
					elem.type == DXElementType::GradientRectangle ||
					elem.type == DXElementType::RoundedRectangle ||
					elem.type == DXElementType::Triangle ||
					elem.type == DXElementType::Shadow) {
					continue;
				}

				if (g_mouseX >= absX && g_mouseX <= absX + elem.width &&
					g_mouseY >= absY && g_mouseY <= absY + elem.height) {
					
					if (elem.type == DXElementType::Button) {
						RakNet::BitStream bs;
						bs.Write((uint8_t)1); // Button click
						bs.Write(elem.id);
						rakhook::send_rpc(192, &bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, false);
						LogToFile("Interaction: Button Click, ID=" + std::to_string(elem.id));
						clickConsumed = true;
					}
					else if (elem.type == DXElementType::Checkbox) {
						elem.checked = !elem.checked;
						RakNet::BitStream bs;
						bs.Write((uint8_t)2); // Checkbox toggle
						bs.Write(elem.id);
						bs.Write(elem.checked);
						rakhook::send_rpc(192, &bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, false);
						LogToFile("Interaction: Checkbox Toggle, ID=" + std::to_string(elem.id) + ", Checked=" + std::to_string(elem.checked));
						clickConsumed = true;
					}
					else if (elem.type == DXElementType::Input) {
						clickedInputId = elem.id;
						LogToFile("Interaction: Input Focus, ID=" + std::to_string(elem.id));
						clickConsumed = true;
					}
					else if (elem.type == DXElementType::Slider) {
						g_activeSliderDragId = elem.id;
						float val = (g_mouseX - absX) / elem.width;
						if (val < 0.0f) val = 0.0f;
						if (val > 1.0f) val = 1.0f;
						elem.scale = val;

						RakNet::BitStream bs;
						bs.Write((uint8_t)5); // Slider change
						bs.Write(elem.id);
						bs.Write(val);
						rakhook::send_rpc(192, &bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, false);
						clickConsumed = true;
					}
					else if (elem.type == DXElementType::ComboBox) {
						elem.isDropped = !elem.isDropped;
						clickConsumed = true;
					}
					else if (elem.type == DXElementType::ListView) {
						float scrollbarW = 12.0f;
						int numItems = (int)elem.options.size();
						float rowHeight = 30.0f;
						int maxVisibleRows = (int)(elem.height / rowHeight);
						bool hasScrollbar = (numItems > maxVisibleRows);

						if (hasScrollbar && g_mouseX >= absX + elem.width - scrollbarW) {
							g_activeListViewScrollDragId = elem.id;
							elem.draggingScroll = true;
							
							float visibleRatio = (float)maxVisibleRows / numItems;
							float thumbH = elem.height * visibleRatio;
							if (thumbH < 20.0f) thumbH = 20.0f;

							float ratio = (g_mouseY - (absY + thumbH * 0.5f)) / (elem.height - thumbH);
							if (ratio < 0.0f) ratio = 0.0f;
							if (ratio > 1.0f) ratio = 1.0f;
							int scrollOffset = (int)(ratio * (numItems - maxVisibleRows));
							if (scrollOffset < 0) scrollOffset = 0;
							if (scrollOffset > numItems - maxVisibleRows) scrollOffset = numItems - maxVisibleRows;
							elem.scrollOffset = scrollOffset;
						} else {
							int rowIdx = elem.scrollOffset + (int)((g_mouseY - absY) / rowHeight);
							if (rowIdx >= 0 && rowIdx < numItems) {
								elem.selectedIndex = rowIdx;
								RakNet::BitStream bs;
								bs.Write((uint8_t)7); // ListView Select
								bs.Write(elem.id);
								bs.Write(rowIdx);
								rakhook::send_rpc(192, &bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, false);
								LogToFile("Interaction: ListView Select, ID=" + std::to_string(elem.id) + ", Index=" + std::to_string(rowIdx));
							}
						}
						clickConsumed = true;
					}
					else if (elem.type == DXElementType::TabPanel) {
						int numTabs = (int)elem.options.size();
						if (numTabs > 0) {
							float tabHeight = 35.0f;
							if (g_mouseY >= absY && g_mouseY <= absY + tabHeight) {
								float tabW = elem.width / numTabs;
								int idx = (int)((g_mouseX - absX) / tabW);
								if (idx >= 0 && idx < numTabs) {
									elem.selectedIndex = idx;
									RakNet::BitStream bs;
									bs.Write((uint8_t)8); // Tab Select
									bs.Write(elem.id);
									bs.Write(idx);
									rakhook::send_rpc(192, &bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, false);
									LogToFile("Interaction: Tab Select, ID=" + std::to_string(elem.id) + ", Index=" + std::to_string(idx));
								}
							}
						}
						clickConsumed = true;
					}
					else if (elem.type == DXElementType::RadialMenu) {
						if (elem.selectedIndex != -1) {
							RakNet::BitStream bs;
							bs.Write((uint8_t)11);
							bs.Write(elem.id);
							bs.Write(elem.selectedIndex);
							rakhook::send_rpc(192, &bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, false);
							LogToFile("Interaction: Radial Menu Click, ID=" + std::to_string(elem.id) + ", Index=" + std::to_string(elem.selectedIndex));
						}
						clickConsumed = true;
					}
					else if (elem.type == DXElementType::InventorySlot) {
						g_activeInventoryDragId = elem.id;
						clickConsumed = true;
						LogToFile("Interaction: Inventory Slot Drag Start, ID=" + std::to_string(elem.id));
					}
					else if (elem.type == DXElementType::ColorPicker) {
						g_activeColorPickerDragId = elem.id;
						
						float rx = (g_mouseX - absX) / elem.width;
						float ry = (g_mouseY - absY) / elem.height;
						if (rx < 0.0f) rx = 0.0f; if (rx > 1.0f) rx = 1.0f;
						if (ry < 0.0f) ry = 0.0f; if (ry > 1.0f) ry = 1.0f;
						
						DWORD baseColor = GetHueColor(rx);
						DWORD selectedColor = 0;
						if (ry < 0.5f) {
							float t = ry * 2.0f;
							BYTE r = (BYTE)(255 * (1.0f - t) + ((baseColor >> 16) & 0xFF) * t);
							BYTE g = (BYTE)(255 * (1.0f - t) + ((baseColor >> 8) & 0xFF) * t);
							BYTE b = (BYTE)(255 * (1.0f - t) + (baseColor & 0xFF) * t);
							selectedColor = 0xFF000000 | (r << 16) | (g << 8) | b;
						} else {
							float t = (ry - 0.5f) * 2.0f;
							BYTE r = (BYTE)(((baseColor >> 16) & 0xFF) * (1.0f - t));
							BYTE g = (BYTE)(((baseColor >> 8) & 0xFF) * (1.0f - t));
							BYTE b = (BYTE)((baseColor & 0xFF) * (1.0f - t));
							selectedColor = 0xFF000000 | (r << 16) | (g << 8) | b;
						}
						elem.color = selectedColor;
						
						RakNet::BitStream bs;
						bs.Write((uint8_t)12);
						bs.Write(elem.id);
						bs.Write((uint32_t)selectedColor);
						rakhook::send_rpc(192, &bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, false);
						clickConsumed = true;
						LogToFile("Interaction: ColorPicker click, ID=" + std::to_string(elem.id) + ", Color=" + std::to_string(selectedColor));
					}
					else if (elem.type == DXElementType::ScrollContainer) {
						float scrollbarW = 12.0f;
						float sbX = absX + elem.width - scrollbarW;
						if (g_mouseX >= sbX) {
							g_activeScrollContainerDragId = elem.id;
							
							float visibleRatio = elem.height / elem.contentHeight;
							float thumbH = elem.height * visibleRatio;
							if (thumbH < 20.0f) thumbH = 20.0f;
							
							float ratio = (g_mouseY - (absY + thumbH * 0.5f)) / (elem.height - thumbH);
							if (ratio < 0.0f) ratio = 0.0f;
							if (ratio > 1.0f) ratio = 1.0f;
							elem.progress = ratio;
							
							RakNet::BitStream bs;
							bs.Write((uint8_t)13);
							bs.Write(elem.id);
							bs.Write(ratio);
							rakhook::send_rpc(192, &bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, false);
						}
						clickConsumed = true;
					}
					break;
				}
			}
		}

		// 3. Check for draggable elements if click not consumed yet
		if (!clickConsumed) {
			for (auto it = g_dxElements.rbegin(); it != g_dxElements.rend(); ++it) {
				auto& elem = it->second;
				if (elem.isDestroying) continue;
				if (elem.draggable) {
					float shiftX = 0.0f, shiftY = 0.0f;
					GetElementShift(elem, shiftX, shiftY);
					float absX = elem.x + shiftX;
					float absY = elem.y + shiftY;
					if (g_mouseX >= absX && g_mouseX <= absX + elem.width &&
						g_mouseY >= absY && g_mouseY <= absY + elem.height) {
						g_activeDragElementId = elem.id;
						elem.isDragging = true;
						elem.dragOffsetX = g_mouseX - absX;
						elem.dragOffsetY = g_mouseY - absY;
						clickConsumed = true;
						LogToFile("Interaction: Drag Start, ID=" + std::to_string(elem.id) + 
							", OffsetX=" + std::to_string(elem.dragOffsetX) + 
							", OffsetY=" + std::to_string(elem.dragOffsetY));
						break;
					}
				}
			}
		}

		g_focusedInputId = clickedInputId;
	}
	else if (uMsg == WM_LBUTTONUP) {
		if (g_activeDragElementId != -1) {
			std::lock_guard<std::mutex> lock(g_dxMutex);
			auto it = g_dxElements.find(g_activeDragElementId);
			if (it != g_dxElements.end()) {
				it->second.isDragging = false;
				LogToFile("Interaction: Drag End, ID=" + std::to_string(g_activeDragElementId));
			}
			g_activeDragElementId = -1;
		}
		if (g_activeSliderDragId != -1) {
			std::lock_guard<std::mutex> lock(g_dxMutex);
			auto it = g_dxElements.find(g_activeSliderDragId);
			if (it != g_dxElements.end() && it->second.type == DXElementType::Slider) {
				auto& elem = it->second;
				float shiftX = 0.0f, shiftY = 0.0f;
				GetElementShift(elem, shiftX, shiftY);
				float absX = elem.x + shiftX;
				float val = (g_mouseX - absX) / elem.width;
				if (val < 0.0f) val = 0.0f;
				if (val > 1.0f) val = 1.0f;
				elem.scale = val;

				RakNet::BitStream bs;
				bs.Write((uint8_t)5); // Slider change
				bs.Write(elem.id);
				bs.Write(val);
				rakhook::send_rpc(192, &bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, false);
			}
			g_activeSliderDragId = -1;
		}
		if (g_activeListViewScrollDragId != -1) {
			std::lock_guard<std::mutex> lock(g_dxMutex);
			auto it = g_dxElements.find(g_activeListViewScrollDragId);
			if (it != g_dxElements.end()) {
				it->second.draggingScroll = false;
			}
			g_activeListViewScrollDragId = -1;
		}
		if (g_activeScrollContainerDragId != -1) {
			g_activeScrollContainerDragId = -1;
		}
		if (g_activeColorPickerDragId != -1) {
			g_activeColorPickerDragId = -1;
		}
		if (g_activeInventoryDragId != -1) {
			std::lock_guard<std::mutex> lock(g_dxMutex);
			int targetId = -1;
			for (auto const& [id, elem] : g_dxElements) {
				if (elem.type == DXElementType::InventorySlot && id != g_activeInventoryDragId) {
					float shiftX = 0.0f, shiftY = 0.0f;
					GetElementShift(elem, shiftX, shiftY);
					float absX = elem.x + shiftX;
					float absY = elem.y + shiftY;
					if (g_mouseX >= absX && g_mouseX <= absX + elem.width &&
						g_mouseY >= absY && g_mouseY <= absY + elem.height) {
						targetId = id;
						break;
					}
				}
			}
			if (targetId != -1) {
				RakNet::BitStream bs;
				bs.Write((uint8_t)10); // Swap slots
				bs.Write(g_activeInventoryDragId);
				bs.Write(targetId);
				rakhook::send_rpc(192, &bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, false);
				LogToFile("Interaction: Swap Slots, Source=" + std::to_string(g_activeInventoryDragId) + ", Target=" + std::to_string(targetId));
			}
			g_activeInventoryDragId = -1;
		}
	}
	else if (uMsg == WM_MOUSEWHEEL) {
		int delta = GET_WHEEL_DELTA_WPARAM(wParam);
		std::lock_guard<std::mutex> lock(g_dxMutex);
		for (auto& [id, elem] : g_dxElements) {
			if (elem.type == DXElementType::ListView) {
				if (g_mouseX >= elem.x && g_mouseX <= elem.x + elem.width &&
					g_mouseY >= elem.y && g_mouseY <= elem.y + elem.height) {
					float rowHeight = 30.0f;
					int maxVisibleRows = (int)(elem.height / rowHeight);
					int numItems = (int)elem.options.size();
					if (numItems > maxVisibleRows) {
						int scrollStep = (delta > 0) ? -2 : 2;
						elem.scrollOffset += scrollStep;
						if (elem.scrollOffset < 0) elem.scrollOffset = 0;
						if (elem.scrollOffset > numItems - maxVisibleRows) elem.scrollOffset = numItems - maxVisibleRows;
					}
					break;
				}
			}
			else if (elem.type == DXElementType::ScrollContainer) {
				float shiftX = 0.0f, shiftY = 0.0f;
				GetElementShift(elem, shiftX, shiftY);
				float absX = elem.x + shiftX;
				float absY = elem.y + shiftY;
				if (g_mouseX >= absX && g_mouseX <= absX + elem.width &&
					g_mouseY >= absY && g_mouseY <= absY + elem.height) {
					float maxScroll = elem.contentHeight - elem.height;
					if (maxScroll > 0.0f) {
						float deltaProgress = (delta > 0) ? -0.05f : 0.05f;
						elem.progress += deltaProgress;
						if (elem.progress < 0.0f) elem.progress = 0.0f;
						if (elem.progress > 1.0f) elem.progress = 1.0f;
						
						RakNet::BitStream bs;
						bs.Write((uint8_t)13);
						bs.Write(elem.id);
						bs.Write(elem.progress);
						rakhook::send_rpc(192, &bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, false);
					}
					break;
				}
			}
		}
	}
	else if (g_focusedInputId != -1) {
		if (uMsg == WM_CHAR) {
			std::lock_guard<std::mutex> lock(g_dxMutex);
			auto it = g_dxElements.find(g_focusedInputId);
			if (it != g_dxElements.end() && it->second.type == DXElementType::Input) {
				auto& elem = it->second;
				if (wParam == VK_BACK) {
					if (!elem.text.empty()) {
						elem.text.pop_back();
						RakNet::BitStream bs;
						bs.Write((uint8_t)3); // Input change
						bs.Write(elem.id);
						uint16_t len = (uint16_t)elem.text.size();
						bs.Write(len);
						if (len > 0) {
							bs.Write(elem.text.c_str(), len);
						}
						rakhook::send_rpc(192, &bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, false);
					}
				}
				else if (wParam == VK_RETURN) {
					RakNet::BitStream bs;
					bs.Write((uint8_t)4); // Input submit
					bs.Write(elem.id);
					uint16_t len = (uint16_t)elem.text.size();
					bs.Write(len);
					if (len > 0) {
						bs.Write(elem.text.c_str(), len);
					}
					rakhook::send_rpc(192, &bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, false);
					LogToFile("Interaction: Input Submit, ID=" + std::to_string(elem.id) + ", Text=" + elem.text);
					g_focusedInputId = -1; // Unfocus on submit
				}
				else if (wParam >= 32) {
					elem.text.push_back((char)wParam);
					RakNet::BitStream bs;
					bs.Write((uint8_t)3); // Input change
					bs.Write(elem.id);
					uint16_t len = (uint16_t)elem.text.size();
					bs.Write(len);
					if (len > 0) {
						bs.Write(elem.text.c_str(), len);
					}
					rakhook::send_rpc(192, &bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, false);
				}
			}
			return 1; // Consume character
		}
		else if (uMsg == WM_KEYDOWN) {
			if (wParam == VK_ESCAPE) {
				g_focusedInputId = -1;
				return 1; // Consume escape
			}
			if ((wParam >= 'A' && wParam <= 'Z') || (wParam >= '0' && wParam <= '9') || wParam == VK_SPACE || wParam == VK_BACK || wParam == VK_RETURN) {
				return 1;
			}
		}
		else if (uMsg == WM_KEYUP) {
			if ((wParam >= 'A' && wParam <= 'Z') || (wParam >= '0' && wParam <= '9') || wParam == VK_SPACE || wParam == VK_BACK || wParam == VK_RETURN) {
				return 1;
			}
		}
	}

	return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}

const wchar_t* fontAwesomeIcons[] = {
	L"\uf013", // 128: gear / cog
	L"\uf004", // 129: heart
	L"\uf007", // 130: user
	L"\uf023", // 131: lock
	L"\uf005", // 132: star
	L"\uf00c", // 133: check
	L"\uf00d", // 134: close / times
	L"\uf028", // 135: volume-up
	L"\uf001", // 136: music
	L"\uf095", // 137: phone
	L"\uf07a", // 138: shopping-cart
	L"\uf015", // 139: home
	L"\uf002", // 140: search
	L"\uf0f3", // 141: bell
	L"\uf0ad", // 142: wrench
	L"\uf0e4", // 143: speedometer / tachometer
	L"\uf011", // 144: power-off (engine)
	L"\uf0eb", // 145: lightbulb-o (lights)
	L"\uf0f9", // 146: ambulance
	L"\uf1b9", // 147: car
	L"\uf0f4", // 148: coffee
	L"\uf000", // 149: glass
	L"\uf085", // 150: cogs
	L"\uf019", // 151: download
	L"\uf093", // 152: upload
	L"\uf0e0", // 153: envelope (mail)
	L"\uf040", // 154: pencil
	L"\uf014", // 155: trash
	L"\uf030", // 156: camera
	L"\uf0fc", // 157: beer
	L"\uf206", // 158: bicycle
	L"\uf0f2", // 159: suitcase
	L"\uf0e7", // 160: bolt / lightning
	L"\uf04b", // 161: play
	L"\uf04c", // 162: pause
	L"\uf04d", // 163: stop
	L"\uf026", // 164: volume-off
	L"\uf027", // 165: volume-down
	L"\uf021", // 166: refresh
	L"\uf069", // 167: asterisk
	L"\uf091", // 168: trophy
	L"\uf13d", // 169: anchor
	L"\uf0f5", // 170: cutlery (food)
	L"\uf233", // 171: server
	L"\uf10b", // 172: mobile
	L"\uf108", // 173: desktop / pc
	L"\uf13b", // 174: html5
	L"\uf179", // 175: apple
	L"\uf17a", // 176: windows
	L"\uf17b", // 177: android
	L"\uf1a0", // 178: google
	L"\uf09b", // 179: github
	L"\uf08a", // 180: heart-o
	L"\uf006", // 181: star-o
	L"\uf0a9", // 182: arrow-circle-right
	L"\uf0a8", // 183: arrow-circle-left
	L"\uf0aa", // 184: arrow-circle-up
	L"\uf0ab", // 185: arrow-circle-down
	L"\uf01e", // 186: undo
	L"\uf086", // 187: comments / chat
	L"\uf0c0", // 188: users
	L"\uf07c", // 189: folder-open
	L"\uf0d6", // 190: money
	L"\uf0a1", // 191: megaphone
	L"\uf124", // 192: location-arrow
	L"\uf022", // 193: list-alt
	L"\uf11b", // 194: gamepad
	L"\uf2be", // 195: user-circle
	L"\uf12b", // 196: virus
	L"\uf1ae", // 197: child / walk
	L"\uf05a", // 198: info-circle
	L"\uf059", // 199: question-circle
	L"\uf057", // 200: times-circle
	L"\uf058", // 201: check-circle
	L"\uf071", // 202: warning
	L"\uf0e3", // 203: gavel
	L"\uf073", // 204: calendar
	L"\uf017", // 205: clock
	L"\uf06e", // 206: eye
	L"\uf070", // 207: eye-slash
	L"\uf112", // 208: reply
	L"\uf0c9", // 209: bars / menu
};

int GetFontAwesomeIconChar(const std::string& name) {
	if (name == "gear" || name == "cog") return 128;
	if (name == "heart") return 129;
	if (name == "user") return 130;
	if (name == "lock") return 131;
	if (name == "star") return 132;
	if (name == "check") return 133;
	if (name == "close" || name == "times") return 134;
	if (name == "volume-up" || name == "volume") return 135;
	if (name == "music") return 136;
	if (name == "phone") return 137;
	if (name == "shopping-cart" || name == "cart") return 138;
	if (name == "home") return 139;
	if (name == "search") return 140;
	if (name == "bell") return 141;
	if (name == "wrench") return 142;
	if (name == "tachometer" || name == "speedometer") return 143;
	if (name == "power-off" || name == "engine") return 144;
	if (name == "lightbulb-o" || name == "lightbulb" || name == "lights") return 145;
	if (name == "ambulance") return 146;
	if (name == "car") return 147;
	if (name == "coffee") return 148;
	if (name == "glass") return 149;
	if (name == "cogs") return 150;
	if (name == "download") return 151;
	if (name == "upload") return 152;
	if (name == "envelope" || name == "mail") return 153;
	if (name == "pencil") return 154;
	if (name == "trash") return 155;
	if (name == "camera") return 156;
	if (name == "beer") return 157;
	if (name == "bicycle") return 158;
	if (name == "suitcase") return 159;
	if (name == "bolt" || name == "lightning") return 160;
	if (name == "play") return 161;
	if (name == "pause") return 162;
	if (name == "stop") return 163;
	if (name == "volume-off") return 164;
	if (name == "volume-down") return 165;
	if (name == "refresh") return 166;
	if (name == "asterisk") return 167;
	if (name == "trophy") return 168;
	if (name == "anchor") return 169;
	if (name == "cutlery" || name == "food") return 170;
	if (name == "server") return 171;
	if (name == "mobile") return 172;
	if (name == "desktop" || name == "pc") return 173;
	if (name == "html5") return 174;
	if (name == "apple") return 175;
	if (name == "windows") return 176;
	if (name == "android") return 177;
	if (name == "google") return 178;
	if (name == "github") return 179;
	if (name == "heart-o") return 180;
	if (name == "star-o") return 181;
	if (name == "arrow-circle-right") return 182;
	if (name == "arrow-circle-left") return 183;
	if (name == "arrow-circle-up") return 184;
	if (name == "arrow-circle-down") return 185;
	if (name == "undo") return 186;
	if (name == "comments" || name == "chat") return 187;
	if (name == "users") return 188;
	if (name == "folder-open") return 189;
	if (name == "money") return 190;
	if (name == "megaphone") return 191;
	if (name == "location-arrow") return 192;
	if (name == "list-alt") return 193;
	if (name == "gamepad") return 194;
	if (name == "user-circle") return 195;
	if (name == "virus") return 196;
	if (name == "child" || name == "walk") return 197;
	if (name == "info-circle" || name == "info") return 198;
	if (name == "question-circle") return 199;
	if (name == "times-circle") return 200;
	if (name == "check-circle") return 201;
	if (name == "exclamation-triangle" || name == "warning") return 202;
	if (name == "gavel") return 203;
	if (name == "calendar") return 204;
	if (name == "clock") return 205;
	if (name == "eye") return 206;
	if (name == "eye-slash") return 207;
	if (name == "reply") return 208;
	if (name == "bars" || name == "menu") return 209;
	return 0;
}

void LoadFontLocal(const std::string& fontFamily, const std::string& localPath);
void DownloadAndLoadFont(std::string fontFamily, std::string url, std::string localPath);

const DXFont* GetFontOrCreate(IDirect3DDevice9* pDevice, const std::string& fontName) {
	std::string activeFont = fontName;
	if (activeFont.empty()) {
		activeFont = "Segoe UI";
	}

	{
		std::lock_guard<std::mutex> lock(g_fontMutex);
		auto it = g_dxFonts.find(activeFont);
		if (it != g_dxFonts.end()) {
			return &it->second;
		}
	}

	DXFont newFont;
	if (FAILED(pDevice->CreateTexture(512, 512, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &newFont.texture, NULL))) {
		LogToFile("GetFontOrCreate: FAILED to create font texture for: " + activeFont);
		return nullptr;
	}

	D3DLOCKED_RECT rect;
	if (SUCCEEDED(newFont.texture->LockRect(0, &rect, NULL, 0))) {
		HDC hdcMem = CreateCompatibleDC(NULL);
		BITMAPINFO bmi = { 0 };
		bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmi.bmiHeader.biWidth = 512;
		bmi.bmiHeader.biHeight = -512; // top-down
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = 32;
		bmi.bmiHeader.biCompression = BI_RGB;

		void* pBits = nullptr;
		HBITMAP hbmMem = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
		HGDIOBJ hOldBmp = SelectObject(hdcMem, hbmMem);

		HFONT hFont = CreateFontA(24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, activeFont.c_str());
		HGDIOBJ hOldFont = SelectObject(hdcMem, hFont);

		SetTextColor(hdcMem, RGB(255, 255, 255));
		SetBkColor(hdcMem, RGB(0, 0, 0));
		SetBkMode(hdcMem, TRANSPARENT);
		memset(pBits, 0, 512 * 512 * 4);

		newFont.charHeight = 32.0f;
		bool isFontAwesome = (activeFont == "FontAwesome");
		for (int i = 32; i < 256; i++) {
			int col = i % 16;
			int row = i / 16;
			int cx = col * 32;
			int cy = row * 32;

			SIZE sz;
			if (isFontAwesome && i >= 128 && i < 128 + (sizeof(fontAwesomeIcons) / sizeof(fontAwesomeIcons[0]))) {
				const wchar_t* wstr = fontAwesomeIcons[i - 128];
				GetTextExtentPoint32W(hdcMem, wstr, 1, &sz);
				newFont.charWidths[i] = (float)sz.cx;
				TextOutW(hdcMem, cx, cy, wstr, 1);
			} else {
				char str[2] = { (char)i, 0 };
				GetTextExtentPoint32A(hdcMem, str, 1, &sz);
				newFont.charWidths[i] = (float)sz.cx;
				TextOutA(hdcMem, cx, cy, str, 1);
			}
		}

		unsigned char* pDest = (unsigned char*)rect.pBits;
		unsigned char* pSrc = (unsigned char*)pBits;
		for (int ty = 0; ty < 512; ty++) {
			for (int tx = 0; tx < 512; tx++) {
				int idx = (ty * 512 + tx) * 4;
				unsigned char val = pSrc[idx + 2]; // Use red channel for alpha
				pDest[ty * rect.Pitch + tx * 4 + 0] = 255;
				pDest[ty * rect.Pitch + tx * 4 + 1] = 255;
				pDest[ty * rect.Pitch + tx * 4 + 2] = 255;
				pDest[ty * rect.Pitch + tx * 4 + 3] = val;
			}
		}

		newFont.texture->UnlockRect(0);
		SelectObject(hdcMem, hOldFont);
		DeleteObject(hFont);
		SelectObject(hdcMem, hOldBmp);
		DeleteObject(hbmMem);
		DeleteDC(hdcMem);
		LogToFile("GetFontOrCreate: Font texture initialized successfully for: " + activeFont);
	} else {
		LogToFile("GetFontOrCreate: FAILED to lock font texture for: " + activeFont);
		newFont.texture->Release();
		return nullptr;
	}

	std::lock_guard<std::mutex> lock(g_fontMutex);
	g_dxFonts[activeFont] = newFont;
	return &g_dxFonts[activeFont];
}

bool IsFontRegistered(const std::string& fontName) {
	if (fontName.empty() || fontName == "Segoe UI" || fontName == "Arial" || fontName == "Tahoma" || fontName == "Courier New" || fontName == "Outfit-Bold" || fontName == "Outfit") {
		return true;
	}
	std::string matchSuffix = "\\" + fontName + ".ttf";
	std::string matchSuffix2 = "/" + fontName + ".ttf";
	std::lock_guard<std::mutex> lock(g_fontMutex);
	for (const auto& path : g_loadedFontPaths) {
		if (path.length() >= matchSuffix.length()) {
			std::string suffix = path.substr(path.length() - matchSuffix.length());
			for (char& c : suffix) c = tolower(c);
			std::string matchLower = matchSuffix;
			for (char& c : matchLower) c = tolower(c);
			if (suffix == matchLower) return true;
		}
		if (path.length() >= matchSuffix2.length()) {
			std::string suffix = path.substr(path.length() - matchSuffix2.length());
			for (char& c : suffix) c = tolower(c);
			std::string matchLower = matchSuffix2;
			for (char& c : matchLower) c = tolower(c);
			if (suffix == matchLower) return true;
		}
	}
	return false;
}

void DrawDXText(IDirect3DDevice9* pDevice, const std::string& text, float x, float y, DWORD color, float scale, const std::string& fontName = "") {
	const DXFont* pFont = GetFontOrCreate(pDevice, fontName);
	if (!pFont) return;

	pDevice->SetTexture(0, pFont->texture);
	pDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	pDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	pDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
	pDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
	pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
	pDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	pDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
 
	pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
 
	pDevice->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1);
 
	float curX = x;
	DWORD activeColor = color;
	for (size_t i = 0; i < text.length(); ++i) {
		if (text[i] == '{' && i + 7 < text.length() && text[i + 7] == '}') {
			std::string hexStr = text.substr(i + 1, 6);
			bool validHex = true;
			for (char hc : hexStr) {
				if (!((hc >= '0' && hc <= '9') || (hc >= 'a' && hc <= 'f') || (hc >= 'A' && hc <= 'F'))) {
					validHex = false;
					break;
				}
			}
			if (validHex) {
				unsigned int parsedVal = 0;
				for (int k = 0; k < 6; ++k) {
					char hc = hexStr[k];
					unsigned int val = 0;
					if (hc >= '0' && hc <= '9') val = hc - '0';
					else if (hc >= 'a' && hc <= 'f') val = hc - 'a' + 10;
					else if (hc >= 'A' && hc <= 'F') val = hc - 'A' + 10;
					parsedVal = (parsedVal << 4) | val;
				}
				DWORD alpha = color & 0xFF000000;
				activeColor = alpha | parsedVal;
				i += 7;
				continue;
			}
		}

		unsigned char uc = (unsigned char)text[i];
		if (uc < 32) continue;
 
		int col = uc % 16;
		int row = uc / 16;
		float u1 = col / 16.0f;
		float v1 = row / 16.0f;
		float u2 = u1 + pFont->charWidths[uc] / 512.0f;
		float v2 = v1 + 32.0f / 512.0f;
 
		float w = pFont->charWidths[uc] * scale;
		float h = pFont->charHeight * scale;
 
		FontVertex vertices[4] = {
			{ curX,     y,     0.0f, 1.0f, activeColor, u1, v1 },
			{ curX + w, y,     0.0f, 1.0f, activeColor, u2, v1 },
			{ curX,     y + h, 0.0f, 1.0f, activeColor, u1, v2 },
			{ curX + w, y + h, 0.0f, 1.0f, activeColor, u2, v2 }
		};
 
		pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(FontVertex));
		curX += w;
	}
}

void CleanupBlurTextures() {
	LogToFile("CleanupBlurTextures called.");
	if (g_pBlurTexture1) { g_pBlurTexture1->Release(); g_pBlurTexture1 = nullptr; }
	if (g_pBlurTexture2) { g_pBlurTexture2->Release(); g_pBlurTexture2 = nullptr; }
	g_blurTexW = 0;
	g_blurTexH = 0;
}

void UpdateBlurTextures(IDirect3DDevice9* pDevice, int w, int h) {
	if (g_blurTexW != w || g_blurTexH != h || !g_pBlurTexture1 || !g_pBlurTexture2) {
		if (g_pBlurTexture1) { g_pBlurTexture1->Release(); g_pBlurTexture1 = nullptr; }
		if (g_pBlurTexture2) { g_pBlurTexture2->Release(); g_pBlurTexture2 = nullptr; }
		
		g_blurTexW = w;
		g_blurTexH = h;
		
		int texW = w / 8;
		int texH = h / 8;
		if (texW < 4) texW = 4;
		if (texH < 4) texH = 4;
		
		HRESULT hr = pDevice->CreateTexture(texW, texH, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &g_pBlurTexture1, NULL);
		if (FAILED(hr)) {
			LogToFile("UpdateBlurTextures: Failed to create BlurTexture1, hr=" + std::to_string(hr));
		}
		hr = pDevice->CreateTexture(texW, texH, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &g_pBlurTexture2, NULL);
		if (FAILED(hr)) {
			LogToFile("UpdateBlurTextures: Failed to create BlurTexture2, hr=" + std::to_string(hr));
		}
	}
}

void RenderBlur(IDirect3DDevice9* pDevice, float x, float y, float w, float h) {
	if (g_screenWidth <= 0.0f || g_screenHeight <= 0.0f) return;
	
	UpdateBlurTextures(pDevice, (int)g_screenWidth, (int)g_screenHeight);
	if (!g_pBlurTexture1 || !g_pBlurTexture2) return;
	
	IDirect3DSurface9* pBackBuffer = nullptr;
	if (FAILED(pDevice->GetRenderTarget(0, &pBackBuffer)) || !pBackBuffer) return;
	
	IDirect3DSurface9* pBlurSurface1 = nullptr;
	if (SUCCEEDED(g_pBlurTexture1->GetSurfaceLevel(0, &pBlurSurface1)) && pBlurSurface1) {
		RECT srcRect = { (LONG)x, (LONG)y, (LONG)(x + w), (LONG)(y + h) };
		
		float scaleX = (float)(g_blurTexW / 8) / g_screenWidth;
		float scaleY = (float)(g_blurTexH / 8) / g_screenHeight;
		RECT destRect = { (LONG)(x * scaleX), (LONG)(y * scaleY), (LONG)((x + w) * scaleX), (LONG)((y + h) * scaleY) };
		
		if (destRect.right > destRect.left && destRect.bottom > destRect.top) {
			pDevice->StretchRect(pBackBuffer, &srcRect, pBlurSurface1, &destRect, D3DTEXF_LINEAR);
		}
		pBlurSurface1->Release();
	}
	pBackBuffer->Release();
	
	pDevice->SetTexture(0, g_pBlurTexture1);
	pDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
	pDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
	pDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	pDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	pDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	
	float u1 = (x / g_screenWidth);
	float v1 = (y / g_screenHeight);
	float u2 = ((x + w) / g_screenWidth);
	float v2 = ((y + h) / g_screenHeight);
	
	FontVertex vertices[4] = {
		{ x,     y,     0.0f, 1.0f, 0xFFFFFFFF, u1, v1 },
		{ x + w, y,     0.0f, 1.0f, 0xFFFFFFFF, u2, v1 },
		{ x,     y + h, 0.0f, 1.0f, 0xFFFFFFFF, u1, v2 },
		{ x + w, y + h, 0.0f, 1.0f, 0xFFFFFFFF, u2, v2 }
	};
	pDevice->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1);
	pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(FontVertex));
	
	pDevice->SetTexture(0, NULL);
}

DWORD GetHueColor(float rx) {
	if (rx < 0.0f) rx = 0.0f;
	if (rx > 1.0f) rx = 1.0f;
	float h = rx * 6.0f;
	int i = (int)h;
	float t = h - i;
	BYTE r = 0, g = 0, b = 0;
	switch (i) {
		case 0: r = 255; g = (BYTE)(t * 255); b = 0; break;
		case 1: r = (BYTE)((1.0f - t) * 255); g = 255; b = 0; break;
		case 2: r = 0; g = 255; b = (BYTE)(t * 255); break;
		case 3: r = 0; g = (BYTE)((1.0f - t) * 255); b = 255; break;
		case 4: r = (BYTE)(t * 255); g = 0; b = 255; break;
		case 5: default: r = 255; g = 0; b = (BYTE)((1.0f - t) * 255); break;
	}
	return 0xFF000000 | (r << 16) | (g << 8) | b;
}

void DrawDXColorPicker(IDirect3DDevice9* pDevice, float x, float y, float w, float h, DWORD selectedColor, float multiplier) {
	int numSegs = 6;
	float segW = w / numSegs;
	float halfH = h / 2.0f;
	
	for (int i = 0; i < numSegs; i++) {
		float sx1 = x + i * segW;
		float sx2 = x + (i + 1) * segW;
		DWORD c1 = GetHueColor((float)i / numSegs);
		DWORD c2 = GetHueColor((float)(i + 1) / numSegs);
		
		DrawDXGradientRectangle(pDevice, sx1, y, segW, halfH, 
			ModulateAlpha(0xFFFFFFFF, multiplier), 
			ModulateAlpha(0xFFFFFFFF, multiplier), 
			ModulateAlpha(c1, multiplier), 
			ModulateAlpha(c2, multiplier));
	}
	
	for (int i = 0; i < numSegs; i++) {
		float sx1 = x + i * segW;
		float sx2 = x + (i + 1) * segW;
		DWORD c1 = GetHueColor((float)i / numSegs);
		DWORD c2 = GetHueColor((float)(i + 1) / numSegs);
		
		DrawDXGradientRectangle(pDevice, sx1, y + halfH, segW, halfH, 
			ModulateAlpha(c1, multiplier), 
			ModulateAlpha(c2, multiplier), 
			ModulateAlpha(0xFF000000, multiplier), 
			ModulateAlpha(0xFF000000, multiplier));
	}
	
	DrawDXRectangle(pDevice, x - 1.0f, y - 1.0f, w + 2.0f, h + 2.0f, ModulateAlpha(0x88FFFFFF, multiplier));
}


void GetElementShift(const DXElement& elem, float& shiftX, float& shiftY) {
	shiftX = 0.0f;
	shiftY = 0.0f;
	int pid = elem.parentId;
	int depth = 0;
	while (pid != -1 && depth < 10) {
		auto it = g_dxElements.find(pid);
		if (it != g_dxElements.end()) {
			shiftX += it->second.x;
			shiftY += it->second.y;
			if (it->second.type == DXElementType::ScrollContainer) {
				float maxScroll = it->second.contentHeight - it->second.height;
				if (maxScroll > 0.0f) {
					shiftY -= it->second.progress * maxScroll;
				}
			}
			pid = it->second.parentId;
			depth++;
		} else {
			break;
		}
	}
}

float GetElementAlphaMultiplier(const DXElement& elem, DWORD now) {
	float mult = 1.0f;
	
	// Fade-in progress
	if (elem.creationTime != 0) {
		DWORD elapsed = now - elem.creationTime;
		float progress = (float)elapsed / 250.0f;
		if (progress < 0.0f) progress = 0.0f;
		if (progress > 1.0f) progress = 1.0f;
		mult = (std::min)(mult, progress);
	}
	
	// Fade-out progress
	if (elem.isDestroying) {
		DWORD elapsed = now - elem.destroyRequestedTime;
		float progress = 1.0f - ((float)elapsed / 200.0f);
		if (progress < 0.0f) progress = 0.0f;
		if (progress > 1.0f) progress = 1.0f;
		mult = (std::min)(mult, progress);
	}
	
	// Recursive parent alpha modulation
	int pid = elem.parentId;
	int depth = 0;
	while (pid != -1 && depth < 10) {
		auto it = g_dxElements.find(pid);
		if (it != g_dxElements.end()) {
			if (it->second.creationTime != 0) {
				DWORD elapsed = now - it->second.creationTime;
				float progress = (float)elapsed / 250.0f;
				if (progress < 0.0f) progress = 0.0f;
				if (progress > 1.0f) progress = 1.0f;
				mult = (std::min)(mult, progress);
			}
			if (it->second.isDestroying) {
				DWORD elapsed = now - it->second.destroyRequestedTime;
				float progress = 1.0f - ((float)elapsed / 200.0f);
				if (progress < 0.0f) progress = 0.0f;
				if (progress > 1.0f) progress = 1.0f;
				mult = (std::min)(mult, progress);
			}
			pid = it->second.parentId;
			depth++;
		} else {
			break;
		}
	}
	
	return mult;
}

DWORD ModulateAlpha(DWORD color, float multiplier) {
	if (multiplier >= 1.0f) return color;
	if (multiplier <= 0.0f) return color & 0x00FFFFFF;
	BYTE a = (BYTE)((color >> 24) & 0xFF);
	BYTE r = (BYTE)((color >> 16) & 0xFF);
	BYTE g = (BYTE)((color >> 8) & 0xFF);
	BYTE b = (BYTE)(color & 0xFF);
	BYTE newA = (BYTE)(a * multiplier);
	return (newA << 24) | (r << 16) | (g << 8) | b;
}

void RenderDXElements(IDirect3DDevice9* pDevice) {
	std::lock_guard<std::mutex> lock(g_dxMutex);

	// 1. Query real-time physical cursor position at rendering framerate
	if (hWnd) {
		POINT pt;
		if (GetCursorPos(&pt)) {
			if (ScreenToClient(hWnd, &pt)) {
				RECT clientRect;
				if (GetClientRect(hWnd, &clientRect)) {
					float clientW = (float)(clientRect.right - clientRect.left);
					float clientH = (float)(clientRect.bottom - clientRect.top);
					if (clientW > 0.0f && clientH > 0.0f && g_screenWidth > 0.0f && g_screenHeight > 0.0f) {
						g_mouseX = ((float)pt.x / clientW) * g_screenWidth;
						g_mouseY = ((float)pt.y / clientH) * g_screenHeight;
					} else {
						g_mouseX = (float)pt.x;
						g_mouseY = (float)pt.y;
					}
				}
			}
		}
	}

	// 2. Perform smooth drag positioning in the rendering loop
	if (g_activeDragElementId != -1) {
		auto it = g_dxElements.find(g_activeDragElementId);
		if (it != g_dxElements.end() && it->second.draggable) {
			auto& elem = it->second;
			float shiftX = 0.0f, shiftY = 0.0f;
			GetElementShift(elem, shiftX, shiftY);
			float newAbsX = g_mouseX - elem.dragOffsetX;
			float newAbsY = g_mouseY - elem.dragOffsetY;
			float newX = newAbsX - shiftX;
			float newY = newAbsY - shiftY;
			float deltaX = newX - elem.x;
			float deltaY = newY - elem.y;

			if (deltaX != 0.0f || deltaY != 0.0f) {
				elem.x = newX;
				elem.y = newY;

				// Shift children locally in real-time (only if they don't have a parent, to avoid double-shifting)
				for (auto& [id, child] : g_dxElements) {
					if (id > elem.id && id <= elem.id + 100 && child.parentId == -1) {
						child.x += deltaX;
						child.y += deltaY;
						child.x2 += deltaX;
						child.y2 += deltaY;
						child.x3 += deltaX;
						child.y3 += deltaY;
					}
				}

				// Throttle sending coordinates to the server so we don't flood the network (~60 packets/sec)
				static DWORD lastSendTime = 0;
				DWORD now = GetTickCount();
				if (now - lastSendTime > 15) {
					lastSendTime = now;
					RakNet::BitStream bs;
					bs.Write((uint8_t)9); // Drag & Drop coordinate update
					bs.Write(elem.id);
					bs.Write(elem.x);
					bs.Write(elem.y);
					rakhook::send_rpc(192, &bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, false);
				}
			}
		}
	}

	if (g_dxElements.empty()) {
		return;
	}

	// Backup render states
	DWORD colorOp, colorArg1, alphaOp, alphaArg1, alphaArg2, blendEnable, srcBlend, destBlend, fvf;
	DWORD zEnable, zWrite, cullMode, lighting;
	DWORD scissorEnable, shadeMode;
	RECT prevScissorRect;
	IDirect3DBaseTexture9* prevTex = nullptr;

	pDevice->GetTexture(0, &prevTex);
	pDevice->GetTextureStageState(0, D3DTSS_COLOROP, &colorOp);
	pDevice->GetTextureStageState(0, D3DTSS_COLORARG1, &colorArg1);
	pDevice->GetTextureStageState(0, D3DTSS_ALPHAOP, &alphaOp);
	pDevice->GetTextureStageState(0, D3DTSS_ALPHAARG1, &alphaArg1);
	pDevice->GetTextureStageState(0, D3DTSS_ALPHAARG2, &alphaArg2);
	pDevice->GetRenderState(D3DRS_ALPHABLENDENABLE, &blendEnable);
	pDevice->GetRenderState(D3DRS_SRCBLEND, &srcBlend);
	pDevice->GetRenderState(D3DRS_DESTBLEND, &destBlend);
	pDevice->GetFVF(&fvf);

	pDevice->GetRenderState(D3DRS_ZENABLE, &zEnable);
	pDevice->GetRenderState(D3DRS_ZWRITEENABLE, &zWrite);
	pDevice->GetRenderState(D3DRS_CULLMODE, &cullMode);
	pDevice->GetRenderState(D3DRS_LIGHTING, &lighting);
	pDevice->GetRenderState(D3DRS_SCISSORTESTENABLE, &scissorEnable);
	pDevice->GetScissorRect(&prevScissorRect);
	pDevice->GetRenderState(D3DRS_SHADEMODE, &shadeMode);

	// Setup critical overlay render states to prevent occlusion
	pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
	pDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
	pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
	pDevice->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);

	bool isScissorEnabled = false;
	DWORD now = GetTickCount();
	std::vector<int> elementsToRemove;

	for (auto& [id, elem] : g_dxElements) {
		// Delta time and hover transitions calculation
		if (elem.lastUpdateFrame == 0) {
			elem.lastUpdateFrame = now;
		}
		float dt = (float)(now - elem.lastUpdateFrame) / 1000.0f;
		elem.lastUpdateFrame = now;

		// Defer actual deletion of destroying elements
		if (elem.isDestroying) {
			if (now - elem.destroyRequestedTime >= 200) {
				elementsToRemove.push_back(id);
				continue;
			}
		}

		// Calculate recursive absolute shift and alpha multiplier
		float shiftX = 0.0f, shiftY = 0.0f;
		GetElementShift(elem, shiftX, shiftY);
		float multiplier = GetElementAlphaMultiplier(elem, now);
		
		float absX = elem.x + shiftX;
		float absY = elem.y + shiftY;
		DWORD activeColor = ModulateAlpha(elem.color, multiplier);

		if (elem.anim.active) {
			DWORD elapsed = now - elem.anim.startTime;
			if (elapsed >= elem.anim.duration) {
				elem.x = elem.anim.targetX;
				elem.y = elem.anim.targetY;
				elem.width = elem.anim.targetW;
				elem.height = elem.anim.targetH;
				elem.scale = elem.anim.targetAlpha;
				elem.anim.active = false;
			} else {
				float t = (float)elapsed / (float)elem.anim.duration;
				float easedT = GetEasedValue(elem.anim.easingType, t);
				elem.x = elem.anim.startX + (elem.anim.targetX - elem.anim.startX) * easedT;
				elem.y = elem.anim.startY + (elem.anim.targetY - elem.anim.startY) * easedT;
				elem.width = elem.anim.startW + (elem.anim.targetW - elem.anim.startW) * easedT;
				elem.height = elem.anim.startH + (elem.anim.targetH - elem.anim.startH) * easedT;
				elem.scale = elem.anim.startAlpha + (elem.anim.targetAlpha - elem.anim.startAlpha) * easedT;
			}
			absX = elem.x + shiftX;
			absY = elem.y + shiftY;
			activeColor = ModulateAlpha(elem.color, elem.scale * multiplier);
		}

		bool childClipped = false;
		RECT oldScissorRect = { 0 };
		DWORD oldScissorEnable = FALSE;
		
		if (elem.parentId != -1) {
			auto itParent = g_dxElements.find(elem.parentId);
			if (itParent != g_dxElements.end() && itParent->second.type == DXElementType::ScrollContainer) {
				float pAbsX = itParent->second.x;
				float pAbsY = itParent->second.y;
				float pShiftX = 0.0f, pShiftY = 0.0f;
				GetElementShift(itParent->second, pShiftX, pShiftY);
				pAbsX += pShiftX;
				pAbsY += pShiftY;
				
				pDevice->GetRenderState(D3DRS_SCISSORTESTENABLE, &oldScissorEnable);
				pDevice->GetScissorRect(&oldScissorRect);
				
				RECT clipRect = { 
					(LONG)pAbsX, 
					(LONG)pAbsY, 
					(LONG)(pAbsX + itParent->second.width), 
					(LONG)(pAbsY + itParent->second.height) 
				};
				pDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
				pDevice->SetScissorRect(&clipRect);
				childClipped = true;
			}
		}

		if (elem.blurBehind && !elem.isDestroying && elem.width > 0.0f && elem.height > 0.0f) {
			RenderBlur(pDevice, absX, absY, elem.width, elem.height);
		}

		// Calculate hover progress for interactive widgets
		bool hovered = false;
		if (elem.type == DXElementType::Button || elem.type == DXElementType::Checkbox ||
			elem.type == DXElementType::Input || elem.type == DXElementType::Slider) {
			hovered = (g_mouseX >= absX && g_mouseX <= absX + elem.width &&
					   g_mouseY >= absY && g_mouseY <= absY + elem.height &&
					   !elem.isDestroying);
			if (hovered) {
				elem.hoverProgress += dt / 0.15f; // 150ms transition
				if (elem.hoverProgress > 1.0f) elem.hoverProgress = 1.0f;
			} else {
				elem.hoverProgress -= dt / 0.15f;
				if (elem.hoverProgress < 0.0f) elem.hoverProgress = 0.0f;
			}
		}

		if (elem.type == DXElementType::Clip) {
			if (elem.width > 0.0f && elem.height > 0.0f) {
				pDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
				RECT rect = { (LONG)absX, (LONG)absY, (LONG)(absX + elem.width), (LONG)(absY + elem.height) };
				pDevice->SetScissorRect(&rect);
				isScissorEnabled = true;
			} else {
				pDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
				isScissorEnabled = false;
			}
		}
		else if (elem.type == DXElementType::Shadow) {
			// Draw 5-pass soft shadow box
			float s = elem.shadowSize;
			float o = elem.shadowOffset;
			DWORD baseColor = elem.color;
			BYTE baseA = (BYTE)((baseColor >> 24) & 0xFF);
			BYTE r = (BYTE)((baseColor >> 16) & 0xFF);
			BYTE g = (BYTE)((baseColor >> 8) & 0xFF);
			BYTE b = (BYTE)(baseColor & 0xFF);
			
			for (int step = 1; step <= 5; ++step) {
				float inset = (float)step * (s / 5.0f);
				float sx = absX + o - inset;
				float sy = absY + o - inset;
				float sw = elem.width + inset * 2.0f;
				float sh = elem.height + inset * 2.0f;
				
				float stepProgress = 1.0f - ((float)step / 5.0f);
				float alphaMult = stepProgress * stepProgress * 0.4f;
				DWORD shadowCol = ((BYTE)(baseA * alphaMult * multiplier) << 24) | (r << 16) | (g << 8) | b;
				
				DrawDXRoundedRectangle(pDevice, sx, sy, sw, sh, elem.radius + inset, shadowCol);
			}
		}
		else if (elem.type == DXElementType::Rectangle) {
			DrawDXRectangle(pDevice, absX, absY, elem.width, elem.height, activeColor);
		}
		else if (elem.type == DXElementType::Text) {
			DrawDXText(pDevice, elem.text, absX, absY, activeColor, elem.scale, elem.font);
		}
		else if (elem.type == DXElementType::Image) {
			DrawDXImage(pDevice, absX, absY, elem.width, elem.height, activeColor, elem.text);
		}
		else if (elem.type == DXElementType::Line) {
			DrawDXLine(pDevice, absX, absY, elem.width + shiftX, elem.height + shiftY, elem.scale, activeColor);
		}
		else if (elem.type == DXElementType::Circle) {
			DrawDXCircle(pDevice, absX, absY, elem.width, activeColor, elem.scale);
		}
		else if (elem.type == DXElementType::GradientRectangle) {
			DWORD colorTR = ModulateAlpha(elem.colorTR, multiplier);
			DWORD colorBL = ModulateAlpha(elem.colorBL, multiplier);
			DWORD colorBR = ModulateAlpha(elem.colorBR, multiplier);
			DrawDXGradientRectangle(pDevice, absX, absY, elem.width, elem.height, activeColor, colorTR, colorBL, colorBR);
		}
		else if (elem.type == DXElementType::RoundedRectangle) {
			DrawDXRoundedRectangle(pDevice, absX, absY, elem.width, elem.height, elem.radius, activeColor);
		}
		else if (elem.type == DXElementType::Triangle) {
			DrawDXTriangle(pDevice, absX, absY, elem.x2 + shiftX, elem.y2 + shiftY, elem.x3 + shiftX, elem.y3 + shiftY, activeColor);
		}
		else if (elem.type == DXElementType::Slider) {
			DrawDXSlider(pDevice, absX, absY, elem.width, elem.height, activeColor, elem.scale, multiplier);
		}
		else if (elem.type == DXElementType::CircularProgress) {
			DrawDXCircularProgress(pDevice, absX, absY, elem.radius, elem.progress, activeColor, elem.scale, multiplier);
		}
		else if (elem.type == DXElementType::Graph) {
			DrawDXGraph(pDevice, absX, absY, elem.width, elem.height, activeColor, elem.graphValues, elem.radius, multiplier);
		}
		else if (elem.type == DXElementType::InventorySlot) {
			bool slotHovered = (g_mouseX >= absX && g_mouseX <= absX + elem.width &&
							   g_mouseY >= absY && g_mouseY <= absY + elem.height &&
							   !elem.isDestroying);
			if (elem.id == g_activeInventoryDragId) {
				DrawDXInventorySlot(pDevice, absX, absY, elem.width, elem.height, activeColor, "", "", 0, false, multiplier);
				float dragX = g_mouseX - elem.width / 2.0f;
				float dragY = g_mouseY - elem.height / 2.0f;
				DrawDXInventorySlot(pDevice, dragX, dragY, elem.width, elem.height, ModulateAlpha(activeColor, 0.7f), elem.text, elem.placeholder, elem.selectedIndex, false, multiplier);
			} else {
				DrawDXInventorySlot(pDevice, absX, absY, elem.width, elem.height, activeColor, elem.text, elem.placeholder, elem.selectedIndex, slotHovered, multiplier);
			}
		}
		else if (elem.type == DXElementType::TexturedProgressBar) {
			DrawDXTexturedProgressBar(pDevice, absX, absY, elem.width, elem.height, elem.placeholder, elem.fillTextureUrl, elem.progress, activeColor, multiplier);
		}
		else if (elem.type == DXElementType::RadialMenu) {
			DrawDXRadialMenu(pDevice, absX, absY, elem.radius, activeColor, elem.options, elem.radialIcons, elem.selectedIndex, multiplier);
		}
		else if (elem.type == DXElementType::ComboBox) {
			DrawDXComboBox(pDevice, absX, absY, elem.width, elem.height, activeColor, elem.selectedIndex, elem.options, elem.isDropped, elem.font, elem.scale, multiplier);
		}
		else if (elem.type == DXElementType::ListView) {
			DrawDXListView(pDevice, absX, absY, elem.width, elem.height, activeColor, elem.selectedIndex, elem.options, elem.scrollOffset, elem.font, elem.scale, elem.draggingScroll, multiplier);
		}
		else if (elem.type == DXElementType::TabPanel) {
			DrawDXTabPanel(pDevice, absX, absY, elem.width, elem.height, activeColor, elem.selectedIndex, elem.options, elem.font, elem.scale, multiplier);
		}
		else if (elem.type == DXElementType::ColorPicker) {
			DrawDXColorPicker(pDevice, absX, absY, elem.width, elem.height, elem.color, multiplier);
		}
		else if (elem.type == DXElementType::ScrollContainer) {
			DrawDXRectangle(pDevice, absX, absY, elem.width, elem.height, activeColor);
			float maxScroll = elem.contentHeight - elem.height;
			if (maxScroll > 0.0f) {
				float sbWidth = 12.0f;
				float sbX = absX + elem.width - sbWidth;
				DrawDXRectangle(pDevice, sbX, absY, sbWidth, elem.height, ModulateAlpha(0x22FFFFFF, multiplier));
				
				float visibleRatio = elem.height / elem.contentHeight;
				float thumbH = elem.height * visibleRatio;
				if (thumbH < 20.0f) thumbH = 20.0f;
				float thumbY = absY + elem.progress * (elem.height - thumbH);
				DrawDXRectangle(pDevice, sbX + 2.0f, thumbY, sbWidth - 4.0f, thumbH, ModulateAlpha(0xFF3498DB, multiplier));
			}
		}
		else if (elem.type == DXElementType::Icon) {
			std::string faName = elem.font.empty() ? "FontAwesome" : elem.font;

			if (faName == "FontAwesome") {
				static bool faDownloadTriggered = false;
				extern bool IsFontRegistered(const std::string& fontName);
				bool faRegistered = IsFontRegistered(faName);
				if (!faRegistered) {
					std::string localPath = "omp-dx\\fonts\\FontAwesome.ttf";
					std::string url = "https://cdnjs.cloudflare.com/ajax/libs/font-awesome/4.7.0/fonts/fontawesome-webfont.ttf";
					CreateDirectoryA("omp-dx", NULL);
					CreateDirectoryA("omp-dx\\fonts", NULL);
					DWORD fileAttr = GetFileAttributesA(localPath.c_str());
					bool fileExists = (fileAttr != INVALID_FILE_ATTRIBUTES && !(fileAttr & FILE_ATTRIBUTE_DIRECTORY));

					if (fileExists) {
						HANDLE hFile = CreateFileA(localPath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
						if (hFile != INVALID_HANDLE_VALUE) {
							DWORD fileSize = GetFileSize(hFile, NULL);
							CloseHandle(hFile);
							if (fileSize > 1000) {
								LoadFontLocal(faName, localPath);
								faRegistered = true;
								LogToFile("Icon: FontAwesome font registered successfully (" + std::to_string(fileSize) + " bytes)");
							} else {
								DeleteFileA(localPath.c_str());
								fileExists = false;
								LogToFile("Icon: FontAwesome.ttf invalid (" + std::to_string(fileSize) + " bytes), deleting for re-download");
							}
						}
					}

					if (!fileExists && !faDownloadTriggered) {
						faDownloadTriggered = true;
						LogToFile("Icon: Starting FontAwesome download from CDN...");
						std::thread t(DownloadAndLoadFont, faName, url, localPath);
						t.detach();
					}
				}
			}

			extern bool IsFontRegistered(const std::string& fontName);
			if (IsFontRegistered(faName)) {
				const DXFont* pFont = GetFontOrCreate(pDevice, faName);
				if (pFont) {
					int charVal = GetFontAwesomeIconChar(elem.text);
					if (charVal > 0) {
						std::string iconStr;
						iconStr.push_back((char)charVal);
						float scale = elem.width / 32.0f;
						DrawDXText(pDevice, iconStr, absX, absY, activeColor, scale, faName);
					}
				}
			}
		}
		else if (elem.type == DXElementType::Button) {
			DrawDXRectangle(pDevice, absX, absY, elem.width, elem.height, activeColor);
			if (elem.hoverProgress > 0.0f) {
				DWORD hoverOverlay = ModulateAlpha(0x20FFFFFF, elem.hoverProgress * multiplier);
				DrawDXRectangle(pDevice, absX, absY, elem.width, elem.height, hoverOverlay);
			}
			const DXFont* pFont = GetFontOrCreate(pDevice, elem.font);
			float textWidth = 0.0f;
			float charHeight = 32.0f;
			if (pFont) {
				charHeight = pFont->charHeight;
				for (char c : elem.text) {
					unsigned char uc = (unsigned char)c;
					if (uc >= 32) textWidth += pFont->charWidths[uc] * elem.scale;
				}
			}
			float textX = absX + (elem.width - textWidth) / 2.0f;
			float textY = absY + (elem.height - charHeight * elem.scale) / 2.0f;
			DWORD buttonTextColor = ModulateAlpha(0xFFFFFFFF, multiplier);
			DrawDXText(pDevice, elem.text, textX, textY, buttonTextColor, elem.scale, elem.font);
		}
		else if (elem.type == DXElementType::Checkbox) {
			float boxSize = 20.0f;
			float boxX = absX;
			float boxY = absY + (elem.height - boxSize) / 2.0f;
			DrawDXRectangle(pDevice, boxX, boxY, boxSize, boxSize, ModulateAlpha(0xFF1A1A1A, multiplier));
			DrawDXBorder(pDevice, boxX, boxY, boxSize, boxSize, 1.5f, activeColor);
			if (elem.hoverProgress > 0.0f) {
				DWORD hoverOverlay = ModulateAlpha(0x20FFFFFF, elem.hoverProgress * multiplier);
				DrawDXRectangle(pDevice, boxX, boxY, boxSize, boxSize, hoverOverlay);
			}
			if (elem.checked) {
				DrawDXRectangle(pDevice, boxX + 4.0f, boxY + 4.0f, boxSize - 8.0f, boxSize - 8.0f, activeColor);
			}
			const DXFont* pFont = GetFontOrCreate(pDevice, elem.font);
			float charHeight = 32.0f;
			if (pFont) {
				charHeight = pFont->charHeight;
			}
			float textX = boxX + boxSize + 10.0f;
			float textY = absY + (elem.height - charHeight * elem.scale) / 2.0f;
			DWORD checkboxTextColor = ModulateAlpha(0xFFFFFFFF, multiplier);
			DrawDXText(pDevice, elem.text, textX, textY, checkboxTextColor, elem.scale, elem.font);
		}
		else if (elem.type == DXElementType::Input) {
			DrawDXRectangle(pDevice, absX, absY, elem.width, elem.height, ModulateAlpha(0xFF121212, multiplier));
			bool isFocused = (g_focusedInputId == elem.id);
			DWORD borderColor = isFocused ? elem.color : 0xFF555555;
			borderColor = ModulateAlpha(borderColor, multiplier);
			DrawDXBorder(pDevice, absX, absY, elem.width, elem.height, 1.5f, borderColor);
			if (elem.hoverProgress > 0.0f && !isFocused) {
				DWORD hoverOutline = ModulateAlpha(0x55FFFFFF, elem.hoverProgress * multiplier);
				DrawDXBorder(pDevice, absX, absY, elem.width, elem.height, 1.5f, hoverOutline);
			}
			const DXFont* pFont = GetFontOrCreate(pDevice, elem.font);
			float charHeight = 32.0f;
			if (pFont) {
				charHeight = pFont->charHeight;
			}
			float textX = absX + 8.0f;
			float textY = absY + (elem.height - charHeight * elem.scale) / 2.0f;
			std::string drawText = elem.text;
			if (elem.isPassword && !elem.text.empty()) {
				drawText = std::string(elem.text.length(), '*');
			}
			if (drawText.empty() && !isFocused) {
				DrawDXText(pDevice, elem.placeholder, textX, textY, ModulateAlpha(0xFF7F8C8D, multiplier), elem.scale, elem.font);
			} else {
				DrawDXText(pDevice, drawText, textX, textY, ModulateAlpha(0xFFFFFFFF, multiplier), elem.scale, elem.font);
			}
			if (isFocused && ((GetTickCount() / 500) % 2 == 0)) {
				float textWidth = 0.0f;
				if (pFont) {
					std::string measureText = elem.isPassword ? std::string(elem.text.length(), '*') : elem.text;
					for (char c : measureText) {
						unsigned char uc = (unsigned char)c;
						if (uc >= 32) textWidth += pFont->charWidths[uc] * elem.scale;
					}
				}
				float cursorX = textX + textWidth + 2.0f;
				DrawDXRectangle(pDevice, cursorX, textY + 2.0f, 2.0f, charHeight * elem.scale - 4.0f, activeColor);
			}
		}

		if (childClipped) {
			pDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, oldScissorEnable);
			pDevice->SetScissorRect(&oldScissorRect);
		}
	}

	for (int rId : elementsToRemove) {
		g_dxElements.erase(rId);
	}

	// Secondary pass to render active ComboBox dropdown menus on top
	for (auto const& [id, elem] : g_dxElements) {
		if (elem.type == DXElementType::ComboBox && elem.isDropped) {
			float shiftX = 0.0f, shiftY = 0.0f;
			GetElementShift(elem, shiftX, shiftY);
			float multiplier = GetElementAlphaMultiplier(elem, now);
			DXElement temp = elem;
			temp.x += shiftX;
			temp.y += shiftY;
			temp.color = ModulateAlpha(elem.color, multiplier);
			DrawDXComboBoxDropdownList(pDevice, temp);
		}
	}

	// Tooltip pass
	int hoveredId = -1;
	for (auto const& [id, elem] : g_dxElements) {
		if (elem.isDestroying) continue;
		float shiftX = 0.0f, shiftY = 0.0f;
		GetElementShift(elem, shiftX, shiftY);
		float absX = elem.x + shiftX;
		float absY = elem.y + shiftY;
		if (g_mouseX >= absX && g_mouseX <= absX + elem.width &&
			g_mouseY >= absY && g_mouseY <= absY + elem.height) {
			hoveredId = id;
		}
	}
	
	static int lastHoveredId = -1;
	static DWORD hoverStartTime = 0;
	if (hoveredId != lastHoveredId) {
		lastHoveredId = hoveredId;
		hoverStartTime = now;
	}
	
	if (hoveredId != -1 && !g_dxElements[hoveredId].tooltipText.empty()) {
		if (now - hoverStartTime > 500) { // 500ms delay
			const auto& elem = g_dxElements[hoveredId];
			float scale = 0.45f;
			const DXFont* pFont = GetFontOrCreate(pDevice, elem.font);
			float textWidth = 0.0f;
			float charHeight = 32.0f;
			if (pFont) {
				charHeight = pFont->charHeight;
				for (char c : elem.tooltipText) {
					unsigned char uc = (unsigned char)c;
					if (uc >= 32) textWidth += pFont->charWidths[uc] * scale;
				}
			} else {
				textWidth = elem.tooltipText.length() * 8.0f;
			}
			
			float tx = g_mouseX + 15.0f;
			float ty = g_mouseY + 15.0f;
			float tw = textWidth + 20.0f;
			float th = charHeight * scale + 12.0f;
			
			if (tx + tw > g_screenWidth) tx = g_mouseX - tw - 5.0f;
			if (ty + th > g_screenHeight) ty = g_mouseY - th - 5.0f;
			
			DrawDXRoundedRectangle(pDevice, tx, ty, tw, th, 4.0f, 0xEE1A1A1E);
			DrawDXBorder(pDevice, tx, ty, tw, th, 1.0f, 0x44FFFFFF);
			DrawDXText(pDevice, elem.tooltipText, tx + 10.0f, ty + 6.0f, 0xFFFFFFFF, scale, elem.font);
		}
	}

	if (isScissorEnabled) {
		pDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
	}

	// Restore render states
	pDevice->SetTexture(0, prevTex);
	if (prevTex) prevTex->Release();
	pDevice->SetTextureStageState(0, D3DTSS_COLOROP, colorOp);
	pDevice->SetTextureStageState(0, D3DTSS_COLORARG1, colorArg1);
	pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, alphaOp);
	pDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, alphaArg1);
	pDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, alphaArg2);
	pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, blendEnable);
	pDevice->SetRenderState(D3DRS_SRCBLEND, srcBlend);
	pDevice->SetRenderState(D3DRS_DESTBLEND, destBlend);
	pDevice->SetFVF(fvf);

	pDevice->SetRenderState(D3DRS_ZENABLE, zEnable);
	pDevice->SetRenderState(D3DRS_ZWRITEENABLE, zWrite);
	pDevice->SetRenderState(D3DRS_CULLMODE, cullMode);
	pDevice->SetRenderState(D3DRS_LIGHTING, lighting);
	pDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, scissorEnable);
	pDevice->SetScissorRect(&prevScissorRect);
	pDevice->SetRenderState(D3DRS_SHADEMODE, shadeMode);
}

void c_plugin::everything()
{
	hWnd = FindWindowA("Grand Theft Auto San Andreas", NULL);
	GetWindowThreadProcessId(hWnd, &procID);
	handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procID);
	LogToFile("everything(): Found HWND=" + std::to_string((uintptr_t)hWnd) + ", ProcessID=" + std::to_string(procID));

	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	Gdiplus::GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, NULL);
	LogToFile("GDI+ initialized.");
}

HRESULT __stdcall hkReset(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pp)
{
	LogToFile("hkReset called. Resetting device...");
	ReleaseFontTextures();
	CleanupImageTextures();
	CleanupBlurTextures();
	g_bwasInitialized = false;
	return oReset(pDevice, pp);
}

HRESULT __stdcall hkPresent(IDirect3DDevice9* pDevice, const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion)
{
	if (!g_bwasInitialized) {
		LogToFile("hkPresent: Initializing on device 0x" + std::to_string((uintptr_t)pDevice));
		g_bwasInitialized = true;
	}

	D3DVIEWPORT9 vp;
	if (SUCCEEDED(pDevice->GetViewport(&vp))) {
		float w = (float)vp.Width;
		float h = (float)vp.Height;
		DWORD now = GetTickCount();
		if (w != g_screenWidth || h != g_screenHeight || (now - g_lastScreenSizeSendTime > 5000)) {
			g_screenWidth = w;
			g_screenHeight = h;
			g_lastScreenSizeSendTime = now;

			RakNet::BitStream bs;
			bs.Write(w);
			bs.Write(h);
			rakhook::send_rpc(191, &bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, false);
		}
	}

	pDevice->BeginScene();
	RenderDXElements(pDevice);
	pDevice->EndScene();

	return oPresent(pDevice, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
}

#include <urlmon.h>
#pragma comment(lib, "urlmon.lib")
#include <thread>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

void PlayCustomSound(const std::string& localPath) {
	mciSendStringA("close dxui_snd", NULL, 0, NULL);
	std::string openCmd = "open \"" + localPath + "\" type mpegvideo alias dxui_snd";
	MCIERROR err = mciSendStringA(openCmd.c_str(), NULL, 0, NULL);
	if (err == 0) {
		mciSendStringA("play dxui_snd from 0", NULL, 0, NULL);
		LogToFile("PlayCustomSound: Successfully playing sound: " + localPath);
	} else {
		LogToFile("PlayCustomSound: mciSendStringA FAILED (error: " + std::to_string(err) + "), falling back to PlaySoundA.");
		PlaySoundA(localPath.c_str(), NULL, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
	}
}

void DownloadAndPlaySound(std::string url, std::string localPath) {
	LogToFile("DownloadAndPlaySound: Starting background download from: " + url + " to: " + localPath);
	if (DownloadFileWinINet(url, localPath)) {
		LogToFile("DownloadAndPlaySound: Download completed successfully, playing sound.");
		PlayCustomSound(localPath);
	} else {
		LogToFile("DownloadAndPlaySound: FAILED to download sound from: " + url);
	}
}

void LoadFontLocal(const std::string& fontFamily, const std::string& localPath) {
	LogToFile("LoadFontLocal: Loading font: " + fontFamily + " from path: " + localPath);
	DWORD numFonts = AddFontResourceExA(localPath.c_str(), FR_PRIVATE, NULL);
	if (numFonts > 0) {
		LogToFile("LoadFontLocal: Successfully loaded private font resource: " + fontFamily + " (Total: " + std::to_string(numFonts) + ")");
		g_loadedFontPaths.push_back(localPath);

		std::lock_guard<std::mutex> lock(g_fontMutex);
		auto it = g_dxFonts.find(fontFamily);
		if (it != g_dxFonts.end()) {
			if (it->second.texture) {
				it->second.texture->Release();
			}
			g_dxFonts.erase(it);
			LogToFile("LoadFontLocal: Cleared pre-existing/fallback texture for font: " + fontFamily);
		}
	} else {
		LogToFile("LoadFontLocal: FAILED to load font resource: " + fontFamily);
	}
}

void DownloadAndLoadFont(std::string fontFamily, std::string url, std::string localPath) {
	LogToFile("DownloadAndLoadFont: Starting background download from: " + url + " to: " + localPath);
	if (DownloadFileWinINet(url, localPath)) {
		LogToFile("DownloadAndLoadFont: Download completed successfully for font: " + fontFamily);
		LoadFontLocal(fontFamily, localPath);
	} else {
		LogToFile("DownloadAndLoadFont: FAILED to download font: " + fontFamily);
	}
}

void c_plugin::game_loop()
{
	static bool initialized = false;

	if (initialized) {
		return game_loop_hook.call_original();
	}

	if (!rakhook::initialize()) {
		return game_loop_hook.call_original();
	}

	if (samp::RefChat() == nullptr) {
		return game_loop_hook.call_original();
	}

	initialized = true;
	StringCompressor::AddReference();

	LogToFile("c_plugin::game_loop(): Plugin initialized successfully!");
	samp::RefChat()->AddMessage(-1, "ASI-Plugin | Custom DX Renderer Active");

	everything();

	void** vTableDevice = *(void***)(*(DWORD*)DEVICE_PTR);
	VTableHookManager* vmtHooks = new VTableHookManager(vTableDevice, D3D_VFUNCTIONS);

	oPresent = (_Present)vmtHooks->Hook(PRESENT_INDEX, (void*)hkPresent);
	oReset = (_Reset)vmtHooks->Hook(RESET_INDEX, (void*)hkReset);

	LogToFile("vTable hooks installed. Present index: 17, Reset index: 16.");

	if (hWnd) {
		oWndProc = (WNDPROC)SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)hkWndProc);
		LogToFile("WndProc hooked successfully.");
	}

	rakhook::on_receive_rpc += [](unsigned char& id, RakNet::BitStream* bs) -> bool {
		if (id == 190) {
			int numBytes = bs->GetNumberOfBytesUsed();
			int numBits = bs->GetNumberOfBitsUsed();
			unsigned char* pData = bs->GetData();
			std::string hexStr;
			for (int i = 0; i < numBytes; i++) {
				char tmp[8];
				sprintf_s(tmp, "%02X ", pData[i]);
				hexStr += tmp;
			}
			LogToFile("on_receive_rpc 190: Bits=" + std::to_string(numBits) + ", Bytes=" + std::to_string(numBytes) + ", Hex: " + hexStr);

			uint8_t subtype = 0;
			if (!bs->Read(subtype)) {
				LogToFile("rakhook::on_receive_rpc: Failed to read subtype!");
				return false;
			}

			std::lock_guard<std::mutex> lock(g_dxMutex);

			int32_t tempElementId = -1;
			if (subtype != 4 && subtype != 8) {
				int originalReadOffset = bs->GetReadOffset();
				bs->Read(tempElementId);
				bs->SetReadOffset(originalReadOffset);
			}

			bool hasOldElement = false;
			bool savedDraggable = false;
			bool savedIsDragging = false;
			float savedDragOffsetX = 0.0f;
			float savedDragOffsetY = 0.0f;
			bool savedIsDropped = false;
			int savedScrollOffset = 0;
			bool savedDraggingScroll = false;
			int savedParentId = -1;
			DWORD savedCreationTime = 0;
			bool savedIsDestroying = false;
			DWORD savedDestroyRequestedTime = 0;
			std::string savedTooltipText = "";
			float savedShadowSize = 0.0f;
			float savedShadowOffset = 0.0f;
			bool savedIsPassword = false;
			float savedProgress = 0.0f;
			bool savedBlurBehind = false;
			float savedContentHeight = 0.0f;
			std::vector<float> savedGraphValues;
			std::vector<std::string> savedRadialIcons;

			bool restoreCoords = false;
			float savedX = 0.0f, savedY = 0.0f;
			float savedX2 = 0.0f, savedY2 = 0.0f;
			float savedX3 = 0.0f, savedY3 = 0.0f;

			bool restoreSliderScale = false;
			float savedSliderScale = 0.0f;

			if (tempElementId != -1) {
				auto it = g_dxElements.find(tempElementId);
				if (it != g_dxElements.end()) {
					hasOldElement = true;
					savedDraggable = it->second.draggable;
					savedIsDragging = it->second.isDragging;
					savedDragOffsetX = it->second.dragOffsetX;
					savedDragOffsetY = it->second.dragOffsetY;
					savedIsDropped = it->second.isDropped;
					savedScrollOffset = it->second.scrollOffset;
					savedDraggingScroll = it->second.draggingScroll;
					savedParentId = it->second.parentId;
					savedCreationTime = it->second.creationTime;
					savedIsDestroying = it->second.isDestroying;
					savedDestroyRequestedTime = it->second.destroyRequestedTime;
					savedTooltipText = it->second.tooltipText;
					savedShadowSize = it->second.shadowSize;
					savedShadowOffset = it->second.shadowOffset;
					savedIsPassword = it->second.isPassword;
					savedProgress = it->second.progress;
					savedBlurBehind = it->second.blurBehind;
					savedContentHeight = it->second.contentHeight;
					savedGraphValues = it->second.graphValues;
					savedRadialIcons = it->second.radialIcons;

					if (g_activeDragElementId != -1 && 
						(tempElementId == g_activeDragElementId || (tempElementId > g_activeDragElementId && tempElementId <= g_activeDragElementId + 100))) {
						restoreCoords = true;
						savedX = it->second.x;
						savedY = it->second.y;
						savedX2 = it->second.x2;
						savedY2 = it->second.y2;
						savedX3 = it->second.x3;
						savedY3 = it->second.y3;
					}

					if (g_activeSliderDragId != -1 && tempElementId == g_activeSliderDragId) {
						restoreSliderScale = true;
						savedSliderScale = it->second.scale;
					}
				}
			}

			if (subtype == 1) { // Create/Update Rectangle
				int32_t elementId;
				float x, y, width, height;
				uint32_t color;
				if (bs->Read(elementId) && bs->Read(x) && bs->Read(y) && bs->Read(width) && bs->Read(height) && bs->Read(color)) {
					DXElement elem;
					elem.id = elementId;
					elem.type = DXElementType::Rectangle;
					elem.x = x;
					elem.y = y;
					elem.width = width;
					elem.height = height;
					elem.color = color;
					g_dxElements[elementId] = elem;
					LogToFile("RPC Subtype 1 (Rectangle): ID=" + std::to_string(elementId) + 
						", X=" + std::to_string(x) + ", Y=" + std::to_string(y) + 
						", W=" + std::to_string(width) + ", H=" + std::to_string(height) + 
						", Color=0x" + std::to_string(color));
				} else {
					LogToFile("RPC Subtype 1 (Rectangle): Failed to read parameters!");
				}
			}
			else if (subtype == 2) { // Create/Update Text
				int32_t elementId;
				float x, y, scale;
				uint32_t color;
				if (bs->Read(elementId) && bs->Read(x) && bs->Read(y) && bs->Read(color) && bs->Read(scale)) {
					uint16_t len = 0;
					if (bs->Read(len)) {
						std::string text;
						if (len > 0) {
							text.resize(len);
							bs->Read(&text[0], len);
						}
						std::string font;
						uint16_t fontLen = 0;
						if (bs->Read(fontLen)) {
							if (fontLen > 0) {
								font.resize(fontLen);
								bs->Read(&font[0], fontLen);
							}
						}
						DXElement elem;
						elem.id = elementId;
						elem.type = DXElementType::Text;
						elem.x = x;
						elem.y = y;
						elem.color = color;
						elem.scale = scale;
						elem.text = text;
						elem.font = font;
						g_dxElements[elementId] = elem;
						LogToFile("RPC Subtype 2 (Text): ID=" + std::to_string(elementId) + 
							", X=" + std::to_string(x) + ", Y=" + std::to_string(y) + 
							", Scale=" + std::to_string(scale) + ", Text='" + text + "', Font='" + font + "'");
					} else {
						LogToFile("RPC Subtype 2 (Text): Failed to read text length!");
					}
				} else {
					LogToFile("RPC Subtype 2 (Text): Failed to read parameters!");
				}
			}
			else if (subtype == 3) { // Destroy
				int32_t elementId;
				if (bs->Read(elementId)) {
					auto it = g_dxElements.find(elementId);
					if (it != g_dxElements.end() && !it->second.isDestroying) {
						it->second.isDestroying = true;
						it->second.destroyRequestedTime = GetTickCount();
						savedIsDestroying = true;
						savedDestroyRequestedTime = it->second.destroyRequestedTime;
					} else {
						g_dxElements.erase(elementId);
					}
					LogToFile("RPC Subtype 3 (Destroy): ID=" + std::to_string(elementId));
				} else {
					LogToFile("RPC Subtype 3 (Destroy): Failed to read ID!");
				}
			}
			else if (subtype == 4) { // ClearAll
				g_dxElements.clear();
				LogToFile("RPC Subtype 4 (ClearAll)");
			}
			else if (subtype == 5) { // Create/Update Button
				int32_t elementId;
				float x, y, width, height, scale;
				uint32_t color;
				if (bs->Read(elementId) && bs->Read(x) && bs->Read(y) && bs->Read(width) && bs->Read(height) && bs->Read(color) && bs->Read(scale)) {
					uint16_t len = 0;
					if (bs->Read(len)) {
						std::string text;
						if (len > 0) {
							text.resize(len);
							bs->Read(&text[0], len);
						}
						std::string font;
						uint16_t fontLen = 0;
						if (bs->Read(fontLen)) {
							if (fontLen > 0) {
								font.resize(fontLen);
								bs->Read(&font[0], fontLen);
							}
						}
						DXElement elem;
						elem.id = elementId;
						elem.type = DXElementType::Button;
						elem.x = x;
						elem.y = y;
						elem.width = width;
						elem.height = height;
						elem.color = color;
						elem.scale = scale;
						elem.text = text;
						elem.font = font;
						g_dxElements[elementId] = elem;
						LogToFile("RPC Subtype 5 (Button): ID=" + std::to_string(elementId) + 
							", X=" + std::to_string(x) + ", Y=" + std::to_string(y) + 
							", W=" + std::to_string(width) + ", H=" + std::to_string(height) + 
							", Scale=" + std::to_string(scale) + ", Text='" + text + "', Font='" + font + "'");
					}
				}
			}
			else if (subtype == 6) { // Create/Update Checkbox
				int32_t elementId;
				float x, y, width, height, scale;
				uint32_t color;
				bool checked;
				if (bs->Read(elementId) && bs->Read(x) && bs->Read(y) && bs->Read(width) && bs->Read(height) && bs->Read(color) && bs->Read(checked) && bs->Read(scale)) {
					uint16_t len = 0;
					if (bs->Read(len)) {
						std::string text;
						if (len > 0) {
							text.resize(len);
							bs->Read(&text[0], len);
						}
						std::string font;
						uint16_t fontLen = 0;
						if (bs->Read(fontLen)) {
							if (fontLen > 0) {
								font.resize(fontLen);
								bs->Read(&font[0], fontLen);
							}
						}
						DXElement elem;
						elem.id = elementId;
						elem.type = DXElementType::Checkbox;
						elem.x = x;
						elem.y = y;
						elem.width = width;
						elem.height = height;
						elem.color = color;
						elem.checked = checked;
						elem.scale = scale;
						elem.text = text;
						elem.font = font;
						g_dxElements[elementId] = elem;
						LogToFile("RPC Subtype 6 (Checkbox): ID=" + std::to_string(elementId) + 
							", X=" + std::to_string(x) + ", Y=" + std::to_string(y) + 
							", W=" + std::to_string(width) + ", H=" + std::to_string(height) + 
							", Checked=" + std::to_string(checked) + ", Scale=" + std::to_string(scale) + ", Label='" + text + "', Font='" + font + "'");
					}
				}
			}
			else if (subtype == 7) { // Create/Update Input
				int32_t elementId;
				float x, y, width, height, scale;
				uint32_t color;
				if (bs->Read(elementId) && bs->Read(x) && bs->Read(y) && bs->Read(width) && bs->Read(height) && bs->Read(color) && bs->Read(scale)) {
					uint16_t textLen = 0;
					std::string text;
					if (bs->Read(textLen)) {
						if (textLen > 0) {
							text.resize(textLen);
							bs->Read(&text[0], textLen);
						}
					}
					uint16_t phLen = 0;
					std::string placeholder;
					if (bs->Read(phLen)) {
						if (phLen > 0) {
							placeholder.resize(phLen);
							bs->Read(&placeholder[0], phLen);
						}
					}
					std::string font;
					uint16_t fontLen = 0;
					if (bs->Read(fontLen)) {
						if (fontLen > 0) {
							font.resize(fontLen);
							bs->Read(&font[0], fontLen);
						}
					}
					DXElement elem;
					elem.id = elementId;
					elem.type = DXElementType::Input;
					elem.x = x;
					elem.y = y;
					elem.width = width;
					elem.height = height;
					elem.color = color;
					elem.scale = scale;
					elem.text = text;
					elem.placeholder = placeholder;
					elem.font = font;
					g_dxElements[elementId] = elem;
					LogToFile("RPC Subtype 7 (Input): ID=" + std::to_string(elementId) + 
						", X=" + std::to_string(x) + ", Y=" + std::to_string(y) + 
						", W=" + std::to_string(width) + ", H=" + std::to_string(height) + 
						", Text='" + text + "', Placeholder='" + placeholder + "', Font='" + font + "'");
				}
			}
			else if (subtype == 8) { // Load Font
				uint16_t familyLen = 0;
				std::string fontFamily;
				if (bs->Read(familyLen)) {
					if (familyLen > 0) {
						fontFamily.resize(familyLen);
						bs->Read(&fontFamily[0], familyLen);
					}
				}
				uint16_t urlLen = 0;
				std::string url;
				if (bs->Read(urlLen)) {
					if (urlLen > 0) {
						url.resize(urlLen);
						bs->Read(&url[0], urlLen);
					}
				}
				if (!fontFamily.empty() && !url.empty()) {
					CreateDirectoryA("omp-dx", NULL);
					CreateDirectoryA("omp-dx\\fonts", NULL);
					std::string localPath = "omp-dx\\fonts\\" + fontFamily + ".ttf";
					DWORD fileAttr = GetFileAttributesA(localPath.c_str());
					if (fileAttr != INVALID_FILE_ATTRIBUTES && !(fileAttr & FILE_ATTRIBUTE_DIRECTORY)) {
						LogToFile("RPC Subtype 8 (Load Font): Local file already exists: " + localPath);
						LoadFontLocal(fontFamily, localPath);
					} else {
						LogToFile("RPC Subtype 8 (Load Font): Starting async download for: " + fontFamily + " from: " + url);
						std::thread downloadThread(DownloadAndLoadFont, fontFamily, url, localPath);
						downloadThread.detach();
					}
				}
			}
			else if (subtype == 9) { // Create/Update Image
				int32_t elementId;
				float x, y, width, height;
				uint32_t color;
				if (bs->Read(elementId) && bs->Read(x) && bs->Read(y) && bs->Read(width) && bs->Read(height) && bs->Read(color)) {
					uint16_t urlLen = 0;
					std::string url;
					if (bs->Read(urlLen)) {
						if (urlLen > 0) {
							url.resize(urlLen);
							bs->Read(&url[0], urlLen);
						}
					}
					DXElement elem;
					elem.id = elementId;
					elem.type = DXElementType::Image;
					elem.x = x;
					elem.y = y;
					elem.width = width;
					elem.height = height;
					elem.color = color;
					elem.text = url;
					g_dxElements[elementId] = elem;
					LogToFile("RPC Subtype 9 (Image): ID=" + std::to_string(elementId) +
						", X=" + std::to_string(x) + ", Y=" + std::to_string(y) +
						", W=" + std::to_string(width) + ", H=" + std::to_string(height) +
						", Color=0x" + std::to_string(color) + ", URL='" + url + "'");
				}
			}
			else if (subtype == 10) { // Create/Update Line
				int32_t elementId;
				float x1, y1, x2, y2, thickness;
				uint32_t color;
				if (bs->Read(elementId) && bs->Read(x1) && bs->Read(y1) && bs->Read(x2) && bs->Read(y2) && bs->Read(thickness) && bs->Read(color)) {
					DXElement elem;
					elem.id = elementId;
					elem.type = DXElementType::Line;
					elem.x = x1;
					elem.y = y1;
					elem.width = x2;
					elem.height = y2;
					elem.scale = thickness;
					elem.color = color;
					g_dxElements[elementId] = elem;
					LogToFile("RPC Subtype 10 (Line): ID=" + std::to_string(elementId) +
						", X1=" + std::to_string(x1) + ", Y1=" + std::to_string(y1) +
						", X2=" + std::to_string(x2) + ", Y2=" + std::to_string(y2) +
						", Thickness=" + std::to_string(thickness) + ", Color=0x" + std::to_string(color));
				}
			}
			else if (subtype == 11) { // Create/Update Circle
				int32_t elementId;
				float x, y, radius, thickness;
				uint32_t color;
				if (bs->Read(elementId) && bs->Read(x) && bs->Read(y) && bs->Read(radius) && bs->Read(color) && bs->Read(thickness)) {
					DXElement elem;
					elem.id = elementId;
					elem.type = DXElementType::Circle;
					elem.x = x;
					elem.y = y;
					elem.width = radius;
					elem.scale = thickness;
					elem.color = color;
					g_dxElements[elementId] = elem;
					LogToFile("RPC Subtype 11 (Circle): ID=" + std::to_string(elementId) +
						", X=" + std::to_string(x) + ", Y=" + std::to_string(y) +
						", Radius=" + std::to_string(radius) + ", Color=0x" + std::to_string(color) +
						", Thickness=" + std::to_string(thickness));
				}
			}
			else if (subtype == 12) { // Create/Update Clip
				int32_t elementId;
				float x, y, width, height;
				if (bs->Read(elementId) && bs->Read(x) && bs->Read(y) && bs->Read(width) && bs->Read(height)) {
					DXElement elem;
					elem.id = elementId;
					elem.type = DXElementType::Clip;
					elem.x = x;
					elem.y = y;
					elem.width = width;
					elem.height = height;
					g_dxElements[elementId] = elem;
					LogToFile("RPC Subtype 12 (Clip): ID=" + std::to_string(elementId) +
						", X=" + std::to_string(x) + ", Y=" + std::to_string(y) +
						", W=" + std::to_string(width) + ", H=" + std::to_string(height));
				}
			}
			else if (subtype == 13) { // Create/Update Gradient Rectangle
				int32_t elementId;
				float x, y, width, height;
				uint32_t colorTL, colorTR, colorBL, colorBR;
				if (bs->Read(elementId) && bs->Read(x) && bs->Read(y) && bs->Read(width) && bs->Read(height) &&
					bs->Read(colorTL) && bs->Read(colorTR) && bs->Read(colorBL) && bs->Read(colorBR)) {
					DXElement elem;
					elem.id = elementId;
					elem.type = DXElementType::GradientRectangle;
					elem.x = x;
					elem.y = y;
					elem.width = width;
					elem.height = height;
					elem.color = colorTL;
					elem.colorTR = colorTR;
					elem.colorBL = colorBL;
					elem.colorBR = colorBR;
					g_dxElements[elementId] = elem;
					LogToFile("RPC Subtype 13 (GradientRectangle): ID=" + std::to_string(elementId) +
						", X=" + std::to_string(x) + ", Y=" + std::to_string(y) +
						", W=" + std::to_string(width) + ", H=" + std::to_string(height));
				}
			}
			else if (subtype == 14) { // Create/Update Rounded Rectangle
				int32_t elementId;
				float x, y, width, height, radius;
				uint32_t color;
				if (bs->Read(elementId) && bs->Read(x) && bs->Read(y) && bs->Read(width) && bs->Read(height) &&
					bs->Read(radius) && bs->Read(color)) {
					DXElement elem;
					elem.id = elementId;
					elem.type = DXElementType::RoundedRectangle;
					elem.x = x;
					elem.y = y;
					elem.width = width;
					elem.height = height;
					elem.radius = radius;
					elem.color = color;
					g_dxElements[elementId] = elem;
					LogToFile("RPC Subtype 14 (RoundedRectangle): ID=" + std::to_string(elementId) +
						", X=" + std::to_string(x) + ", Y=" + std::to_string(y) +
						", W=" + std::to_string(width) + ", H=" + std::to_string(height) +
						", Radius=" + std::to_string(radius));
				}
			}
			else if (subtype == 15) { // Create/Update Triangle
				int32_t elementId;
				float x1, y1, x2, y2, x3, y3;
				uint32_t color;
				if (bs->Read(elementId) && bs->Read(x1) && bs->Read(y1) && bs->Read(x2) && bs->Read(y2) &&
					bs->Read(x3) && bs->Read(y3) && bs->Read(color)) {
					DXElement elem;
					elem.id = elementId;
					elem.type = DXElementType::Triangle;
					elem.x = x1;
					elem.y = y1;
					elem.x2 = x2;
					elem.y2 = y2;
					elem.x3 = x3;
					elem.y3 = y3;
					elem.color = color;
					g_dxElements[elementId] = elem;
					LogToFile("RPC Subtype 15 (Triangle): ID=" + std::to_string(elementId) +
						", X1=" + std::to_string(x1) + ", Y1=" + std::to_string(y1) +
						", X2=" + std::to_string(x2) + ", Y2=" + std::to_string(y2) +
						", X3=" + std::to_string(x3) + ", Y3=" + std::to_string(y3));
				}
			}
			else if (subtype == 16) { // Create/Update Slider
				int32_t elementId;
				float x, y, width, height;
				uint32_t color;
				float value;
				if (bs->Read(elementId) && bs->Read(x) && bs->Read(y) && bs->Read(width) && bs->Read(height) &&
					bs->Read(color) && bs->Read(value)) {
					std::string font;
					uint16_t fontLen = 0;
					if (bs->Read(fontLen) && fontLen > 0) {
						font.resize(fontLen);
						bs->Read(&font[0], fontLen);
					}
					DXElement elem;
					elem.id = elementId;
					elem.type = DXElementType::Slider;
					elem.x = x;
					elem.y = y;
					elem.width = width;
					elem.height = height;
					elem.color = color;
					elem.scale = value; // scale field stores slider value (0.0 - 1.0)
					elem.font = font;
					g_dxElements[elementId] = elem;
					LogToFile("RPC Subtype 16 (Slider): ID=" + std::to_string(elementId) +
						", X=" + std::to_string(x) + ", Y=" + std::to_string(y) +
						", W=" + std::to_string(width) + ", H=" + std::to_string(height) +
						", Value=" + std::to_string(value));
				}
			}
			else if (subtype == 17) { // Create/Update ComboBox
				int32_t elementId;
				float x, y, width, height;
				uint32_t color;
				int32_t selectedIndex;
				if (bs->Read(elementId) && bs->Read(x) && bs->Read(y) && bs->Read(width) && bs->Read(height) &&
					bs->Read(color) && bs->Read(selectedIndex)) {
					std::string optionsStr;
					uint16_t optLen = 0;
					if (bs->Read(optLen) && optLen > 0) {
						optionsStr.resize(optLen);
						bs->Read(&optionsStr[0], optLen);
					}
					std::string font;
					uint16_t fontLen = 0;
					if (bs->Read(fontLen) && fontLen > 0) {
						font.resize(fontLen);
						bs->Read(&font[0], fontLen);
					}
					DXElement elem;
					elem.id = elementId;
					elem.type = DXElementType::ComboBox;
					elem.x = x;
					elem.y = y;
					elem.width = width;
					elem.height = height;
					elem.color = color;
					elem.selectedIndex = selectedIndex;
					elem.options = SplitString(optionsStr, ';');
					elem.font = font;
					elem.isDropped = false;
					g_dxElements[elementId] = elem;
					LogToFile("RPC Subtype 17 (ComboBox): ID=" + std::to_string(elementId) +
						", Options='" + optionsStr + "', SelectedIndex=" + std::to_string(selectedIndex));
				}
			}
			else if (subtype == 18) { // Create/Update ListView
				int32_t elementId;
				float x, y, width, height;
				uint32_t color;
				int32_t selectedIndex;
				if (bs->Read(elementId) && bs->Read(x) && bs->Read(y) && bs->Read(width) && bs->Read(height) &&
					bs->Read(color) && bs->Read(selectedIndex)) {
					std::string itemsStr;
					uint16_t itemsLen = 0;
					if (bs->Read(itemsLen) && itemsLen > 0) {
						itemsStr.resize(itemsLen);
						bs->Read(&itemsStr[0], itemsLen);
					}
					std::string font;
					uint16_t fontLen = 0;
					if (bs->Read(fontLen) && fontLen > 0) {
						font.resize(fontLen);
						bs->Read(&font[0], fontLen);
					}
					DXElement elem;
					elem.id = elementId;
					elem.type = DXElementType::ListView;
					elem.x = x;
					elem.y = y;
					elem.width = width;
					elem.height = height;
					elem.color = color;
					elem.selectedIndex = selectedIndex;
					elem.options = SplitString(itemsStr, ';');
					elem.font = font;
					elem.scrollOffset = 0;
					g_dxElements[elementId] = elem;
					LogToFile("RPC Subtype 18 (ListView): ID=" + std::to_string(elementId) +
						", Items='" + itemsStr + "', SelectedIndex=" + std::to_string(selectedIndex));
				}
			}
			else if (subtype == 19) { // Create/Update TabPanel
				int32_t elementId;
				float x, y, width, height;
				uint32_t color;
				int32_t selectedIndex;
				if (bs->Read(elementId) && bs->Read(x) && bs->Read(y) && bs->Read(width) && bs->Read(height) &&
					bs->Read(color) && bs->Read(selectedIndex)) {
					std::string tabsStr;
					uint16_t tabsLen = 0;
					if (bs->Read(tabsLen) && tabsLen > 0) {
						tabsStr.resize(tabsLen);
						bs->Read(&tabsStr[0], tabsLen);
					}
					std::string font;
					uint16_t fontLen = 0;
					if (bs->Read(fontLen) && fontLen > 0) {
						font.resize(fontLen);
						bs->Read(&font[0], fontLen);
					}
					DXElement elem;
					elem.id = elementId;
					elem.type = DXElementType::TabPanel;
					elem.x = x;
					elem.y = y;
					elem.width = width;
					elem.height = height;
					elem.color = color;
					elem.selectedIndex = selectedIndex;
					elem.options = SplitString(tabsStr, ';');
					elem.font = font;
					g_dxElements[elementId] = elem;
					LogToFile("RPC Subtype 19 (TabPanel): ID=" + std::to_string(elementId) +
						", Tabs='" + tabsStr + "', SelectedIndex=" + std::to_string(selectedIndex));
				}
			}
			else if (subtype == 20) { // SetDraggable
				int32_t elementId;
				bool draggable;
				if (bs->Read(elementId) && bs->Read(draggable)) {
					auto it = g_dxElements.find(elementId);
					if (it != g_dxElements.end()) {
						it->second.draggable = draggable;
						savedDraggable = draggable;
						LogToFile("RPC Subtype 20 (SetDraggable): ID=" + std::to_string(elementId) + ", Draggable=" + std::to_string(draggable));
					}
				}
			}
			else if (subtype == 21) { // SetParent
				int32_t elementId;
				int32_t parentId;
				if (bs->Read(elementId) && bs->Read(parentId)) {
					auto it = g_dxElements.find(elementId);
					if (it != g_dxElements.end()) {
						it->second.parentId = parentId;
						savedParentId = parentId;
						LogToFile("RPC Subtype 21 (SetParent): ID=" + std::to_string(elementId) + ", ParentID=" + std::to_string(parentId));
					}
				}
			}
			else if (subtype == 22) { // DrawShadow
				int32_t elementId;
				float x, y, width, height;
				uint32_t color;
				float shadowSize, shadowOffset;
				if (bs->Read(elementId) && bs->Read(x) && bs->Read(y) && bs->Read(width) && bs->Read(height) &&
					bs->Read(color) && bs->Read(shadowSize) && bs->Read(shadowOffset)) {
					
					DXElement elem;
					elem.id = elementId;
					elem.type = DXElementType::Shadow;
					elem.x = x;
					elem.y = y;
					elem.width = width;
					elem.height = height;
					elem.color = color;
					elem.shadowSize = shadowSize;
					elem.shadowOffset = shadowOffset;
					
					if (hasOldElement) {
						auto it = g_dxElements.find(elementId);
						if (it != g_dxElements.end()) {
							elem.radius = it->second.radius;
						}
					}
					
					g_dxElements[elementId] = elem;
					LogToFile("RPC Subtype 22 (DrawShadow): ID=" + std::to_string(elementId) + 
						", Size=" + std::to_string(shadowSize) + ", Offset=" + std::to_string(shadowOffset));
				}
			}
			else if (subtype == 23) { // SetTooltip
				int32_t elementId;
				uint16_t textLen = 0;
				if (bs->Read(elementId) && bs->Read(textLen)) {
					std::string tooltip;
					if (textLen > 0) {
						tooltip.resize(textLen);
						bs->Read(&tooltip[0], textLen);
					}
					auto it = g_dxElements.find(elementId);
					if (it != g_dxElements.end()) {
						it->second.tooltipText = tooltip;
						savedTooltipText = tooltip;
						LogToFile("RPC Subtype 23 (SetTooltip): ID=" + std::to_string(elementId) + ", Text='" + tooltip + "'");
					}
				}
			}
			else if (subtype == 24) { // DrawCircularProgress
				int32_t elementId;
				float x, y, radius, progress, thickness;
				uint32_t color;
				if (bs->Read(elementId) && bs->Read(x) && bs->Read(y) && bs->Read(radius) && bs->Read(progress) && bs->Read(color) && bs->Read(thickness)) {
					DXElement elem;
					elem.id = elementId;
					elem.type = DXElementType::CircularProgress;
					elem.x = x;
					elem.y = y;
					elem.radius = radius;
					elem.progress = progress;
					elem.color = color;
					elem.scale = thickness;
					
					if (hasOldElement) {
						auto it = g_dxElements.find(elementId);
						if (it != g_dxElements.end()) {
							elem.isPassword = it->second.isPassword;
						}
					}
					g_dxElements[elementId] = elem;
					LogToFile("RPC Subtype 24 (DrawCircularProgress): ID=" + std::to_string(elementId) + 
						", X=" + std::to_string(x) + ", Y=" + std::to_string(y) + 
						", Radius=" + std::to_string(radius) + ", Progress=" + std::to_string(progress));
				}
			}
			else if (subtype == 25) { // SetInputPassword
				int32_t elementId;
				bool enable;
				if (bs->Read(elementId) && bs->Read(enable)) {
					auto it = g_dxElements.find(elementId);
					if (it != g_dxElements.end()) {
						it->second.isPassword = enable;
						savedIsPassword = enable;
						LogToFile("RPC Subtype 25 (SetInputPassword): ID=" + std::to_string(elementId) + ", Enable=" + std::to_string(enable));
					}
				}
			}
			else if (subtype == 26) { // Animate
				int32_t elementId;
				float targetX, targetY, targetW, targetH, targetAlpha;
				int32_t durationMs;
				uint8_t easingType;
				if (bs->Read(elementId) && bs->Read(targetX) && bs->Read(targetY) && 
					bs->Read(targetW) && bs->Read(targetH) && bs->Read(targetAlpha) && 
					bs->Read(durationMs) && bs->Read(easingType)) {
					auto it = g_dxElements.find(elementId);
					if (it != g_dxElements.end()) {
						auto& elem = it->second;
						elem.anim.startX = elem.x;
						elem.anim.startY = elem.y;
						elem.anim.startW = elem.width;
						elem.anim.startH = elem.height;
						elem.anim.startAlpha = elem.scale;
						
						elem.anim.targetX = targetX;
						elem.anim.targetY = targetY;
						elem.anim.targetW = targetW;
						elem.anim.targetH = targetH;
						elem.anim.targetAlpha = targetAlpha;
						
						elem.anim.startTime = GetTickCount();
						elem.anim.duration = durationMs;
						elem.anim.easingType = easingType;
						elem.anim.active = true;
						
						LogToFile("RPC Subtype 26 (Animate): ID=" + std::to_string(elementId) + 
							", Duration=" + std::to_string(durationMs) + ", Easing=" + std::to_string(easingType));
					}
				}
			}
			else if (subtype == 27) { // DrawGraph
				int32_t elementId;
				float x, y, w, h;
				uint32_t color;
				int32_t numValues;
				float maxVal;
				if (bs->Read(elementId) && bs->Read(x) && bs->Read(y) && bs->Read(w) && bs->Read(h) &&
					bs->Read(color) && bs->Read(numValues) && bs->Read(maxVal)) {
					std::vector<float> values;
					for (int i = 0; i < numValues; i++) {
						float val = 0.0f;
						bs->Read(val);
						values.push_back(val);
					}
					DXElement elem;
					elem.id = elementId;
					elem.type = DXElementType::Graph;
					elem.x = x;
					elem.y = y;
					elem.width = w;
					elem.height = h;
					elem.color = color;
					elem.radius = maxVal;
					elem.graphValues = values;
					g_dxElements[elementId] = elem;
					LogToFile("RPC Subtype 27 (DrawGraph): ID=" + std::to_string(elementId) + ", NumValues=" + std::to_string(numValues));
				}
			}
			else if (subtype == 28) { // DrawInventorySlot
				int32_t elementId;
				float x, y, w, h;
				uint32_t color;
				int32_t amount;
				if (bs->Read(elementId) && bs->Read(x) && bs->Read(y) && bs->Read(w) && bs->Read(h) &&
					bs->Read(color) && bs->Read(amount)) {
					std::string iconUrl, label;
					uint16_t iconLen = 0, labelLen = 0;
					if (bs->Read(iconLen) && iconLen > 0) {
						iconUrl.resize(iconLen);
						bs->Read(&iconUrl[0], iconLen);
					}
					if (bs->Read(labelLen) && labelLen > 0) {
						label.resize(labelLen);
						bs->Read(&label[0], labelLen);
					}
					DXElement elem;
					elem.id = elementId;
					elem.type = DXElementType::InventorySlot;
					elem.x = x;
					elem.y = y;
					elem.width = w;
					elem.height = h;
					elem.color = color;
					elem.text = iconUrl;
					elem.placeholder = label;
					elem.selectedIndex = amount;
					g_dxElements[elementId] = elem;
					LogToFile("RPC Subtype 28 (DrawInventorySlot): ID=" + std::to_string(elementId) + ", Label='" + label + "'");
				}
			}
			else if (subtype == 29) { // DrawTexturedProgressBar
				int32_t elementId;
				float x, y, w, h;
				std::string bgUrl, fillUrl;
				float progress;
				uint32_t color;
				if (bs->Read(elementId) && bs->Read(x) && bs->Read(y) && bs->Read(w) && bs->Read(h) &&
					bs->Read(progress) && bs->Read(color)) {
					uint16_t bgLen = 0, fillLen = 0;
					if (bs->Read(bgLen) && bgLen > 0) {
						bgUrl.resize(bgLen);
						bs->Read(&bgUrl[0], bgLen);
					}
					if (bs->Read(fillLen) && fillLen > 0) {
						fillUrl.resize(fillLen);
						bs->Read(&fillUrl[0], fillLen);
					}
					DXElement elem;
					elem.id = elementId;
					elem.type = DXElementType::TexturedProgressBar;
					elem.x = x;
					elem.y = y;
					elem.width = w;
					elem.height = h;
					elem.placeholder = bgUrl;
					elem.fillTextureUrl = fillUrl;
					elem.progress = progress;
					elem.color = color;
					g_dxElements[elementId] = elem;
					LogToFile("RPC Subtype 29 (DrawTexturedProgressBar): ID=" + std::to_string(elementId));
				}
			}
			else if (subtype == 30) { // DrawRadialMenu
				int32_t elementId;
				float x, y, radius;
				uint32_t color;
				int32_t selectedIndex;
				if (bs->Read(elementId) && bs->Read(x) && bs->Read(y) && bs->Read(radius) &&
					bs->Read(color) && bs->Read(selectedIndex)) {
					std::string itemsStr, iconsStr;
					uint16_t itemsLen = 0, iconsLen = 0;
					if (bs->Read(itemsLen) && itemsLen > 0) {
						itemsStr.resize(itemsLen);
						bs->Read(&itemsStr[0], itemsLen);
					}
					if (bs->Read(iconsLen) && iconsLen > 0) {
						iconsStr.resize(iconsLen);
						bs->Read(&iconsStr[0], iconsLen);
					}
					DXElement elem;
					elem.id = elementId;
					elem.type = DXElementType::RadialMenu;
					elem.x = x;
					elem.y = y;
					elem.radius = radius;
					elem.color = color;
					elem.selectedIndex = selectedIndex;
					elem.options = SplitString(itemsStr, ';');
					elem.radialIcons = SplitString(iconsStr, ';');
					g_dxElements[elementId] = elem;
					LogToFile("RPC Subtype 30 (DrawRadialMenu): ID=" + std::to_string(elementId) + ", NumItems=" + std::to_string(elem.options.size()));
				}
			}
			else if (subtype == 31) { // SetBlurBehind
				int32_t elementId;
				bool enable;
				if (bs->Read(elementId) && bs->Read(enable)) {
					auto it = g_dxElements.find(elementId);
					if (it != g_dxElements.end()) {
						it->second.blurBehind = enable;
						LogToFile("RPC Subtype 31 (SetBlurBehind): ID=" + std::to_string(elementId) + ", Enable=" + std::to_string(enable));
					}
				}
			}
			else if (subtype == 32) { // DrawColorPicker
				int32_t elementId;
				float x, y, w, h;
				uint32_t selectedColor;
				if (bs->Read(elementId) && bs->Read(x) && bs->Read(y) && bs->Read(w) && bs->Read(h) && bs->Read(selectedColor)) {
					DXElement elem;
					elem.id = elementId;
					elem.type = DXElementType::ColorPicker;
					elem.x = x;
					elem.y = y;
					elem.width = w;
					elem.height = h;
					elem.color = selectedColor;
					g_dxElements[elementId] = elem;
					LogToFile("RPC Subtype 32 (DrawColorPicker): ID=" + std::to_string(elementId));
				}
			}
			else if (subtype == 33) { // DrawScrollContainer
				int32_t elementId;
				float x, y, w, h, contentHeight, scrollVal;
				uint32_t color;
				if (bs->Read(elementId) && bs->Read(x) && bs->Read(y) && bs->Read(w) && bs->Read(h) &&
					bs->Read(contentHeight) && bs->Read(scrollVal) && bs->Read(color)) {
					DXElement elem;
					elem.id = elementId;
					elem.type = DXElementType::ScrollContainer;
					elem.x = x;
					elem.y = y;
					elem.width = w;
					elem.height = h;
					elem.contentHeight = contentHeight;
					elem.progress = scrollVal;
					elem.color = color;
					g_dxElements[elementId] = elem;
					LogToFile("RPC Subtype 33 (DrawScrollContainer): ID=" + std::to_string(elementId));
				}
			}
			else if (subtype == 35) { // DrawIcon
				int32_t elementId;
				float x, y, size;
				uint32_t color;
				if (bs->Read(elementId) && bs->Read(x) && bs->Read(y) && bs->Read(size) && bs->Read(color)) {
					std::string iconName;
					uint16_t nameLen = 0;
					if (bs->Read(nameLen) && nameLen > 0) {
						iconName.resize(nameLen);
						bs->Read(&iconName[0], nameLen);
					}
					std::string fontName;
					uint16_t fontLen = 0;
					if (bs->Read(fontLen) && fontLen > 0) {
						fontName.resize(fontLen);
						bs->Read(&fontName[0], fontLen);
					}
					DXElement elem;
					elem.id = elementId;
					elem.type = DXElementType::Icon;
					elem.x = x;
					elem.y = y;
					elem.width = size;
					elem.height = size;
					elem.text = iconName;
					elem.color = color;
					elem.font = fontName;
					g_dxElements[elementId] = elem;
					LogToFile("RPC Subtype 35 (DrawIcon): ID=" + std::to_string(elementId) + ", IconName='" + iconName + "', Font='" + fontName + "'");
				}
			}
			else if (subtype == 36) { // PlaySound
				uint16_t urlLen = 0;
				std::string url;
				if (bs->Read(urlLen) && urlLen > 0) {
					url.resize(urlLen);
					bs->Read(&url[0], urlLen);
				}
				if (!url.empty()) {
					CreateDirectoryA("omp-dx", NULL);
					CreateDirectoryA("omp-dx\\sounds", NULL);
					
					uint32_t hash = 2166136261u;
					for (char c : url) {
						hash ^= (uint8_t)c;
						hash *= 16777619u;
					}
					std::string ext = ".wav";
					if (url.find(".mp3") != std::string::npos) ext = ".mp3";
					else if (url.find(".ogg") != std::string::npos) ext = ".ogg";
					
					std::string localPath = "omp-dx\\sounds\\" + std::to_string(hash) + ext;
					DWORD fileAttr = GetFileAttributesA(localPath.c_str());
					if (fileAttr != INVALID_FILE_ATTRIBUTES && !(fileAttr & FILE_ATTRIBUTE_DIRECTORY)) {
						LogToFile("RPC Subtype 36 (PlaySound): Local sound file already exists, playing.");
						PlayCustomSound(localPath);
					} else {
						LogToFile("RPC Subtype 36 (PlaySound): Starting background download for sound: " + url);
						std::thread soundThread(DownloadAndPlaySound, url, localPath);
						soundThread.detach();
					}
				}
			}


			if (hasOldElement) {
				auto it = g_dxElements.find(tempElementId);
				if (it != g_dxElements.end()) {
					it->second.draggable = savedDraggable;
					it->second.isDragging = savedIsDragging;
					it->second.dragOffsetX = savedDragOffsetX;
					it->second.dragOffsetY = savedDragOffsetY;
					it->second.isDropped = savedIsDropped;
					it->second.scrollOffset = savedScrollOffset;
					it->second.draggingScroll = savedDraggingScroll;
					it->second.parentId = savedParentId;
					it->second.creationTime = savedCreationTime;
					it->second.isDestroying = savedIsDestroying;
					it->second.destroyRequestedTime = savedDestroyRequestedTime;
					it->second.tooltipText = savedTooltipText;
					it->second.shadowSize = savedShadowSize;
					it->second.shadowOffset = savedShadowOffset;
					it->second.isPassword = savedIsPassword;
					it->second.blurBehind = savedBlurBehind;
					it->second.contentHeight = savedContentHeight;
					if (it->second.graphValues.empty()) it->second.graphValues = savedGraphValues;
					if (it->second.radialIcons.empty()) it->second.radialIcons = savedRadialIcons;
				}
			} else if (tempElementId != -1) {
				auto it = g_dxElements.find(tempElementId);
				if (it != g_dxElements.end()) {
					it->second.creationTime = GetTickCount();
					it->second.lastUpdateFrame = GetTickCount();
				}
			}

			if (restoreCoords) {
				auto it = g_dxElements.find(tempElementId);
				if (it != g_dxElements.end()) {
					it->second.x = savedX;
					it->second.y = savedY;
					it->second.x2 = savedX2;
					it->second.y2 = savedY2;
					it->second.x3 = savedX3;
					it->second.y3 = savedY3;
				}
			}

			if (restoreSliderScale) {
				auto it = g_dxElements.find(tempElementId);
				if (it != g_dxElements.end()) {
					it->second.scale = savedSliderScale;
				}
			}

			return false; // Prevent SA-MP client from parsing it
		}
		return true;
	};

	LogToFile("RakHook RPC listener registered.");

	return game_loop_hook.call_original();
}

void c_plugin::attach_console()
{
	if (!AllocConsole())
		return;

	FILE* f;
	freopen_s(&f, "CONOUT$", "w", stdout);
	freopen_s(&f, "CONOUT$", "w", stderr);
	freopen_s(&f, "CONIN$", "r", stdin);
}

c_plugin::c_plugin(HMODULE hmodule) : hmodule(hmodule)
{
	LogToFile("Plugin loaded via DLL_PROCESS_ATTACH.");
	game_loop_hook.add(&c_plugin::game_loop);
}

c_plugin::~c_plugin()
{
	LogToFile("Plugin unloading.");
	if (hWnd && oWndProc) {
		SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)oWndProc);
		LogToFile("WndProc unhooked.");
	}
	ReleaseFontTextures();
	UnregisterFontResources();
	CleanupImageTextures();
	CleanupBlurTextures();
	if (g_gdiplusToken) {
		Gdiplus::GdiplusShutdown(g_gdiplusToken);
	}
	rakhook::destroy();
}
