// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright OpenGlide Project

#include "WeatherLayer.hpp"

#include "ui/canvas/Canvas.hpp"
#include "ui/dim/BulkPoint.hpp"
#include "Projection/WindowProjection.hpp"
#include "Geo/GeoPoint.hpp"

#include <algorithm>
#include <cmath>

namespace OpenGlide {

// Colours (no alpha on all platforms)
static constexpr Color kRainFill(100, 100, 255);
static constexpr Color kStormFill(255, 50, 50);
static constexpr Color kStormBorder(200, 0, 0);
static constexpr Color kCumulusFill(80, 200, 80);
static constexpr Color kThermalUp(255, 140, 0);
static constexpr Color kSuggestionOk(50, 200, 50);
static constexpr Color kSuggestionWarn(255, 200, 0);

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

  std::vector<BulkPixelPoint> points;
  points.reserve(ring.size());
  for (const auto &[lon, lat] : ring) {
    auto pt = GeoToScreen(proj, lat, lon);
    points.push_back(BulkPixelPoint{pt.x, pt.y});
  }

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
  (void)stale;  // TODO: grey out stale data

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

  // 2. OGN thermal markers (small circles).
  for (const auto &th : response.thermals) {
    auto pt = GeoToScreen(projection, th.lat, th.lon);
    unsigned radius = std::clamp(static_cast<unsigned>(th.climb_ms * 4), 4u, 16u);
    canvas.Select(Brush(kThermalUp));
    canvas.Select(Pen(1, Color(200, 100, 0)));
    canvas.DrawCircle(pt, radius);
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

  // The colour depends on the severity: green for a gain, yellow for a warning.
  Color line_color = suggestion.gain_s > 0 ? kSuggestionOk : kSuggestionWarn;
  canvas.Select(Pen(3, line_color));
  
  // A cross-hair at the target.
  canvas.DrawLine(PixelPoint{target.x - 12, target.y}, 
                  PixelPoint{target.x + 12, target.y});
  canvas.DrawLine(PixelPoint{target.x, target.y - 12}, 
                  PixelPoint{target.x, target.y + 12});
  canvas.DrawCircle(target, 14);
}

} // namespace OpenGlide
