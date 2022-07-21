#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#define TMPATCH_EXPORTS __declspec(dllexport)

extern "C"
{

TMPATCH_EXPORTS int WINAPI D3DPERF_SetOptions(DWORD);
TMPATCH_EXPORTS void* WINAPI Direct3DCreate9(UINT);

}