/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2026 InputLeap contributors
 */

#pragma once

#include <cstdint>
#include <functional>

namespace inputleap {

using PeerNodeId = std::uint64_t;
using PeerSessionId = std::uint64_t;

enum class PeerInputState {
    Local,
    Controlling,
    ControlledBy
};

enum class InputReleaseReason {
    Explicit,
    ReplacedSession,
    Disconnected
};

/**
 * Arbitrates ownership of local input in peer mode.
 *
 * A session is deliberately part of every transition.  This prevents delayed
 * packets (or a disconnect from an old socket) from changing ownership after a
 * peer has reconnected.  The release handler is invoked whenever injected
 * input may still be held, allowing the platform layer to synthesize key and
 * button-up events before returning to Local.
 */
class InputArbiter {
public:
    using ReleaseHandler = std::function<void(PeerNodeId, PeerSessionId,
                                               InputReleaseReason)>;

    explicit InputArbiter(PeerNodeId local_node_id,
                          ReleaseHandler release_handler = {});

    bool request_outbound(PeerNodeId peer, PeerSessionId session);
    bool request_inbound(PeerNodeId peer, PeerSessionId session);
    bool release(PeerNodeId peer, PeerSessionId session);
    bool disconnected(PeerNodeId peer, PeerSessionId session);

    bool should_forward_captured_input(PeerNodeId peer, PeerSessionId session) const;
    bool should_apply_remote_input(PeerNodeId peer, PeerSessionId session) const;

    PeerInputState state() const { return state_; }
    PeerNodeId active_peer() const { return active_peer_; }
    PeerSessionId active_session() const { return active_session_; }

private:
    bool release_active(InputReleaseReason reason);
    void set_local();
    void set_active(PeerInputState state, PeerNodeId peer, PeerSessionId session);

    PeerNodeId local_node_id_;
    ReleaseHandler release_handler_;
    PeerInputState state_{PeerInputState::Local};
    PeerNodeId active_peer_{0};
    PeerSessionId active_session_{0};
};

} // namespace inputleap
