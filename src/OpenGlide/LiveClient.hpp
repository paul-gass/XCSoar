// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright OpenGlide Project

/**
 * @file LiveClient.hpp
 * @brief Background HTTP client: polls the OpenGlide server.
 *
 * Every 1–2 minutes (configurable) the client sends a compact JSON
 * payload with the glider's current state and receives:
 *
 * - A route suggestion (or null).
 * - Weather polygons for the map overlay.
 * - Live thermal markers from OGN.
 * - The data age so the UI can grey out stale info.
 *
 * The client is resilient to network loss: if no response comes back,
 * the last confirmed state stays active and no new suggestions appear.
 * The XCSoar task and map are never touched without pilot confirmation.
 *
 * Threading: the client runs on its own thread. It reads the GPS/task
 * state through thread-safe accessors and writes suggestions to a
 * lock-free slot that the UI thread polls on each map redraw.
 */

#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace OpenGlide {

/**
 * What the server proposes.
 */
struct Suggestion {
  enum class Kind : uint8_t {
    None,
    AATTarget,   // move AAT sector target
    Detour,      // insert an intermediate waypoint
    Reroute,     // change the whole line (free flight)
  };

  Kind kind = Kind::None;
  int sector_index = -1;        // 1-based for AAT
  double target_lat = 0;
  double target_lon = 0;
  std::string reason;           // shown in the cockpit banner
  double gain_s = 0;            // expected time saved
  double valid_until = 0;       // unix timestamp
  double data_age_s = 0;
};

/**
 * A polygon the map should draw (storm cell, rain area, cumulus field).
 */
struct WeatherPolygon {
  std::string kind;             // "storm_cell", "precipitation", "cumulus_field"
  std::string severity;         // "warning", "danger"
  std::string label;
  std::vector<std::pair<double, double>> ring;  // (lon, lat) closed ring
  double buffer_m = 0;
};

/**
 * A thermal observed via OGN.
 */
struct ThermalMarker {
  double lat;
  double lon;
  double climb_ms;
  int count;
  int age_s;
};

/**
 * What the server sent back.
 */
struct ServerResponse {
  std::optional<Suggestion> suggestion;
  std::vector<WeatherPolygon> weather_polygons;
  std::vector<ThermalMarker> thermals;
  double data_age_s = -1;       // -1 = no data
  bool valid = false;
};

/**
 * Configuration for the live client.
 */
struct ClientConfig {
  std::string server_url = "https://api.openglide.org";
  double poll_interval_s = 90;  // 1.5 minutes
  double timeout_s = 10;
  std::string pilot_id;
  std::string auth_token;
};

class LiveClientImpl;

/**
 * The live client. Create one, call Start(), read the latest response
 * from GetLatest() on each map tick.
 */
class LiveClient {
public:
  explicit LiveClient(const ClientConfig &config);
  ~LiveClient();

  LiveClient(const LiveClient &) = delete;
  LiveClient &operator=(const LiveClient &) = delete;

  void Start();
  void Stop();

  [[nodiscard]] bool IsRunning() const noexcept;
  [[nodiscard]] bool IsConnected() const noexcept;

  /**
   * The most recent server response (thread-safe snapshot).
   */
  [[nodiscard]] ServerResponse GetLatest() const noexcept;

private:
  std::unique_ptr<LiveClientImpl> impl;
};

} // namespace OpenGlide
