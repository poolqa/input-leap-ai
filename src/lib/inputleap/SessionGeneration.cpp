/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2026 InputLeap contributors
 */

#include "inputleap/SessionGeneration.h"

#include <limits>

namespace inputleap {

PeerSessionId SessionGeneration::next(PeerNodeId peer)
{
    if (peer == 0) {
        return 0;
    }
    auto& generation = generations_[peer];
    if (generation == (std::numeric_limits<PeerSessionId>::max)()) {
        generation = 1;
    }
    else {
        ++generation;
    }
    return generation;
}

PeerSessionId SessionGeneration::current(PeerNodeId peer) const
{
    const auto it = generations_.find(peer);
    return it == generations_.end() ? 0 : it->second;
}

bool SessionGeneration::is_current(PeerNodeId peer, PeerSessionId session) const
{
    return session != 0 && current(peer) == session;
}

void SessionGeneration::invalidate(PeerNodeId peer)
{
    (void)next(peer);
}

} // namespace inputleap
