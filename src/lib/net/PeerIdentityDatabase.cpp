/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2026 InputLeap contributors
 */

#include "net/PeerIdentityDatabase.h"

#include "net/FingerprintDatabase.h"

#include <fstream>
#include <iomanip>
#include <sstream>

namespace inputleap {

void PeerIdentityDatabase::read(const fs::path& path)
{
    std::ifstream file;
    open_utf8_path(file, path);
    read_stream(file);
}

void PeerIdentityDatabase::write(const fs::path& path) const
{
    std::ofstream file;
    open_utf8_path(file, path, std::ios_base::out);
    write_stream(file);
}

void PeerIdentityDatabase::read_stream(std::istream& stream)
{
    if (!stream.good()) {
        return;
    }

    std::string line;
    while (std::getline(stream, line)) {
        const auto separator = line.find('\t');
        if (separator == std::string::npos) {
            continue;
        }
        std::istringstream node_stream(line.substr(0, separator));
        PeerNodeId node_id = 0;
        node_stream >> std::hex >> node_id;
        const auto parsed = FingerprintDatabase::parse_db_line(line.substr(separator + 1));
        if (!node_stream || node_id == 0 || !parsed.valid()) {
            continue;
        }
        identities_[node_id] = parsed;
    }
}

void PeerIdentityDatabase::write_stream(std::ostream& stream) const
{
    if (!stream.good()) {
        return;
    }
    for (const auto& identity : identities_) {
        stream << std::hex << identity.first << '\t'
               << FingerprintDatabase::to_db_line(identity.second) << '\n';
    }
}

void PeerIdentityDatabase::trust(PeerNodeId node_id, const FingerprintData& fingerprint)
{
    if (node_id != 0 && fingerprint.valid()) {
        identities_[node_id] = fingerprint;
    }
}

bool PeerIdentityDatabase::is_trusted(PeerNodeId node_id,
                                      const FingerprintData& fingerprint) const
{
    const auto* expected = this->fingerprint(node_id);
    return expected != nullptr && *expected == fingerprint;
}

const FingerprintData* PeerIdentityDatabase::fingerprint(PeerNodeId node_id) const
{
    const auto it = identities_.find(node_id);
    return it == identities_.end() ? nullptr : &it->second;
}

} // namespace inputleap
