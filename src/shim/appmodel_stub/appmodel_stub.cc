// Side-by-side stub for the Win8+ AppModel runtime contract.
//
// Covers the UCRT thread-startup query that terminates Millennium/Steam on
// Windows 7 (incl. under VxKex): beginthreadex() -> __acrt_get_begin_thread_init_policy()
// -> AppPolicyGetThreadInitializationType. Semantics mirror Wine's
// dlls/kernelbase/main.c: report "no special policy" and return success so
// callers take the classic-desktop, non-WinRT path.
//
// Build (Windows, MSVC): this directory is built as part of the main CMake
// project on WIN32 as api-ms-win-appmodel-runtime-l1-1-2.dll. Drop the built
// DLL next to steam.exe (app-local) so the loader prefers it over the
// missing system contract. Alternatively with MinGW:
//   x86_64-w64-mingw32-g++ -shared -O2 -o api-ms-win-appmodel-runtime-l1-1-2.dll appmodel_stub.cc
// Requires nothing beyond kernel32.
#include <windows.h>

// Keep in sync with appmodel.h (SDK) without requiring the Win10 SDK on the
// Win7 build host.
typedef enum AppPolicyThreadInitializationType {
    AppPolicyThreadInitializationType_None = 0,
    AppPolicyThreadInitializationType_InitializeWinRT = 1,
} AppPolicyThreadInitializationType;

typedef enum AppPolicyProcessTerminationMethod {
    AppPolicyProcessTerminationMethod_ExitProcess = 0,
    AppPolicyProcessTerminationMethod_TerminateProcess = 1,
} AppPolicyProcessTerminationMethod;

typedef enum AppPolicyWindowingModel {
    AppPolicyWindowingModel_ClassicDesktop = 0,
    AppPolicyWindowingModel_Universal = 1,
} AppPolicyWindowingModel;

typedef enum AppPolicyMediaFoundationCodecLoading {
    AppPolicyMediaFoundationCodecLoading_All = 0,
    AppPolicyMediaFoundationCodecLoading_InboxOnly = 1,
} AppPolicyMediaFoundationCodecLoading;

typedef enum AppPolicyShowDeveloperDiagnostic {
    AppPolicyShowDeveloperDiagnostic_ShowUI = 0,
    AppPolicyShowDeveloperDiagnostic_NoUI = 1,
} AppPolicyShowDeveloperDiagnostic;

#define STUB_EXPORT extern "C" __declspec(dllexport)

// UCRT beginthreadex policy query. None == classic desktop startup.
STUB_EXPORT LONG WINAPI AppPolicyGetThreadInitializationType(HANDLE processToken, AppPolicyThreadInitializationType* policy)
{
    (void)processToken;
    if (policy) {
        *policy = AppPolicyThreadInitializationType_None;
    }
    return ERROR_SUCCESS;
}

STUB_EXPORT LONG WINAPI AppPolicyGetProcessTerminationMethod(HANDLE processToken, AppPolicyProcessTerminationMethod* policy)
{
    (void)processToken;
    if (policy) {
        *policy = AppPolicyProcessTerminationMethod_ExitProcess;
    }
    return ERROR_SUCCESS;
}

STUB_EXPORT LONG WINAPI AppPolicyGetWindowingModel(HANDLE processToken, AppPolicyWindowingModel* policy)
{
    (void)processToken;
    if (policy) {
        *policy = AppPolicyWindowingModel_ClassicDesktop;
    }
    return ERROR_SUCCESS;
}

STUB_EXPORT LONG WINAPI AppPolicyGetMediaFoundationCodecLoading(HANDLE processToken, AppPolicyMediaFoundationCodecLoading* policy)
{
    (void)processToken;
    if (policy) {
        *policy = AppPolicyMediaFoundationCodecLoading_All;
    }
    return ERROR_SUCCESS;
}

STUB_EXPORT LONG WINAPI AppPolicyGetShowDeveloperDiagnostic(HANDLE processToken, AppPolicyShowDeveloperDiagnostic* policy)
{
    (void)processToken;
    if (policy) {
        *policy = AppPolicyShowDeveloperDiagnostic_ShowUI;
    }
    return ERROR_SUCCESS;
}

BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID)
{
    return TRUE;
}
