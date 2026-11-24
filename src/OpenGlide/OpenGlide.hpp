// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright OpenGlide Project

#pragma once

/**
 * @file OpenGlide.hpp
 * @brief Main header for the OpenGlide live-routing extension.
 *
 * All OpenGlide code lives in src/OpenGlide/. It hooks into XCSoar
 * through a small number of well-defined integration points:
 *
 * 1. **LiveClient** — background HTTP client that polls the OpenGlide
 *    server every 1–2 minutes with the glider's state and receives
 *    route suggestions and weather polygons.
 *
 * 2. **SuggestionUI** — draws the dashed suggestion line on the map,
 *    shows the banner with the reason, and handles the accept tap.
 *
 * 3. **WeatherLayers** — map overlays for rain radar, storm cells,
 *    cumulus fields, and OGN thermal markers.
 *
 * 4. **TaskBridge** — sets AAT target points or inserts detour
 *    waypoints in the XCSoar task engine when a suggestion is accepted.
 *
 * 5. **PlanImport** — downloads a saved flight plan from the OpenGlide
 *    server (account login) and loads it as the active task.
 *
 * The XCSoar core (task engine, map renderer, airspace, FLARM, logger)
 * is NOT modified. Integration uses existing public APIs where possible
 * and a minimal set of new entry points added to the XCSoar source
 * (each marked with "// OpenGlide hook" in the diff).
 *
 * No audio — the pilot decides, the app does not speak.
 */

// Forward declarations (global namespace)
class Canvas;
class WindowProjection;

namespace OpenGlide {

/**
 * Initialise OpenGlide services. Called once from XCSoar startup
 * after the profile and task are loaded.
 */
void Initialise();

/**
 * Shut down background threads and free resources.
 * Called from XCSoar's shutdown path.
 */
void Deinitialise();

/**
 * True when the live client has an active server connection.
 */
[[nodiscard]]
bool IsConnected() noexcept;

/**
 * True when a suggestion is pending (not yet accepted or dismissed).
 */
[[nodiscard]]
bool HasPendingSuggestion() noexcept;

/**
 * Accept the current suggestion: apply it to the task engine.
 */
void AcceptSuggestion();

/**
 * Dismiss the current suggestion without applying it.
 */
void DismissSuggestion();

/**
 * Called by the map renderer on each redraw to update overlays and
 * check for new suggestions.
 */
void OnMapTick();

/**
 * Called by MapWindow::DrawOverlay() — renders weather + suggestion.
 */
void DrawOverlays(Canvas &canvas, const WindowProjection &projection);

} // namespace OpenGlide
