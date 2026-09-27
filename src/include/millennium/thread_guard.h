// thread_guard: keep worker-thread failures from aborting the process.
//
// An exception escaping a std::thread entry calls std::terminate -> abort()
// (0xE0000001). On fragile hosts (Win7/8/8.1 under VxKex) ordinary failures
// (network/TLS, filesystem, pipes) are more likely, so every Millennium-owned
// thread entry should run through thread_guard::run(), which converts an
// escaping exception into a log line. Use make_thread() when spawning.
#pragma once

#include "millennium/logger.h"

#include <exception>
#include <thread>
#include <utility>

namespace thread_guard
{
// Run fn(), logging (not throwing) any escaping exception.
template <typename Fn> void run(const char* thread_name, Fn&& fn)
{
    try {
        std::forward<Fn>(fn)();
    } catch (const std::exception& e) {
        LOG_ERROR("Thread '{}' swallowed exception: {}", thread_name, e.what());
    } catch (...) {
        LOG_ERROR("Thread '{}' swallowed unknown exception.", thread_name);
    }
}

// std::thread equivalent that guards the entry point.
template <typename Fn> std::thread make_thread(const char* thread_name, Fn&& fn)
{
    return std::thread([name = thread_name, fn = std::forward<Fn>(fn)]() mutable
    {
        run(name, std::move(fn));
    });
}
} // namespace thread_guard
