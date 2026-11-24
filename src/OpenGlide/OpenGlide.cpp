// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright OpenGlide Project

#include "OpenGlide.hpp"
#include "LiveClient.hpp"
#include "TaskBridge.hpp"
#include "WeatherLayer.hpp"

#include "LogFile.hpp"

#include <memory>
#include <mutex>
#include <optional>

namespace OpenGlide {

static std::unique_ptr<LiveClient> live_client;
static std::mutex suggestion_mutex;
static std::optional<Suggestion> pending_suggestion;
static ServerResponse latest_response;

void
Initialise()
{
  LogFormat("OpenGlide: initialising live routing client");

  // Load config from the profile (server URL, auth token).
  // For now, hardcoded defaults — the real implementation reads
  // from the XCSoar profile file.
  ClientConfig config;
  config.server_url = "https://api.openglide.org";
  config.poll_interval_s = 90;  // 1.5 minutes
  config.timeout_s = 10;

  live_client = std::make_unique<LiveClient>(config);
  live_client->Start();

  LogFormat("OpenGlide: live client started, polling every %.0f s",
            config.poll_interval_s);
}

void
Deinitialise()
{
  LogFormat("OpenGlide: shutting down");
  if (live_client) {
    live_client->Stop();
    live_client.reset();
  }
  {
    std::lock_guard lock(suggestion_mutex);
    pending_suggestion.reset();
  }
}

bool
IsConnected() noexcept
{
  return live_client && live_client->IsConnected();
}

bool
HasPendingSuggestion() noexcept
{
  std::lock_guard lock(suggestion_mutex);
  return pending_suggestion.has_value();
}

void
AcceptSuggestion()
{
  std::lock_guard lock(suggestion_mutex);
  if (!pending_suggestion.has_value())
    return;

  // The TaskBridge applies the suggestion to XCSoar's task engine.
  // This requires the ProtectedTaskManager, which is passed through
  // the global CommonInterface in the real integration:
  //
  //   auto &task_mgr = CommonInterface::GetComputerSettings()...;
  //   ApplySuggestion(task_mgr, *pending_suggestion);

  LogFormat("OpenGlide: suggestion accepted — %s",
            pending_suggestion->reason.c_str());
  pending_suggestion.reset();
}

void
DismissSuggestion()
{
  std::lock_guard lock(suggestion_mutex);
  if (pending_suggestion.has_value()) {
    LogFormat("OpenGlide: suggestion dismissed — %s",
              pending_suggestion->reason.c_str());
    pending_suggestion.reset();
  }
}

// Called by the map renderer on each redraw to update overlays and
// check for new suggestions.
void
OnMapTick()
{
  if (!live_client)
    return;

  latest_response = live_client->GetLatest();

  if (latest_response.valid && latest_response.suggestion.has_value()) {
    std::lock_guard lock(suggestion_mutex);
    if (!pending_suggestion.has_value()) {
      pending_suggestion = latest_response.suggestion;
      LogFormat("OpenGlide: new suggestion — %s (gain %.0f s)",
                pending_suggestion->reason.c_str(),
                pending_suggestion->gain_s);
    }
  }
}

// Called by MapWindow::DrawOverlay() — renders weather + suggestion.
void
DrawOverlays(Canvas &canvas, const WindowProjection &projection)
{
  DrawWeatherOverlay(canvas, projection, latest_response);

  std::lock_guard lock(suggestion_mutex);
  if (pending_suggestion.has_value())
    DrawSuggestionOverlay(canvas, projection, *pending_suggestion);
}

} // namespace OpenGlide
