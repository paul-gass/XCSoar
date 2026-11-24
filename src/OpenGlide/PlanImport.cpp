// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright OpenGlide Project

#include "PlanImport.hpp"

#include "net/http/Easy.hxx"
#include "Task/ProtectedTaskManager.hpp"
#include "Task/TaskFile.hpp"
#include "system/Path.hpp"
#include "io/FileOutputStream.hxx"
#include "LogFile.hpp"

#include <filesystem>

namespace OpenGlide {

bool
ImportLatestPlan(ProtectedTaskManager &task_manager,
                 const PlanImportConfig &config)
{
  if (config.auth_token.empty()) {
    LogFormat("OpenGlide: no auth token, skipping plan import");
    return false;
  }

  // 1. Download the latest plan's .tsk from the server.
  const std::string url = config.server_url + "/api/plans/latest";

  // This is the structure — the actual HTTP download will use
  // XCSoar's Net::Session with the auth token in the header:
  //
  //   GET /api/plans/latest
  //   Authorization: Bearer <token>
  //
  // Response: JSON with { plan: { id, result, ... } }
  // Then:
  //   GET /api/plans/<id>/tsk
  //   → .tsk XML file
  //
  //   GET /api/plans/<id>/export
  //   → ZIP with .tsk + .cup + RASP overlay

  // 2. Write the .tsk to the cache directory.
  std::filesystem::path cache_dir(config.cache_dir);
  std::filesystem::create_directories(cache_dir);
  auto tsk_path = cache_dir / "latest_plan.tsk";

  // Placeholder: in the real implementation, the HTTP response body
  // is written to tsk_path and then loaded into the task engine.

  // 3. Load the task into XCSoar's task engine.
  // TaskFile::GetTask() reads a .tsk and returns an OrderedTask.
  // ProtectedTaskManager::TaskCommit() makes it the active task.

  LogFormat("OpenGlide: plan import from %s (placeholder)", url.c_str());

  // The actual implementation will be:
  //
  //   auto task = TaskFile::GetTask(Path(tsk_path.c_str()), ...);
  //   if (task) {
  //     ProtectedTaskManager::ExclusiveLease lease(task_manager);
  //     lease->Commit(*task);
  //     return true;
  //   }

  return false;
}

} // namespace OpenGlide
