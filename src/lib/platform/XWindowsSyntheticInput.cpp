/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2026 InputLeap contributors
 */

#include "platform/XWindowsSyntheticInput.h"

#include <deque>
#include <mutex>
#include <unordered_map>

namespace inputleap {
namespace {
std::mutex g_mutex;
std::unordered_map<Display*, std::deque<unsigned long>> g_requests;
constexpr std::size_t kMaximumPendingRequests = 256;
}

void mark_xwindows_synthetic_request(Display* display, unsigned long serial)
{
    if (display == nullptr) return;
    std::lock_guard<std::mutex> lock(g_mutex);
    auto& requests = g_requests[display];
    requests.push_back(serial);
    if (requests.size() > kMaximumPendingRequests) requests.pop_front();
}

bool consume_xwindows_synthetic_event(Display* display, unsigned long serial)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    auto found = g_requests.find(display);
    if (found == g_requests.end()) return false;
    auto& requests = found->second;
    for (auto it = requests.begin(); it != requests.end(); ++it) {
        if (*it == serial) {
            requests.erase(it);
            if (requests.empty()) g_requests.erase(found);
            return true;
        }
    }
    return false;
}

void forget_xwindows_synthetic_requests(Display* display)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    g_requests.erase(display);
}

} // namespace inputleap
