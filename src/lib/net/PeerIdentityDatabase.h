/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2026 InputLeap contributors
 */

#pragma once

#include "inputleap/InputArbiter.h"
#include "net/FingerprintData.h"
#include "io/filesystem.h"

#include <iosfwd>
#include <unordered_map>

namespace inputleap {

/** Binds a stable peer node id to exactly one authenticated certificate. */
class PeerIdentityDatabase {
public:
    void read(const fs::path& path);
    void write(const fs::path& path) const;
    void read_stream(std::istream& stream);
    void write_stream(std::ostream& stream) const;

    void trust(PeerNodeId node_id, const FingerprintData& fingerprint);
    bool is_trusted(PeerNodeId node_id, const FingerprintData& fingerprint) const;
    const FingerprintData* fingerprint(PeerNodeId node_id) const;

private:
    std::unordered_map<PeerNodeId, FingerprintData> identities_;
};

} // namespace inputleap
