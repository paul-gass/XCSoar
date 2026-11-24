// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright OpenGlide Project

/**
 * @file TaskBridge.hpp
 * @brief Bridge between OpenGlide suggestions and XCSoar's task engine.
 *
 * When the pilot accepts a suggestion, TaskBridge translates it into
 * the XCSoar task engine's API:
 *
 * - AAT target move: calls ProtectedTaskManager::SetTarget() on the
 *   sector's observation zone.
 * - Detour: inserts an intermediate waypoint into the ordered task.
 * - Reroute: replaces the task with a new set of waypoints.
 *
 * None of these happen without an explicit pilot confirmation.
 */

#pragma once

#include "LiveClient.hpp"

class ProtectedTaskManager;

namespace OpenGlide {

/**
 * Apply an accepted suggestion to the active task.
 *
 * Returns true if the task was modified, false if the suggestion
 * could not be applied (e.g. the task has changed since the
 * suggestion was made).
 */
bool ApplySuggestion(ProtectedTaskManager &task_manager,
                     const Suggestion &suggestion);

} // namespace OpenGlide
