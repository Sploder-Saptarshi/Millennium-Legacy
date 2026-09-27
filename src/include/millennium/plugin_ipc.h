/**
 * ==================================================
 *   _____ _ _ _             _
 *  |     |_| | |___ ___ ___|_|_ _ _____
 *  | | | | | | | -_|   |   | | | |     |
 *  |_|_|_|_|_|_|___|_|_|_|_|_|___|_|_|_|
 *
 * ==================================================
 *
 * Copyright (c) 2026 Project Millennium
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include <nlohmann/json.hpp>

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#ifndef _WIN32
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <poll.h>
#else
#include <afunix.h>
#endif

/**
 * millennium (parent) <-> child plugin IPC protocol.
 *
 * format: [4-byte LE length][msgpack payload]
 * msgpack (via nlohmann) instead of JSON text — same schema, ~2x less
 * wire overhead and no parse/stringify cost for the hot path.
 */
namespace plugin_ipc
{
constexpr const char* TYPE_REQUEST = "req";
constexpr const char* TYPE_RESPONSE = "res";
constexpr const char* TYPE_NOTIFY = "notify";

namespace child_method
{
constexpr const char* READY = "ready";
constexpr const char* CALL_FRONTEND_METHOD = "call_frontend_method";
constexpr const char* ADD_BROWSER_CSS = "add_browser_css";
constexpr const char* ADD_BROWSER_JS = "add_browser_js";
constexpr const char* REMOVE_BROWSER_MODULE = "remove_browser_module";
constexpr const char* LOG = "log";
constexpr const char* VERSION = "version";
constexpr const char* STEAM_PATH = "steam_path";
constexpr const char* INSTALL_PATH = "install_path";
constexpr const char* IS_PLUGIN_ENABLED = "is_plugin_enabled";
constexpr const char* CMP_VERSION = "cmp_version";
constexpr const char* PATCHES = "patches";
constexpr const char* HTTP_REQUEST = "http_request";
constexpr const char* HTTP_DOWNLOAD = "http_download";
constexpr const char* CONFIG_GET = "config_get";
constexpr const char* CONFIG_SET = "config_set";
constexpr const char* CONFIG_DELETE = "config_delete";
constexpr const char* CONFIG_GET_ALL = "config_get_all";
} // namespace child_method

namespace parent_method
{
constexpr const char* INIT = "init";
constexpr const char* EVALUATE = "evaluate";
constexpr const char* ON_FRONTEND_LOADED = "on_frontend_loaded";
constexpr const char* GET_METRICS = "get_metrics";
constexpr const char* SHUTDOWN = "shutdown";
constexpr const char* CONFIG_CHANGED = "config_changed";
} // namespace parent_method

#ifdef _WIN32
using socket_fd = SOCKET;
constexpr socket_fd INVALID_FD = INVALID_SOCKET;
#else
using socket_fd = int;
constexpr socket_fd INVALID_FD = -1;
#endif

static constexpr uint32_t MAX_FRAME_SIZE = 16u * 1024u * 1024u; /* 16 MB */

inline bool recv_all(socket_fd fd, void* buf, size_t n)
{
    size_t total = 0;
    while (total < n) {
#ifdef _WIN32
        int r = ::recv(fd, static_cast<char*>(buf) + total, static_cast<int>(n - total), 0);
        if (r == 0) return false; /* peer closed */
        if (r < 0) return false;  /* real error */
#else
        ssize_t r = ::recv(fd, static_cast<char*>(buf) + total, n - total, 0);
        if (r == 0) return false; /* peer closed */
        if (r < 0) {
            if (errno == EINTR) continue; /* signal, retry */
            return false;
        }
#endif
        total += static_cast<size_t>(r);
    }
    return true;
}

inline bool send_all(socket_fd fd, const void* buf, size_t n)
{
    size_t total = 0;
    while (total < n) {
#ifdef _WIN32
        int s = ::send(fd, static_cast<const char*>(buf) + total, static_cast<int>(n - total), 0);
#else
        ssize_t s = ::send(fd, static_cast<const char*>(buf) + total, n - total, MSG_NOSIGNAL);
#endif
        if (s <= 0) return false;
        total += static_cast<size_t>(s);
    }
    return true;
}

/* x86/x64 is always LE so these are no-ops, but kept for clarity */
inline uint32_t to_le32(uint32_t x)
{
    return x;
}
inline uint32_t from_le32(uint32_t x)
{
    return x;
}

inline bool read_frame(socket_fd fd, std::vector<uint8_t>& out)
{
    uint32_t len_le = 0;
    if (!recv_all(fd, &len_le, sizeof(len_le))) return false;

    uint32_t len = from_le32(len_le);
    if (len == 0 || len > MAX_FRAME_SIZE) return false;

    out.resize(len);
    return recv_all(fd, out.data(), len);
}

inline bool write_frame(socket_fd fd, const std::vector<uint8_t>& payload)
{
    uint32_t len_le = to_le32(static_cast<uint32_t>(payload.size()));
    return send_all(fd, &len_le, sizeof(len_le)) && send_all(fd, payload.data(), payload.size());
}

/* convenience wrappers so callers don't have to think about serialization */
inline bool read_msg(socket_fd fd, nlohmann::json& out)
{
    std::vector<uint8_t> buf;
    if (!read_frame(fd, buf)) return false;
    out = nlohmann::json::from_msgpack(buf, true, false);
    return !out.is_discarded();
}

inline bool write_msg(socket_fd fd, const nlohmann::json& msg)
{
    return write_frame(fd, nlohmann::json::to_msgpack(msg));
}

inline void close_fd(socket_fd fd)
{
#ifdef _WIN32
    ::closesocket(fd);
#else
    ::close(fd);
#endif
}

/**
 * Transport endpoints.
 *
 * AF_UNIX sockets only exist on Windows 10 1809+. On legacy hosts (Win7/8/8.1,
 * incl. under VxKex) IPC uses TCP loopback instead, encoded as
 * "tcp://127.0.0.1:<port>". Anything else is a filesystem path for AF_UNIX.
 * Framing (read_frame/write_frame) is transport-agnostic.
 */
inline bool is_tcp_endpoint(const std::string& endpoint)
{
    return endpoint.rfind("tcp://", 0) == 0;
}

inline std::string format_tcp_endpoint(uint16_t port)
{
    return "tcp://127.0.0.1:" + std::to_string(static_cast<unsigned>(port));
}

/* Extracts the port from a "tcp://127.0.0.1:<port>" endpoint, 0 on failure. */
inline uint16_t parse_tcp_endpoint_port(const std::string& endpoint)
{
    if (!is_tcp_endpoint(endpoint)) return 0;
    const std::string::size_type colon = endpoint.rfind(':');
    if (colon == std::string::npos) return 0;
    try {
        const unsigned long port = std::stoul(endpoint.substr(colon + 1));
        return port <= 0xFFFFu ? static_cast<uint16_t>(port) : 0;
    } catch (...) {
        return 0;
    }
}

/* Creates a listening TCP socket on 127.0.0.1:<ephemeral>, outputs the bound
   port via out_port. Returns INVALID_FD on failure. */
inline socket_fd listen_tcp_loopback(uint16_t& out_port)
{
    out_port = 0;
#ifdef _WIN32
    socket_fd fd = static_cast<socket_fd>(::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
    if (fd == INVALID_SOCKET) return INVALID_FD;
#else
    socket_fd fd = static_cast<socket_fd>(::socket(AF_INET, SOCK_STREAM, 0));
    if (fd < 0) return INVALID_FD;
#endif

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0; /* ephemeral */

#ifdef _WIN32
    if (::bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR || ::listen(fd, 16) == SOCKET_ERROR) {
        ::closesocket(fd);
        return INVALID_FD;
    }
#else
    if (::bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0 || ::listen(fd, 16) < 0) {
        ::close(fd);
        return INVALID_FD;
    }
#endif

    sockaddr_in bound{};
#ifdef _WIN32
    int len = sizeof(bound);
#else
    socklen_t len = sizeof(bound);
#endif
    if (::getsockname(fd, reinterpret_cast<sockaddr*>(&bound), &len) != 0) {
        close_fd(fd);
        return INVALID_FD;
    }
    out_port = ntohs(bound.sin_port);
    return fd;
}

/* Connects to either endpoint flavor. Returns INVALID_FD on failure. */
inline socket_fd dial_endpoint(const std::string& endpoint)
{
    if (is_tcp_endpoint(endpoint)) {
        const uint16_t port = parse_tcp_endpoint_port(endpoint);
        if (port == 0) return INVALID_FD;
#ifdef _WIN32
        socket_fd fd = static_cast<socket_fd>(::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
        if (fd == INVALID_SOCKET) return INVALID_FD;
#else
        socket_fd fd = static_cast<socket_fd>(::socket(AF_INET, SOCK_STREAM, 0));
        if (fd < 0) return INVALID_FD;
#endif
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = htons(port);
#ifdef _WIN32
        if (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
            ::closesocket(fd);
            return INVALID_FD;
        }
#else
        if (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
            ::close(fd);
            return INVALID_FD;
        }
#endif
        return fd;
    }

#ifdef _WIN32
    socket_fd fd = static_cast<socket_fd>(::socket(AF_UNIX, SOCK_STREAM, 0));
    if (fd == INVALID_SOCKET) return INVALID_FD;
#else
    socket_fd fd = static_cast<socket_fd>(::socket(AF_UNIX, SOCK_STREAM, 0));
    if (fd < 0) return INVALID_FD;
#endif
    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, endpoint.c_str(), sizeof(addr.sun_path) - 1);
#ifdef _WIN32
    if (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        ::closesocket(fd);
        return INVALID_FD;
    }
#else
    if (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        ::close(fd);
        return INVALID_FD;
    }
#endif
    return fd;
}

#ifdef _WIN32
using poll_fd_t = WSAPOLLFD;
inline int sys_poll(WSAPOLLFD* fds, ULONG n, int ms)
{
    return ::WSAPoll(fds, n, ms);
}
#else
using poll_fd_t = struct pollfd;
inline int sys_poll(struct pollfd* fds, nfds_t n, int ms)
{
    return ::poll(fds, n, ms);
}
#endif

} // namespace plugin_ipc
