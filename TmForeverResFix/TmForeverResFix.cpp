#include "TmForeverResFix.h"
#include "minhook/include/MinHook.h"
#include <string>
#include <ShlObj.h>

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
		auto iWidth = std::atoi(iniWidth.c_str());
		auto iHeight = std::atoi(iniHeight.c_str());

		SetWindowTextA(hWnd, iniTitle.c_str());
		SetWindowPos(hWnd, HWND_TOP, 200, 200, iWidth, iHeight,
			SWP_NOMOVE | SWP_NOREPOSITION);

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
	MH_EnableHook(MH_ALL_HOOKS);

	return actualFn(a);
}

}
