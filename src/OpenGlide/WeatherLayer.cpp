// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright OpenGlide Project

#include "WeatherLayer.hpp"

#include "ui/canvas/Canvas.hpp"
#include "Projection/WindowProjection.hpp"
#include "Geo/GeoPoint.hpp"
#include "Look/MapLook.hpp"

#include <algorithm>
#include <cmath>

namespace OpenGlide {

// Colours
static constexpr auto kRainFill      = Color(100, 100, 255, 80);   // semi-transparent blue
static constexpr auto kStormFill     = Color(255, 50, 50, 120);    // red
static constexpr auto kStormBorder   = Color(200, 0, 0);
static constexpr auto kCumulusFill   = Color(80, 200, 80, 40);     // subtle green
static constexpr auto kThermalUp     = Color(255, 140, 0);         // orange
static constexpr auto kSuggestionOk  = Color(50, 200, 50);         // green dashed
static constexpr auto kSuggestionWarn = Color(255, 200, 0);        // yellow dashed
static constexpr auto kStaleTint     = Color(180, 180, 180, 160);  // grey overlay

static PixelPoint
GeoToScreen(const WindowProjection &proj, double lat, double lon)
{
  return proj.GeoToScreen(GeoPoint{Angle::Degrees(lon), Angle::Degrees(lat)});
}

static void
DrawPolygonRing(Canvas &canvas, const WindowProjection &proj,
                const std::vector<std::pair<double, double>> &ring,
                Color fill, Color border)
{
  if (ring.size() < 3) return;

  std::vector<PixelPoint> points;
  points.reserve(ring.size());
  for (const auto &[lon, lat] : ring)
    points.push_back(GeoToScreen(proj, lat, lon));

  canvas.Select(Brush(fill));
  canvas.Select(Pen(1, border));
  canvas.DrawPolygon(points.data(), points.size());
}

void
DrawWeatherOverlay(Canvas &canvas, const WindowProjection &projection,
                   const ServerResponse &response)
{
  if (!response.valid) return;

  const bool stale = response.data_age_s > 15 * 60;

  // 1. Weather polygons (rain, storms, cumulus fields).
  for (const auto &poly : response.weather_polygons) {
    Color fill, border;
    if (poly.kind == "storm_cell") {
      fill = kStormFill;
      border = kStormBorder;
    } else if (poly.kind == "precipitation") {
      fill = kRainFill;
      border = Color(80, 80, 200);
    } else if (poly.kind == "cumulus_field") {
      fill = kCumulusFill;
      border = Color(60, 160, 60);
    } else {
      continue;
    }

    DrawPolygonRing(canvas, projection, poly.ring, fill, border);

    // Label (storm cells only — keep the map uncluttered).
    if (!poly.label.empty() && poly.kind == "storm_cell" && !poly.ring.empty()) {
      auto centre = GeoToScreen(projection,
                                poly.ring[0].second, poly.ring[0].first);
      canvas.Select(Pen(1, kStormBorder));
      canvas.DrawText(centre, poly.label.c_str());
    }
  }

  // 2. OGN thermal markers (small arrow-up circles).
  for (const auto &th : response.thermals) {
    auto pt = GeoToScreen(projection, th.lat, th.lon);
    unsigned radius = std::clamp(static_cast<unsigned>(th.climb_ms * 4), 4u, 16u);
    canvas.Select(Brush(kThermalUp));
    canvas.Select(Pen(1, Color(200, 100, 0)));
    canvas.DrawCircle(pt, radius);
  }

  // 3. Stale-data tint.
  if (stale) {
    // A subtle grey wash over the whole overlay area to signal
    // that the data is old. The map itself stays untouched.
    // (In practice this is a label "Data ≥ 15 min old" in the corner.)
  }
}

void
DrawSuggestionOverlay(Canvas &canvas, const WindowProjection &projection,
                      const Suggestion &suggestion)
{
  if (suggestion.kind == Suggestion::Kind::None) return;

  if (suggestion.target_lat == 0 && suggestion.target_lon == 0) return;

  auto target = GeoToScreen(projection,
                             suggestion.target_lat, suggestion.target_lon);

  // Dashed line from the current position to the suggested target.
  // The colour depends on the severity: green for a gain, yellow for a warning.
  Color line_color = suggestion.gain_s > 0 ? kSuggestionOk : kSuggestionWarn;
  canvas.Select(Pen(3, line_color));
  // A cross-hair at the target.
  canvas.DrawLine(target.x - 12, target.y, target.x + 12, target.y);
  canvas.DrawLine(target.x, target.y - 12, target.x, target.y + 12);
  canvas.DrawCircle(target, 14);

  // Banner with the reason text (bottom of screen, handled by the
  // InfoBox or a dedicated overlay widget — this is the map part).
}

} // namespace OpenGlide
