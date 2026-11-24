// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright OpenGlide Project

/**
 * @file PlanImport.hpp
 * @brief Download a flight plan from the OpenGlide server and load it.
 *
 * The pilot logs in to their OpenGlide account (stored in the profile),
 * and the most recent saved plan is downloaded as a .tsk file and
 * loaded into XCSoar's task engine. The thermal-forecast overlay
 * (RASP tiles) is downloaded alongside.
 *
 * This replaces the manual export → USB/Bluetooth → file-manager
 * workflow. The plan is cached for offline use.
 */

#pragma once

#include <string>

class ProtectedTaskManager;

namespace OpenGlide {

struct PlanImportConfig {
  std::string server_url = "https://api.openglide.org";
  std::string auth_token;
  std::string cache_dir;  // e.g. ~/.openglidenav/plans/
};

/**
 * Download the latest saved plan and load it as the active task.
 *
 * Returns true on success, false on network or parse error.
 * On failure, the current task is not modified.
 */
bool ImportLatestPlan(ProtectedTaskManager &task_manager,
                      const PlanImportConfig &config);

} // namespace OpenGlide
