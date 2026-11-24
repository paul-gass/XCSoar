// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <concepts>

#ifdef __clang__
/* GCC still tokenizes the skipped branch of a one-line #if, so
   __has_warning() has to live in its own directive. The warning
   group exists only on newer Clang. */
#  if __has_warning("-Wnan-infinity-disabled")
#    pragma GCC diagnostic ignored "-Wnan-infinity-disabled"
#  endif
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
