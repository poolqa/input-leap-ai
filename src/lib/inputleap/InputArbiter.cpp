/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2026 InputLeap contributors
 */

#include "inputleap/InputArbiter.h"

#include <utility>

namespace inputleap {

InputArbiter::InputArbiter(PeerNodeId local_node_id, ReleaseHandler release_handler) :
    local_node_id_(local_node_id),
    release_handler_(std::move(release_handler))
{
}

bool InputArbiter::request_outbound(PeerNodeId peer, PeerSessionId session)
{
    if (peer == 0 || session == 0 || state_ != PeerInputState::Local) {
        return false;
    }
    set_active(PeerInputState::Controlling, peer, session);
    return true;
}

bool InputArbiter::request_inbound(PeerNodeId peer, PeerSessionId session)
{
    if (peer == 0 || session == 0) {
        return false;
    }

    if (state_ == PeerInputState::Local) {
        set_active(PeerInputState::ControlledBy, peer, session);
        return true;
    }

    if (state_ == PeerInputState::ControlledBy) {
        if (active_peer_ != peer || session <= active_session_) {
            return false;
        }
        release_active(InputReleaseReason::ReplacedSession);
        set_active(PeerInputState::ControlledBy, peer, session);
        return true;
    }

    // Simultaneous crossings are resolved from stable node ids.  The lower id
    // stays controller, so both peers make the same decision independently.
    if (active_peer_ == peer && local_node_id_ > peer) {
        set_active(PeerInputState::ControlledBy, peer, session);
        return true;
    }

    return false;
}

bool InputArbiter::release(PeerNodeId peer, PeerSessionId session)
{
    if (state_ == PeerInputState::Local || active_peer_ != peer || active_session_ != session) {
        return false;
    }
    return release_active(InputReleaseReason::Explicit);
}

bool InputArbiter::disconnected(PeerNodeId peer, PeerSessionId session)
{
    if (state_ == PeerInputState::Local || active_peer_ != peer || active_session_ != session) {
        return false;
    }
    return release_active(InputReleaseReason::Disconnected);
}

bool InputArbiter::should_forward_captured_input(PeerNodeId peer, PeerSessionId session) const
{
    return state_ == PeerInputState::Controlling && active_peer_ == peer &&
           active_session_ == session;
}

bool InputArbiter::should_apply_remote_input(PeerNodeId peer, PeerSessionId session) const
{
    return state_ == PeerInputState::ControlledBy && active_peer_ == peer &&
           active_session_ == session;
}

bool InputArbiter::release_active(InputReleaseReason reason)
{
    const auto peer = active_peer_;
    const auto session = active_session_;
    const bool injected_input_may_be_held = state_ == PeerInputState::ControlledBy;
    set_local();
    if (injected_input_may_be_held && release_handler_) {
        release_handler_(peer, session, reason);
    }
    return true;
}

void InputArbiter::set_local()
{
    state_ = PeerInputState::Local;
    active_peer_ = 0;
    active_session_ = 0;
}

void InputArbiter::set_active(PeerInputState state, PeerNodeId peer, PeerSessionId session)
{
    state_ = state;
    active_peer_ = peer;
    active_session_ = session;
}

} // namespace inputleap
