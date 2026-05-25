#include <windows.h>
#include <iostream>

#include "Plugin.h"

std::unique_ptr<c_plugin> plugin;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason) {
		case DLL_PROCESS_ATTACH:
			DisableThreadLibraryCalls(hModule);
			plugin = std::make_unique<c_plugin>(hModule);
			break;
		case DLL_PROCESS_DETACH:
			plugin.reset();
			break;
	}
	return TRUE;
}
