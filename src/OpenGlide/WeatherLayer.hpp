// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright OpenGlide Project

/**
 * @file WeatherLayer.hpp
 * @brief Map overlay renderer for live weather data.
 *
 * Draws the following layers on XCSoar's map canvas:
 *
 * - Rain radar polygons (semi-transparent blue/purple)
 * - Storm cells with safety buffer (red hatched)
 * - Cumulus fields (subtle green tint)
 * - OGN thermal markers (small rising-arrow icons)
 * - Suggestion line (dashed yellow/green)
 *
 * Each layer can be toggled independently. Stale data (older than
 * the configured threshold) is drawn greyed out with an age label.
 *
 * The renderer reads from the LiveClient's latest ServerResponse
 * and does not make network calls or block.
 */

#pragma once

#include "LiveClient.hpp"

class Canvas;
class WindowProjection;

namespace OpenGlide {

/**
 * Render weather polygons and thermal markers on the map.
 *
 * Called from XCSoar's MapWindow::DrawOverlay() path.
 */
void DrawWeatherOverlay(Canvas &canvas,
                        const WindowProjection &projection,
                        const ServerResponse &response);

/**
 * Render the suggestion line (dashed) and accept banner.
 *
 * Called from XCSoar's MapWindow::DrawOverlay() path.
 */
void DrawSuggestionOverlay(Canvas &canvas,
                           const WindowProjection &projection,
                           const Suggestion &suggestion);

} // namespace OpenGlide
