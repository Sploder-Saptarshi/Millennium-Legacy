// See include/millennium/win7_compat.h for rationale.
#include "millennium/win7_compat.h"

#ifdef _WIN32
#include <windows.h>
// RtlGetVersion from ntdll is less likely to be version-spoofed than the
// VerifyVersionInfo path, so prefer it for the "real" OS check.
#include <winternl.h>
#endif

#ifdef _WIN32
#include "millennium/config.h"
#endif

namespace win7_compat
{
#ifdef _WIN32
static bool has_vxkex_modules()
{
    // VxKex injects these when enabled for the process; their presence means
    // the version lie (Win10) is active but API coverage is still Win7-level.
    static const wchar_t* modules[] = { L"KexDll.dll", L"KxBase.dll", L"KxNt.dll" };
    for (const auto* name : modules) {
        if (GetModuleHandleW(name) != nullptr) {
            return true;
        }
    }
    return false;
}

static bool has_appmodel_export()
{
    static const char* proc = "AppPolicyGetThreadInitializationType";
    // UCRT may bind it via kernel32, kernelbase, or the api-set contract.
    static const wchar_t* hosts[] = {
        L"kernel32.dll",
        L"kernelbase.dll",
        L"api-ms-win-appmodel-runtime-l1-1-2.dll",
    };
    for (const auto* host : hosts) {
        if (HMODULE mod = GetModuleHandleW(host)) {
            if (GetProcAddress(mod, proc) != nullptr) {
                return true;
            }
        }
    }
    // Fall back to an explicit load of the api-set contract; a side-by-side
    // stub (src/shim/appmodel_stub) also satisfies this.
    if (HMODULE mod = LoadLibraryExW(L"api-ms-win-appmodel-runtime-l1-1-2.dll", nullptr, LOAD_LIBRARY_AS_DATAFILE)) {
        bool found = GetProcAddress(mod, proc) != nullptr;
        FreeLibrary(mod);
        if (found) {
            return true;
        }
    }
    return false;
}

static bool real_os_is_pre10()
{
    // RtlGetVersion reports the true kernel version even when the app-compat
    // / VxKex layer spoofs VerifyVersionInfo/GetVersionEx.
    if (HMODULE ntdll = GetModuleHandleW(L"ntdll.dll")) {
        if (auto rtl_get_version = reinterpret_cast<LONG(WINAPI*)(PRTL_OSVERSIONINFOW)>(reinterpret_cast<void*>(GetProcAddress(ntdll, "RtlGetVersion")))) {
            RTL_OSVERSIONINFOW info{};
            info.dwOSVersionInfoSize = sizeof(info);
            if (rtl_get_version(&info) == 0 /* STATUS_SUCCESS */) {
                return info.dwMajorVersion < 10;
            }
        }
    }
    return false;
}
#endif

bool is_legacy_windows()
{
#ifdef _WIN32
    static const bool cached = has_vxkex_modules() || !has_appmodel_export() || real_os_is_pre10();
    return cached;
#else
    return false;
#endif
}

bool appmodel_runtime_available()
{
#ifdef _WIN32
    static const bool cached = has_appmodel_export();
    return cached;
#else
    return true;
#endif
}

bool auto_update_allowed()
{
#ifdef _WIN32
    if (!is_legacy_windows()) {
        return true;
    }
    // Explicit opt-in for legacy hosts; default off (see default_cfg.cc).
    try {
        return CONFIG.get({ "general", "allowLegacyAutoUpdate" }, false).get<bool>();
    } catch (...) {
        return false;
    }
#else
    return true;
#endif
}

bool use_tcp_loopback()
{
#ifdef _WIN32
    return is_legacy_windows();
#else
    return false;
#endif
}
} // namespace win7_compat
