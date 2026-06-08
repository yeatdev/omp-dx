#include <Windows.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

#include <cstring>

namespace {
HMODULE g_proxyModule = nullptr;
HMODULE g_realDinput8 = nullptr;
HMODULE g_asiModule = nullptr;

HMODULE LoadRealDinput8()
{
	if (g_realDinput8) {
		return g_realDinput8;
	}

	char systemPath[MAX_PATH] {};
	if (!GetSystemDirectoryA(systemPath, static_cast<UINT>(sizeof(systemPath)))) {
		return nullptr;
	}

	strcat_s(systemPath, "\\dinput8.dll");
	g_realDinput8 = LoadLibraryA(systemPath);
	return g_realDinput8;
}

void LoadOmpDxAsi()
{
	if (g_asiModule) {
		return;
	}

	char asiPath[MAX_PATH] {};
	if (!GetModuleFileNameA(g_proxyModule, asiPath, static_cast<DWORD>(sizeof(asiPath)))) {
		return;
	}

	char* slash = strrchr(asiPath, '\\');
	if (slash == nullptr) {
		return;
	}
	strcpy_s(slash + 1, MAX_PATH - (slash - asiPath) - 1, "omp-dx.asi");

	g_asiModule = LoadLibraryA(asiPath);
}

template <typename Fn>
Fn GetRealProc(const char* name)
{
	HMODULE real = LoadRealDinput8();
	if (!real) {
		return nullptr;
	}
	return reinterpret_cast<Fn>(GetProcAddress(real, name));
}
}

extern "C" HRESULT WINAPI DirectInput8Create(HINSTANCE hinst, DWORD version, REFIID riidltf, LPVOID* out, LPUNKNOWN outer)
{
	using Fn = HRESULT(WINAPI*)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);
	Fn fn = GetRealProc<Fn>("DirectInput8Create");
	if (!fn) {
		return E_FAIL;
	}
	return fn(hinst, version, riidltf, out, outer);
}

extern "C" HRESULT WINAPI DllCanUnloadNow()
{
	using Fn = HRESULT(WINAPI*)();
	Fn fn = GetRealProc<Fn>("DllCanUnloadNow");
	return fn ? fn() : S_FALSE;
}

extern "C" HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID iid, LPVOID* out)
{
	using Fn = HRESULT(WINAPI*)(REFCLSID, REFIID, LPVOID*);
	Fn fn = GetRealProc<Fn>("DllGetClassObject");
	return fn ? fn(clsid, iid, out) : CLASS_E_CLASSNOTAVAILABLE;
}

extern "C" HRESULT WINAPI DllRegisterServer()
{
	using Fn = HRESULT(WINAPI*)();
	Fn fn = GetRealProc<Fn>("DllRegisterServer");
	return fn ? fn() : E_NOTIMPL;
}

extern "C" HRESULT WINAPI DllUnregisterServer()
{
	using Fn = HRESULT(WINAPI*)();
	Fn fn = GetRealProc<Fn>("DllUnregisterServer");
	return fn ? fn() : E_NOTIMPL;
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID)
{
	if (reason == DLL_PROCESS_ATTACH) {
		g_proxyModule = module;
		DisableThreadLibraryCalls(module);
		LoadOmpDxAsi();
	}
	return TRUE;
}
