/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2026 InputLeap contributors
 */

#pragma once

#include "inputleap/InputArbiter.h"

#include <cstdint>
#include <cerrno>
#include <cstdlib>
#include <limits>
#include <sstream>
#include <string>

namespace inputleap {

enum class PeerDirection {
    Bidirectional,
    ControlOnly,
    ReceiveOnly
};

enum class PeerCapability : std::uint32_t {
    Capture = 1u << 0,
    Injection = 1u << 1,
    Clipboard = 1u << 2,
    DragDrop = 1u << 3,
    RelativePointer = 1u << 4,
    MutualTls = 1u << 5,
    SessionGeneration = 1u << 6
};

using PeerCapabilities = std::uint32_t;

constexpr PeerCapabilities peer_capability(PeerCapability capability)
{
    return static_cast<PeerCapabilities>(capability);
}

constexpr bool has_peer_capability(PeerCapabilities capabilities, PeerCapability capability)
{
    return (capabilities & peer_capability(capability)) != 0;
}

constexpr PeerCapabilities kDefaultPeerCapabilities =
    peer_capability(PeerCapability::Capture) |
    peer_capability(PeerCapability::Injection) |
    peer_capability(PeerCapability::Clipboard) |
    peer_capability(PeerCapability::RelativePointer) |
    peer_capability(PeerCapability::MutualTls) |
    peer_capability(PeerCapability::SessionGeneration);

struct PeerConfig {
    PeerNodeId node_id{0};
    std::string display_name;
    std::string host;
    std::uint16_t port{24800};
    std::string certificate_fingerprint;
    PeerCapabilities capabilities{0};
    PeerDirection direction{PeerDirection::Bidirectional};
    bool enabled{true};

    bool is_valid() const;
    bool may_control_local() const;
    bool may_be_controlled() const;
};

inline std::string peer_capabilities_to_string(PeerCapabilities capabilities)
{
    std::ostringstream stream;
    stream << std::hex << std::nouppercase << capabilities;
    return stream.str();
}

inline bool peer_capabilities_from_string(const std::string& text,
                                          PeerCapabilities& capabilities)
{
    if (text.empty() || text.front() == '-') {
        return false;
    }
    char* end = nullptr;
    errno = 0;
    const auto value = std::strtoull(text.c_str(), &end, 16);
    if (errno != 0 || end == text.c_str() || *end != '\0' ||
        value > std::numeric_limits<PeerCapabilities>::max()) {
        return false;
    }
    capabilities = static_cast<PeerCapabilities>(value);
    return true;
}

} // namespace inputleap
