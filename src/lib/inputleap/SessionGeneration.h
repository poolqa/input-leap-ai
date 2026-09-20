/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2026 InputLeap contributors
 */

#pragma once

#include "inputleap/InputArbiter.h"

#include <unordered_map>

namespace inputleap {

/** Allocates monotonically increasing, non-zero generations per peer. */
class SessionGeneration {
public:
    PeerSessionId next(PeerNodeId peer);
    PeerSessionId current(PeerNodeId peer) const;
    bool is_current(PeerNodeId peer, PeerSessionId session) const;
    void invalidate(PeerNodeId peer);

private:
    std::unordered_map<PeerNodeId, PeerSessionId> generations_;
};

} // namespace inputleap
