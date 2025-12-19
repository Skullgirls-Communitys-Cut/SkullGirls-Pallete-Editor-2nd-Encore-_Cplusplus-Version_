#include "pch.h"

#pragma pack(1)

HINSTANCE hlThis = 0;
HINSTANCE hlD3D9 = 0;
FARPROC origProc[15] = { 0 };


bool DirectXWrapper::Init() {
    char system_Path[MAX_PATH] = { 0 };

    // Получаем путь к SysWOW64 директории
    if (GetSystemWow64DirectoryA(system_Path, MAX_PATH) == 0) { //For 64-bit
        GetSystemDirectoryA(system_Path, MAX_PATH);
    }

    // path to the original dll
    strcat_s(system_Path, "\\d3d9.dll");

    // Загружаем библиотеку
    HMODULE g_OriginalD3D9 = LoadLibraryA(system_Path);
    if (!g_OriginalD3D9) {
        return false;
    }

    origProc[0] = GetProcAddress(g_OriginalD3D9, "D3DPERF_BeginEvent");
    origProc[1] = GetProcAddress(g_OriginalD3D9, "D3DPERF_EndEvent");
    origProc[2] = GetProcAddress(g_OriginalD3D9, "D3DPERF_GetStatus");
    origProc[3] = GetProcAddress(g_OriginalD3D9, "D3DPERF_QueryRepeatFrame");
    origProc[4] = GetProcAddress(g_OriginalD3D9, "D3DPERF_SetMarker");
    origProc[5] = GetProcAddress(g_OriginalD3D9, "D3DPERF_SetOptions");
    origProc[6] = GetProcAddress(g_OriginalD3D9, "D3DPERF_SetRegion");
    origProc[7] = GetProcAddress(g_OriginalD3D9, "DebugSetLevel");
    origProc[8] = GetProcAddress(g_OriginalD3D9, "DebugSetMute");
    origProc[9] = GetProcAddress(g_OriginalD3D9, "Direct3DCreate9");
    origProc[10] = GetProcAddress(g_OriginalD3D9, "Direct3DCreate9Ex");
    origProc[11] = GetProcAddress(g_OriginalD3D9, "Direct3DShaderValidatorCreate9");
    origProc[12] = GetProcAddress(g_OriginalD3D9, "PSGPError");
    origProc[13] = GetProcAddress(g_OriginalD3D9, "PSGPSampleTexture");

    return true;
}

//Direct3DCreate9
extern "C" IDirect3D9* WINAPI __ProxyFunc9(UINT SDKVersion) {
    // Получаем оригинальную функцию
    typedef IDirect3D9* (WINAPI* Direct3DCreate9Func)(UINT sdkver);
    Direct3DCreate9Func origDirect3DCreate9 = (Direct3DCreate9Func)origProc[9];

    if (!origDirect3DCreate9) {
        return NULL;
    }

    // Вызываем оригинальную функцию
    IDirect3D9* res = origDirect3DCreate9(SDKVersion);

    if (res) {
        // Создаём прокси
        return new IDirect3D9Proxy(res);
    }

    return NULL;
}

//Direct3DCreate9Ex
extern "C" __declspec(naked) void __stdcall __ProxyFunc10() {
	__asm {
		jmp origProc[10 * 4];
	}
}

extern "C" __declspec(naked) void __stdcall __ProxyFunc0() {
	__asm {
		jmp origProc[0 * 4];
	}
}
extern "C" __declspec(naked) void __stdcall __ProxyFunc1() {
	__asm {
		jmp origProc[1 * 4];
	}
}
extern "C" __declspec(naked) void __stdcall __ProxyFunc2() {
	__asm {
		jmp origProc[2 * 4];
	}
}
extern "C" __declspec(naked) void __stdcall __ProxyFunc3() {
	__asm {
		jmp origProc[3 * 4];
	}
}
extern "C" __declspec(naked) void __stdcall __ProxyFunc4() {
	__asm {
		jmp origProc[4 * 4];
	}
}
extern "C" __declspec(naked) void __stdcall __ProxyFunc5() {
	__asm {
		jmp origProc[5 * 4];
	}
}
extern "C" __declspec(naked) void __stdcall __ProxyFunc6() {
	__asm {
		jmp origProc[6 * 4];
	}
}
extern "C" __declspec(naked) void __stdcall __ProxyFunc7() {
	__asm {
		jmp origProc[7 * 4];
	}
}
extern "C" __declspec(naked) void __stdcall __ProxyFunc8() {
	__asm {
		jmp origProc[8 * 4];
	}
}
extern "C" __declspec(naked) void __stdcall __ProxyFunc11() {
	__asm {
		jmp origProc[11 * 4];
	}
}
extern "C" __declspec(naked) void __stdcall __ProxyFunc12() {
	__asm {
		jmp origProc[12 * 4];
	}
}
extern "C" __declspec(naked) void __stdcall __ProxyFunc13() {
	__asm {
		jmp origProc[13 * 4];
	}
}