#include <Windows.h>
#include "AsepPersist.h"


int WINAPI
WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR LpCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hInstance);
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(LpCmdLine);
	UNREFERENCED_PARAMETER(nCmdShow);

	DWORD ParentPid = GetParentProcessId();

	std::wstring msg = std::format(L"[+] ParentPid=0x{:08x}\n", ParentPid);
	OutputDebugStringW(msg.c_str());
	// Process Window messages
	// Blocks till the application terminates
	ProcessMessage(hInstance);

	return EXIT_SUCCESS;
}