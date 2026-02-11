#pragma once
#include <Windows.h>
#include <format>
#include <iostream>

DWORD GetParentProcessId(VOID);
bool ProcessMessage(HINSTANCE hInstance);
bool InstallASEP();
