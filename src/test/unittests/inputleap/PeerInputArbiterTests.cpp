#include "inputleap/PeerInputArbiter.h"
#include "inputleap/PeerConfig.h"
#include "inputleap/PeerNode.h"
#include "inputleap/SessionGeneration.h"

#include <gtest/gtest.h>

#include <vector>

namespace inputleap {

namespace {
class DummyPeerScreen : public IPeerInputSink {
public:
    void apply_peer_input(const PeerInputEvent& event) override
    {
        events.push_back(event);
    }

    void release_peer_input() override { ++release_count; }

    std::vector<PeerInputEvent> events;
    int release_count{0};
};
}

TEST(PeerNodeTests, twoDummyScreensSupportBidirectionalSessionsWithoutLoopback)
{
    DummyPeerScreen screen_a;
    DummyPeerScreen screen_b;
    PeerNode node_a(10, kDefaultPeerCapabilities, screen_a);
    PeerNode node_b(20, kDefaultPeerCapabilities, screen_b);

    ASSERT_TRUE(node_a.begin_control(node_b));
    EXPECT_TRUE(node_a.send_input(node_b, {PeerInputEvent::Type::Pointer, 4, -2}));
    EXPECT_TRUE(screen_a.events.empty());
    ASSERT_EQ(1u, screen_b.events.size());
    EXPECT_EQ(4, screen_b.events.front().value1);
    EXPECT_TRUE(node_a.end_control(node_b));
    EXPECT_EQ(1, screen_b.release_count);

    ASSERT_TRUE(node_b.begin_control(node_a));
    EXPECT_TRUE(node_b.send_input(node_a, {PeerInputEvent::Type::Key, 42, 1}));
    EXPECT_EQ(1u, screen_a.events.size());
    EXPECT_EQ(1u, screen_b.events.size());

    node_b.disconnect(node_a);
    EXPECT_EQ(PeerInputState::Local, node_a.input_state());
    EXPECT_EQ(PeerInputState::Local, node_b.input_state());
    EXPECT_EQ(1, screen_a.release_count);
}

TEST(PeerInputArbiterTests, forwardsOnlyTheCurrentOutboundSession)
{
    PeerInputArbiter arbiter(10);

    EXPECT_TRUE(arbiter.request_outbound(20, 7));
    EXPECT_TRUE(arbiter.should_forward_captured_input(20, 7));
    EXPECT_FALSE(arbiter.should_forward_captured_input(20, 6));
    EXPECT_FALSE(arbiter.should_apply_remote_input(20, 7));
}

TEST(PeerInputArbiterTests, ignoresStaleInboundSessions)
{
    PeerInputArbiter arbiter(10);

    EXPECT_TRUE(arbiter.request_inbound(20, 7));
    EXPECT_TRUE(arbiter.request_inbound(20, 8));
    EXPECT_FALSE(arbiter.request_inbound(20, 7));
    EXPECT_FALSE(arbiter.release(20, 7));
    EXPECT_TRUE(arbiter.should_apply_remote_input(20, 8));
}

TEST(PeerInputArbiterTests, resolvesSimultaneousCrossingByStableNodeId)
{
    PeerInputArbiter lower_id(10);
    PeerInputArbiter higher_id(20);

    ASSERT_TRUE(lower_id.request_outbound(20, 1));
    ASSERT_TRUE(higher_id.request_outbound(10, 1));

    EXPECT_FALSE(lower_id.request_inbound(20, 2));
    EXPECT_TRUE(higher_id.request_inbound(10, 2));
    EXPECT_EQ(PeerInputState::Controlling, lower_id.state());
    EXPECT_EQ(PeerInputState::ControlledBy, higher_id.state());
}

TEST(PeerInputArbiterTests, disconnectReleasesOnlyMatchingSession)
{
    PeerNodeId released_peer = 0;
    PeerSessionId released_session = 0;
    InputReleaseReason released_reason = InputReleaseReason::Explicit;
    PeerInputArbiter arbiter(10, [&](auto peer, auto session, auto reason) {
        released_peer = peer;
        released_session = session;
        released_reason = reason;
    });

    ASSERT_TRUE(arbiter.request_inbound(20, 9));
    EXPECT_FALSE(arbiter.disconnected(20, 8));
    EXPECT_EQ(PeerInputState::ControlledBy, arbiter.state());
    EXPECT_TRUE(arbiter.disconnected(20, 9));
    EXPECT_EQ(PeerInputState::Local, arbiter.state());
    EXPECT_EQ(20u, released_peer);
    EXPECT_EQ(9u, released_session);
    EXPECT_EQ(InputReleaseReason::Disconnected, released_reason);
}

TEST(PeerInputArbiterTests, replacingInboundSessionReleasesInjectedInput)
{
    int releases = 0;
    InputReleaseReason reason = InputReleaseReason::Explicit;
    InputArbiter arbiter(10, [&](auto, auto, auto release_reason) {
        ++releases;
        reason = release_reason;
    });

    ASSERT_TRUE(arbiter.request_inbound(20, 4));
    ASSERT_TRUE(arbiter.request_inbound(20, 5));
    EXPECT_EQ(1, releases);
    EXPECT_EQ(InputReleaseReason::ReplacedSession, reason);
    EXPECT_TRUE(arbiter.should_apply_remote_input(20, 5));
}

TEST(SessionGenerationTests, advancesIndependentlyAndInvalidatesStaleConnections)
{
    SessionGeneration generations;

    EXPECT_EQ(1u, generations.next(20));
    EXPECT_EQ(1u, generations.next(30));
    EXPECT_EQ(2u, generations.next(20));
    EXPECT_TRUE(generations.is_current(20, 2));
    EXPECT_FALSE(generations.is_current(20, 1));

    generations.invalidate(20);
    EXPECT_EQ(3u, generations.current(20));
    EXPECT_FALSE(generations.is_current(20, 2));
}

TEST(PeerConfigTests, capabilityTextRoundTripsForBonjourTxtRecords)
{
    const auto expected = kDefaultPeerCapabilities |
        peer_capability(PeerCapability::DragDrop);
    PeerCapabilities actual = 0;

    EXPECT_TRUE(peer_capabilities_from_string(peer_capabilities_to_string(expected), actual));
    EXPECT_EQ(expected, actual);
    EXPECT_FALSE(peer_capabilities_from_string("not-hex", actual));
}

TEST(PeerConfigTests, bidirectionalPeerRequiresIdentityAndSafetyCapabilities)
{
    PeerConfig peer;
    peer.node_id = 42;
    peer.display_name = "work-mac";
    peer.host = "work-mac.local";
    peer.certificate_fingerprint = "sha256:example";
    peer.capabilities = kDefaultPeerCapabilities;

    EXPECT_TRUE(peer.is_valid());
    EXPECT_TRUE(peer.may_control_local());
    EXPECT_TRUE(peer.may_be_controlled());
}

} // namespace inputleap
