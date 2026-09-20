/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2026 InputLeap contributors
 */

#pragma once

#include "inputleap/InputArbiter.h"
#include "inputleap/PeerConfig.h"
#include "inputleap/SessionGeneration.h"

#include <cstdint>

namespace inputleap {

struct PeerInputEvent {
    enum class Type { Key, Button, Pointer };

    Type type{Type::Pointer};
    std::int32_t value1{0};
    std::int32_t value2{0};
};

class IPeerInputSink {
public:
    virtual ~IPeerInputSink() = default;
    virtual void apply_peer_input(const PeerInputEvent& event) = 0;
    virtual void release_peer_input() = 0;
};

/**
 * Transport-independent peer session endpoint.
 *
 * Network transports feed begin/input/end/disconnect into this class. Keeping
 * arbitration here makes the same session-generation and disconnect-release
 * rules testable without a window system or live sockets.
 */
class PeerNode {
public:
    PeerNode(PeerNodeId node_id, PeerCapabilities capabilities,
             IPeerInputSink& input_sink);

    bool begin_control(PeerNode& peer);
    bool send_input(PeerNode& peer, const PeerInputEvent& event);
    bool end_control(PeerNode& peer);
    void disconnect(PeerNode& peer);

    PeerNodeId node_id() const { return node_id_; }
    PeerSessionId active_session() const { return arbiter_.active_session(); }
    PeerInputState input_state() const { return arbiter_.state(); }
    PeerCapabilities negotiated_capabilities() const { return negotiated_capabilities_; }

private:
    bool accept_control(PeerNodeId peer, PeerSessionId session,
                        PeerCapabilities capabilities);
    bool receive_input(PeerNodeId peer, PeerSessionId session,
                       const PeerInputEvent& event);
    bool receive_release(PeerNodeId peer, PeerSessionId session);
    void receive_disconnect(PeerNodeId peer, PeerSessionId session);

    PeerNodeId node_id_;
    PeerCapabilities capabilities_;
    IPeerInputSink& input_sink_;
    InputArbiter arbiter_;
    SessionGeneration session_generations_;
    PeerCapabilities negotiated_capabilities_{0};
};

} // namespace inputleap
