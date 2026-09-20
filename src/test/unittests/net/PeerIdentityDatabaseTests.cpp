#include "net/PeerIdentityDatabase.h"

#include <gtest/gtest.h>
#include <sstream>

namespace inputleap {

TEST(PeerIdentityDatabaseTests, bindsNodeIdToCertificateAcrossPersistence)
{
    FingerprintData fingerprint;
    fingerprint.algorithm = "sha256";
    fingerprint.data = {0x01, 0x23, 0x45, 0x67};

    PeerIdentityDatabase written;
    written.trust(0xabc, fingerprint);
    std::stringstream storage;
    written.write_stream(storage);

    PeerIdentityDatabase read;
    read.read_stream(storage);
    EXPECT_TRUE(read.is_trusted(0xabc, fingerprint));
    EXPECT_FALSE(read.is_trusted(0xdef, fingerprint));
}

} // namespace inputleap
