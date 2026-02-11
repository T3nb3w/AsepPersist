#include "AsepPersist.h"
#include <Psapi.h>
#include <wininet.h>


class AutoRegCloseKey
{
	HKEY m_Key;
public:
	// Remove Copy Semantics
	AutoRegCloseKey(const AutoRegCloseKey&) = delete;
	void operator=(const AutoRegCloseKey&) = delete;
	
	AutoRegCloseKey() { Clear(); }
	explicit AutoRegCloseKey(HKEY Key) { Set(Key); }
	~AutoRegCloseKey() { if (m_Key) { RegCloseKey(m_Key); Clear(); } }
	
	void Set(HKEY Key) { m_Key = Key; }
	void Clear() { m_Key = nullptr; }
}; 


bool InstallASEP()
{
    const HKEY AUTOSTART_HIVE{ HKEY_CURRENT_USER };
    constexpr LPCWSTR AUTOSTART_REGKEY{
        // RunOnce is FINE — the exe re-registers on each shutdown
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce"
    };
    constexpr LPCWSTR AUTOSTART_REGVALUE{ L"AutoStart" };

    // FIX: Use a properly sized buffer and shrink after call
    WCHAR ExePathBuf[MAX_PATH] = {};
    ULONG ModResult = GetModuleFileNameExW(
        GetCurrentProcess(),
        nullptr,
        ExePathBuf,
        MAX_PATH
    );

    if (!ModResult)
    {
        OutputDebugStringW(L"[-] GetModuleFileNameExW failed\n");
        return false;
    }

    // ExePathBuf is a plain array — exactly ModResult chars + null
    // No trailing garbage, no size mismatch
    DWORD ByteSize = (ModResult + 1) * sizeof(WCHAR);

    HKEY KeyHandle = nullptr;
    LSTATUS Result = RegOpenKeyExW(
        AUTOSTART_HIVE, AUTOSTART_REGKEY, 0, KEY_WRITE, &KeyHandle
    );
    if (Result != ERROR_SUCCESS) return false;

    AutoRegCloseKey RAIIKeyHandle(KeyHandle);

    Result = RegSetValueExW(
        KeyHandle,
        AUTOSTART_REGVALUE,
        0,
        REG_SZ,
        reinterpret_cast<const BYTE*>(ExePathBuf),
        ByteSize   // ← exact size, no padding
    );

    return Result == ERROR_SUCCESS;
}