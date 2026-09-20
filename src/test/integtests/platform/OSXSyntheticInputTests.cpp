#include "platform/OSXSyntheticInput.h"

#include <gtest/gtest.h>

namespace inputleap {

TEST(OSXSyntheticInputTests, markerSurvivesMouseEventFields)
{
    CGEventRef event = CGEventCreateMouseEvent(
        nullptr, kCGEventMouseMoved, CGPointMake(10.0, 20.0), kCGMouseButtonLeft);
    ASSERT_NE(nullptr, event);

    EXPECT_FALSE(is_osx_synthetic_input(event));
    mark_osx_synthetic_input(event);
    EXPECT_TRUE(is_osx_synthetic_input(event));

    CFRelease(event);
}

TEST(OSXSyntheticInputTests, markerSurvivesKeyboardEventFields)
{
    CGEventRef event = CGEventCreateKeyboardEvent(nullptr, 0, true);
    ASSERT_NE(nullptr, event);

    mark_osx_synthetic_input(event);
    EXPECT_TRUE(is_osx_synthetic_input(event));

    CFRelease(event);
}

TEST(OSXSyntheticInputTests, markerSurvivesScrollEventFields)
{
    CGEventRef event = CGEventCreateScrollWheelEvent(nullptr, kCGScrollEventUnitLine, 2, 1, 0);
    ASSERT_NE(nullptr, event);

    mark_osx_synthetic_input(event);
    EXPECT_TRUE(is_osx_synthetic_input(event));

    CFRelease(event);
}

TEST(OSXSyntheticInputTests, captureForwardsPhysicalButNotInjectedInput)
{
    CGEventRef physicalEvent = CGEventCreateMouseEvent(
        nullptr, kCGEventMouseMoved, CGPointMake(10.0, 20.0), kCGMouseButtonLeft);
    CGEventRef injectedEvent = CGEventCreateMouseEvent(
        nullptr, kCGEventMouseMoved, CGPointMake(30.0, 40.0), kCGMouseButtonLeft);
    ASSERT_NE(nullptr, physicalEvent);
    ASSERT_NE(nullptr, injectedEvent);

    mark_osx_synthetic_input(injectedEvent);

    EXPECT_TRUE(should_forward_osx_captured_input(physicalEvent));
    EXPECT_FALSE(should_forward_osx_captured_input(injectedEvent));
    EXPECT_FALSE(should_forward_osx_captured_input(nullptr));

    CFRelease(injectedEvent);
    CFRelease(physicalEvent);
}

} // namespace inputleap
