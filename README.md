# AutoStart — Windows ASEP Persistence via Hidden Window

## Overview

AutoStart is a Windows persistence proof-of-concept that demonstrates how a program can survive reboots and logoffs using only legitimate, documented Win32 APIs. It uses a **self-reinstalling RunOnce** strategy,  rather than permanently sitting in the registry, it re-writes its own autostart entry every time the system shuts down or the user logs off.

No admin privileges are required. Everything operates entirely within the current user's context (`HKEY_CURRENT_USER`).

---

## How It Works

The mechanism relies on three components working together in a continuous cycle:

```
Boot / Login
    │
    ▼
RunOnce launches the EXE
    │
    ▼
EXE creates a hidden window → enters message loop (sleeping, zero CPU)
    │
    ▼
User works normally...
    │
    ▼
Shutdown / Logoff triggered
    │
    ▼
WM_ENDSESSION fires → InstallASEP() writes path to RunOnce
    │
    ▼
System shuts down → on next boot, cycle repeats forever
```

### Stage 1 — Hidden Window & Message Loop

On startup, the program registers a window class with Windows and creates a zero-size, fully hidden window. This window has no visible presence, no taskbar entry, no border, no title. It exists purely as a message receiver inside the Windows kernel.

The main thread then enters a blocking message loop (`GetMessageW`), suspending itself until a system event arrives. The program consumes no CPU while waiting.

### Stage 2 — Shutdown Notification via `WM_ENDSESSION`

When the user initiates a shutdown or logoff, Windows broadcasts `WM_ENDSESSION` to all top-level windows — including hidden ones. Crucially, this is a *sent* message, meaning Windows calls the window procedure directly and waits for it to return before proceeding with shutdown. This gives the program a guaranteed execution window right before the system halts.

The `lParam` of `WM_ENDSESSION` identifies the reason:

| `lParam` value | Meaning |
|---|---|
| `0x00000000` | Full shutdown or restart |
| `ENDSESSION_LOGOFF` | User logging off |

### Stage 3 — Registry Write (`InstallASEP`)

When `WM_ENDSESSION` fires, `InstallASEP()` runs. It:

1. Queries its own full executable path using `GetModuleFileNameExW`
2. Opens `HKCU\SOFTWARE\Microsoft\Windows\CurrentVersion\RunOnce` with write access
3. Writes the path as a `REG_SZ` value named `AutoStart`
4. Closes the registry handle cleanly via RAII (`AutoRegCloseKey`)

On the next boot, Windows reads `RunOnce`, launches the executable, then deletes the entry. But since the EXE is now running again, it re-installs the entry on the *next* shutdown,  perpetuating the cycle indefinitely.

---

## Key Design Decisions

**Why `RunOnce` instead of `Run`?**
`RunOnce` entries are deleted before execution, making them slightly less visible to casual registry inspection than a permanent `Run` entry. The self-reinstalling design means the entry only exists in the registry for the brief period between shutdown and the next login.

**Why a hidden window instead of a service or scheduled task?**
A hidden window requires no elevated privileges, no service registration, and no task scheduler interaction. It uses standard Win32 message infrastructure that is present on every version of Windows.

**Why `NtQueryInformationProcess` for the parent PID?**
The native NT API operates below the Win32 layer, making it less observable to security tools that hook higher-level APIs like `CreateToolhelp32Snapshot`. `Reserved3` in the public `PROCESS_BASIC_INFORMATION` struct is the undocumented field `InheritedFromUniqueProcessId`.

**Why RAII for the registry handle?**
`AutoRegCloseKey` ensures `RegCloseKey` is always called regardless of which code path is taken,  including early returns on error. This prevents handle leaks in a function with multiple exit points.

---

## File Structure

| File | Purpose |
|---|---|
| `AsepPersist.h` | Shared header, declares `InstallASEP()` |
| `AsepPersist.cpp` | Implements `InstallASEP()` — registry write logic |
| `AutoStart.cpp` | Implements `WindowProc`, `ProcessMessage`, `GetParentProcessId` |

---

## APIs Used

| API | Purpose |
|---|---|
| `NtQueryInformationProcess` | Retrieve parent process ID via NT layer |
| `RegisterClassExW` | Register hidden window class with callback |
| `CreateWindowExW` | Create zero-size invisible window |
| `ShowWindow(SW_HIDE)` | Ensure window has no visual presence |
| `GetMessageW` | Block thread until system message arrives |
| `DispatchMessageW` | Route message to `WindowProc` callback |
| `WM_ENDSESSION` | Shutdown/logoff notification from Windows |
| `GetModuleFileNameExW` | Retrieve own executable path dynamically |
| `RegOpenKeyExW` | Open `RunOnce` registry key with write access |
| `RegSetValueExW` | Write executable path as `REG_SZ` value |

---

## Registry Target

```
HKEY_CURRENT_USER
  └── SOFTWARE
        └── Microsoft
              └── Windows
                    └── CurrentVersion
                          └── RunOnce
                                └── AutoStart = "C:\path\to\AutoStart.exe"
```

---

## Requirements

- Windows (any modern version)
- No administrator privileges required
- Visual Studio with C++17 or later
- `ntdll.lib` (linked via `#pragma comment(lib, "ntdll")`)
- `Psapi.lib` (for `GetModuleFileNameExW`)

---

## Legal Disclaimer

**FOR EDUCATIONAL AND AUTHORIZED SECURITY RESEARCH ONLY**

This tool is provided for:
- Educational purposes to understand Windows security mechanisms
- Authorized penetration testing in controlled environments
- Security research and vulnerability analysis
- Red team exercises with proper authorization

**⚡ Remember**: With great power comes great responsibility. Use ethically.
