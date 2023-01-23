#include "TmForeverResFix.h"
#include "minhook/include/MinHook.h"
#include <string>
#include <ShlObj.h>

enum E_FullscreenMode
{
	E_WINDOWED,
	E_FULLSCREEN,
	E_BORDERLESS
};

std::string ReadFromIni(LPCSTR section, LPCSTR key, LPCSTR default_, LPCSTR file)
{
	char temp[1024];
	int res = GetPrivateProfileStringA(section, key, default_, temp, sizeof(temp), file);
	return std::string(temp, res);
}

std::string GetPathToWindir()
{
	char temp[MAX_PATH];
	SHGetSpecialFolderPathA(NULL, temp, CSIDL_SYSTEMX86, FALSE);
	return std::string(temp, strlen(temp));
}

static BOOL pShouldUnfullscreenOnExit = FALSE;

typedef BOOL (WINAPI* ShowWindow_t)(HWND, int);
static ShowWindow_t pShowWindow;

BOOL WINAPI HookShowWindow(HWND hWnd, int nCmdShow)
{
	static BOOL swDone = false;

	if (!swDone)
	{
		auto nadeoIniTitle = ReadFromIni("TmForever", "WindowTitle",
			"TrackMania United Forever: Window Patch", ".\\Nadeo.ini");

		auto iniTitle = ReadFromIni("Window", "Title", nadeoIniTitle.c_str(), ".\\TmWindow.ini");
		auto iniWidth = ReadFromIni("Window", "Width", "1280", ".\\TmWindow.ini");
		auto iniHeight = ReadFromIni("Window", "Height", "720", ".\\TmWindow.ini");
		auto iniFsMode = ReadFromIni("Window", "FullscreenMode", "0", ".\\TmWindow.ini");

		auto iWidth = std::atoi(iniWidth.c_str());
		auto iHeight = std::atoi(iniHeight.c_str());
		auto fsMode = (E_FullscreenMode)std::atoi(iniFsMode.c_str());

		SetWindowTextA(hWnd, iniTitle.c_str());

		switch (fsMode)
		{
		// Borderless windowed (based, correct choice)
		case E_BORDERLESS:
		{
			auto smScreenw = GetSystemMetrics(SM_CXSCREEN);
			auto smScreenh = GetSystemMetrics(SM_CYSCREEN);

			iWidth = smScreenw;
			iHeight = smScreenh;

			// Go borderless
			SetWindowLongPtr(hWnd, GWL_STYLE, WS_VISIBLE | WS_POPUP);

			SetWindowPos(hWnd, HWND_TOP, 0, 0, iWidth, iHeight,
				SWP_FRAMECHANGED);

			break;
		}
		// Regular windowed
		case E_WINDOWED:
		{
			SetWindowPos(hWnd, HWND_TOP, 200, 200, iWidth, iHeight,
				SWP_NOMOVE | SWP_NOREPOSITION);

			break;
		}
		// Why would you ever use this
		case E_FULLSCREEN:
		{
			// Do a bunch of bs with winapi garbage to go fullscreen
			DEVMODE dmSettings;

			memset(&dmSettings, 0, sizeof(DEVMODE));

			EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dmSettings);

			dmSettings.dmPelsWidth = iWidth;
			dmSettings.dmPelsHeight = iHeight;
			dmSettings.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;

			ChangeDisplaySettings(&dmSettings, CDS_FULLSCREEN);

			SetWindowLongPtr(hWnd, GWL_STYLE,
				WS_VISIBLE | WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);

			SetWindowPos(hWnd, HWND_TOP, 0, 0, iWidth, iHeight,
				SWP_FRAMECHANGED);

			pShouldUnfullscreenOnExit = TRUE;

			break;
		}
		}

		// Tell the game our window size changed
		WINDOWPOS pos;
		memset(&pos, 0, sizeof(WINDOWPOS));
		pos.hwnd = hWnd;
		pos.hwndInsertAfter = HWND_TOP;
		pos.x = 0;
		pos.y = 0;
		pos.cx = iWidth;
		pos.cy = iHeight;
		pos.flags = SWP_NOMOVE | SWP_NOREPOSITION;

		RECT r;
		memset(&r, 0, sizeof(RECT));
		r.left = 0;
		r.top = 0;
		r.right = iWidth;
		r.bottom = iHeight;

		// One of these PostMessages is probably redundant
		// but I'm leaving them in, else DX misbehaves
		PostMessage(hWnd, WM_WINDOWPOSCHANGING, 0, (LPARAM)&pos);
		PostMessage(hWnd, WM_WINDOWPOSCHANGED, 0, (LPARAM)&pos);
		PostMessage(hWnd, WM_MOVING, 0, (LPARAM)&r);
		PostMessage(hWnd, WM_MOVE, 0, 0);

		swDone = true;
	}

	return pShowWindow(hWnd, nCmdShow);
}

static HMODULE hD3d;
typedef void* (WINAPI* Direct3DCreate9_t)(UINT);
typedef int (WINAPI* D3DPERF_SetOptions_t)(DWORD);

typedef void (WINAPI* PostQuitMessage_t)(INT);
static PostQuitMessage_t pPostQuitMessage;

void HookPostQuitMessage(INT x)
{
	if (pShouldUnfullscreenOnExit)
	{
		// Just unfullscreen the game
		ChangeDisplaySettings(NULL, 0);
	}

	pPostQuitMessage(x);
}

extern "C"
{

TMPATCH_EXPORTS int WINAPI D3DPERF_SetOptions(DWORD a)
{
	D3DPERF_SetOptions_t actualFn = (D3DPERF_SetOptions_t)GetProcAddress(hD3d, "D3DPERF_SetOptions");
	return actualFn(a);
}

TMPATCH_EXPORTS void* WINAPI Direct3DCreate9(UINT a)
{
	if (hD3d)
	{
		Direct3DCreate9_t actualFn = (Direct3DCreate9_t)GetProcAddress(hD3d, "Direct3DCreate9");
		return actualFn(a);
	}

	auto windirPath = GetPathToWindir();
	windirPath.append("\\d3d9.dll");
	hD3d = LoadLibraryA(windirPath.c_str());
	if (!hD3d)
	{
		MessageBoxW(NULL,
			L"Could not load d3d9.dll",
			L"TrackMania Forever Res Fix",
			MB_OK | MB_ICONERROR);
		ExitProcess(1);
		return NULL;
	}

	Direct3DCreate9_t actualFn = (Direct3DCreate9_t)GetProcAddress(hD3d, "Direct3DCreate9");
	if (!actualFn)
	{
		MessageBoxW(NULL,
			L"Could not find Direct3DCreate9 function",
			L"TrackMania Forever Res Fix",
			MB_OK | MB_ICONERROR);
		ExitProcess(1);
		return NULL;
	}

	MH_Initialize();
	MH_CreateHookApi(L"user32", "ShowWindow", HookShowWindow, reinterpret_cast<LPVOID*>(&pShowWindow));
	MH_CreateHookApi(L"user32", "PostQuitMessage", HookPostQuitMessage, reinterpret_cast<LPVOID*>(&pPostQuitMessage));
	MH_EnableHook(MH_ALL_HOOKS);

	return actualFn(a);
}

}
