/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2026 InputLeap contributors
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace inputleap {

// Passed through dwExtraInfo by SendInput/keybd_event/mouse_event so a
// concurrently capturing hybrid screen never forwards its own injection.
constexpr ULONG_PTR kMSWindowsSyntheticInputMarker =
    static_cast<ULONG_PTR>(0x494c5045u); // "ILPE"

} // namespace inputleap
