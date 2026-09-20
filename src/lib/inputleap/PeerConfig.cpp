/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2026 InputLeap contributors
 */

#include "inputleap/PeerConfig.h"

namespace inputleap {

bool PeerConfig::is_valid() const
{
    return node_id != 0 && !display_name.empty() && !host.empty() && port != 0 &&
           !certificate_fingerprint.empty() &&
           has_peer_capability(capabilities, PeerCapability::MutualTls) &&
           has_peer_capability(capabilities, PeerCapability::SessionGeneration);
}

bool PeerConfig::may_control_local() const
{
    return direction != PeerDirection::ControlOnly &&
           has_peer_capability(capabilities, PeerCapability::Capture);
}

bool PeerConfig::may_be_controlled() const
{
    return direction != PeerDirection::ReceiveOnly &&
           has_peer_capability(capabilities, PeerCapability::Injection);
}

} // namespace inputleap
