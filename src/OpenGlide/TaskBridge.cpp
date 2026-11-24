// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright OpenGlide Project

#include "TaskBridge.hpp"

// XCSoar task engine
#include "Task/ProtectedTaskManager.hpp"
#include "Engine/Task/TaskManager.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Ordered/Points/AATPoint.hpp"
#include "Geo/GeoPoint.hpp"

namespace OpenGlide {

bool
ApplySuggestion(ProtectedTaskManager &task_manager,
                const Suggestion &suggestion)
{
  switch (suggestion.kind) {
  case Suggestion::Kind::AATTarget: {
    if (suggestion.sector_index < 1)
      return false;

    const GeoPoint target{
      Angle::Degrees(suggestion.target_lon),
      Angle::Degrees(suggestion.target_lat),
    };

    // The suggestion uses 1-based indexing; XCSoar uses 0-based.
    const unsigned tp_index =
      static_cast<unsigned>(suggestion.sector_index);

    ProtectedTaskManager::ExclusiveLease lease(task_manager);
    const OrderedTask &task = lease->GetOrderedTask();
    
    if (tp_index >= task.TaskSize())
      return false;

    AATPoint *ap = task.GetAATTaskPoint(tp_index);
    if (ap == nullptr)
      return false;

    // SetTarget with a GeoPoint directly
    ap->SetTarget(target, true /* override */);
    return true;
  }

  case Suggestion::Kind::Detour:
    // Phase 5: insert an intermediate waypoint.
    // Not yet implemented — the UI shows the warning but does not
    // modify the task.
    return false;

  case Suggestion::Kind::Reroute:
    // Phase 5+: replace the whole route.
    return false;

  case Suggestion::Kind::None:
    return false;
  }

  return false;
}

} // namespace OpenGlide
