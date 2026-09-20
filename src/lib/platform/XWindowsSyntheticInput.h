/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2026 InputLeap contributors
 */

#pragma once

#include <X11/Xlib.h>

namespace inputleap {

// XTEST does not expose a user-data field.  The generated core event carries
// the serial of the XTEST request that created it, which gives a hybrid screen
// a deterministic per-Display loopback marker.
void mark_xwindows_synthetic_request(Display* display, unsigned long serial);
bool consume_xwindows_synthetic_event(Display* display, unsigned long serial);
void forget_xwindows_synthetic_requests(Display* display);

} // namespace inputleap
