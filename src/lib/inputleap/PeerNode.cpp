/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2026 InputLeap contributors
 */

#include "inputleap/PeerNode.h"

namespace inputleap {

namespace {
constexpr PeerCapabilities kRequiredSessionCapabilities =
    peer_capability(PeerCapability::Capture) |
    peer_capability(PeerCapability::Injection) |
    peer_capability(PeerCapability::MutualTls) |
    peer_capability(PeerCapability::SessionGeneration);
}

PeerNode::PeerNode(PeerNodeId node_id, PeerCapabilities capabilities,
                   IPeerInputSink& input_sink) :
    node_id_(node_id),
    capabilities_(capabilities),
    input_sink_(input_sink),
    arbiter_(node_id, [this](auto, auto, auto) { input_sink_.release_peer_input(); })
{
}

bool PeerNode::begin_control(PeerNode& peer)
{
    if (node_id_ == 0 || peer.node_id_ == 0 || node_id_ == peer.node_id_) {
        return false;
    }
    const auto negotiated = capabilities_ & peer.capabilities_;
    if ((negotiated & kRequiredSessionCapabilities) != kRequiredSessionCapabilities) {
        return false;
    }

    const auto session = session_generations_.next(peer.node_id_);
    if (!arbiter_.request_outbound(peer.node_id_, session)) {
        return false;
    }
    if (!peer.accept_control(node_id_, session, capabilities_)) {
        arbiter_.release(peer.node_id_, session);
        return false;
    }
    negotiated_capabilities_ = negotiated;
    return true;
}

bool PeerNode::send_input(PeerNode& peer, const PeerInputEvent& event)
{
    const auto session = arbiter_.active_session();
    return arbiter_.should_forward_captured_input(peer.node_id_, session) &&
           peer.receive_input(node_id_, session, event);
}

bool PeerNode::end_control(PeerNode& peer)
{
    const auto session = arbiter_.active_session();
    if (!arbiter_.should_forward_captured_input(peer.node_id_, session)) {
        return false;
    }
    const bool remote_released = peer.receive_release(node_id_, session);
    const bool local_released = arbiter_.release(peer.node_id_, session);
    negotiated_capabilities_ = 0;
    return remote_released && local_released;
}

void PeerNode::disconnect(PeerNode& peer)
{
    const auto session = arbiter_.active_session();
    if (session == 0 || arbiter_.active_peer() != peer.node_id_) {
        return;
    }
    peer.receive_disconnect(node_id_, session);
    arbiter_.disconnected(peer.node_id_, session);
    session_generations_.invalidate(peer.node_id_);
    negotiated_capabilities_ = 0;
}

bool PeerNode::accept_control(PeerNodeId peer, PeerSessionId session,
                              PeerCapabilities capabilities)
{
    const auto negotiated = capabilities_ & capabilities;
    if ((negotiated & kRequiredSessionCapabilities) != kRequiredSessionCapabilities ||
        !arbiter_.request_inbound(peer, session)) {
        return false;
    }
    negotiated_capabilities_ = negotiated;
    return true;
}

bool PeerNode::receive_input(PeerNodeId peer, PeerSessionId session,
                             const PeerInputEvent& event)
{
    if (!arbiter_.should_apply_remote_input(peer, session)) {
        return false;
    }
    input_sink_.apply_peer_input(event);
    return true;
}

bool PeerNode::receive_release(PeerNodeId peer, PeerSessionId session)
{
    const bool released = arbiter_.release(peer, session);
    if (released) {
        negotiated_capabilities_ = 0;
    }
    return released;
}

void PeerNode::receive_disconnect(PeerNodeId peer, PeerSessionId session)
{
    if (arbiter_.disconnected(peer, session)) {
        negotiated_capabilities_ = 0;
    }
}

} // namespace inputleap
