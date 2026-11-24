// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <concepts>

#if defined(__clang__) && __has_warning("-Wnan-infinity-disabled")
/* Clang warns about __builtin_isfinite() under -ffast-math. The
   warning group exists only on newer Clang; older NDKs (Clang 18)
   reject the pragma under -Werror. Keep the check as a sanity test. */
#pragma GCC diagnostic ignored "-Wnan-infinity-disabled"
#endif

/**
 * A constexpr wrapper for std::isfinite().  This uses the
 * non-standard __builtin_isfinite() function (specific to GCC and
 * clang) because the C++ standard library is not "constexpr".
 */
constexpr bool
IsFinite(std::floating_point auto value) noexcept
{
  return __builtin_isfinite(value);
}
