# AppModel stub for legacy Windows

`api-ms-win-appmodel-runtime-l1-1-2.dll` — app-local stub covering the
`AppPolicy*` exports missing on Windows 7/8/8.1 that the UCRT thread-startup
path (`beginthreadex` -> `AppPolicyGetThreadInitializationType`) queries.
Without it, any `std::thread` creation under VxKex can end in
`std::terminate` / `abort (0xE0000001)`. Semantics mirror Wine's
`dlls/kernelbase/main.c`: report classic-desktop defaults, return
`ERROR_SUCCESS`.

## Deploy

Copy the built DLL next to `steam.exe` (and optionally next to
`millennium.dll`). The loader resolves the api-set contract app-locally
first. No registry or VxKex config change needed.

## Interplay with guards

`src/include/millennium/win7_compat.h` detects this stub via
`GetProcAddress`: with the stub present, `appmodel_runtime_available()`
returns true, but `is_legacy_windows()` still returns true under VxKex, so
Millennium stays notify-only for updates unless
`general.allowLegacyAutoUpdate` is set. The stub fixes thread creation; the
guards keep the updater's download/extract/replace cycle off fragile hosts.
