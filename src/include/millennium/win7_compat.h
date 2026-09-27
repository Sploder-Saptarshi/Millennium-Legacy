// Legacy-Windows (7 / 8 / 8.1, incl. under VxKex) compatibility helpers.
//
// Background: on Windows 7 the UCRT's beginthreadex() path queries
// AppPolicyGetThreadInitializationType (api-ms-win-appmodel-runtime-l1-1-2,
// forwarded from kernel32/kernelbase). That export does not exist on Win7 and
// VxKex does not shim it, so any std::thread creation can end in
// std::terminate -> abort (0xE0000001). These helpers let Millennium detect
// that environment and stay on a notify-only update path. The actual stub for
// the missing export lives in src/shim/appmodel_stub/.
#pragma once

namespace win7_compat
{
// True when running on pre-10 Windows, under VxKex, or when the AppModel
// runtime export is missing. Result is cached after the first call.
bool is_legacy_windows();

// True when AppPolicyGetThreadInitializationType can be resolved from a
// loaded kernel32/kernelbase/api-ms module. Result is cached.
bool appmodel_runtime_available();

// True when an automatic download+extract+replace is considered safe:
// either not legacy, or the user explicitly opted in via
// general.allowLegacyAutoUpdate.
bool auto_update_allowed();

// True when IPC must use TCP loopback instead of AF_UNIX (absent before
// Win10 1809, and no VxKex shim can add an address family).
bool use_tcp_loopback();
} // namespace win7_compat
