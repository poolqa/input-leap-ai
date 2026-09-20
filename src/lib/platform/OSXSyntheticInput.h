/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2026 InputLeap contributors
 */

#pragma once

#include <ApplicationServices/ApplicationServices.h>
#include <cstdint>

namespace inputleap {

// "INPUTLEP" encoded as an integer.  Quartz preserves kCGEventSourceUserData
// as an event passes through a HID event tap, allowing a hybrid screen to
// distinguish InputLeap injection from physical input without timing guesses.
constexpr std::int64_t kOSXSyntheticInputMarker = 0x494e5055544c4550LL;

inline void mark_osx_synthetic_input(CGEventRef event)
{
    if (event != nullptr) {
        CGEventSetIntegerValueField(event, kCGEventSourceUserData, kOSXSyntheticInputMarker);
    }
}

inline bool is_osx_synthetic_input(CGEventRef event)
{
    return event != nullptr &&
           CGEventGetIntegerValueField(event, kCGEventSourceUserData) ==
               kOSXSyntheticInputMarker;
}

// Capture callbacks must not forward events injected by this process back to
// a peer.  Keep this decision beside the marker helpers so the production
// event tap and its platform test exercise exactly the same rule.
inline bool should_forward_osx_captured_input(CGEventRef event)
{
    return event != nullptr && !is_osx_synthetic_input(event);
}

} // namespace inputleap
