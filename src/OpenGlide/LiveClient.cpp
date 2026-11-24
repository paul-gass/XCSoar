// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright OpenGlide Project

#include "LiveClient.hpp"

#include <atomic>
#include <chrono>
#include <cmath>
#include <mutex>
#include <thread>

// XCSoar's HTTP client (libcurl wrapper)
#include "net/http/Easy.hxx"
#include "net/http/Init.hxx"
#include "json/ParserOutputStream.hxx"
#include "util/StringCompare.hxx"

#ifdef HAVE_POSIX
#include <unistd.h>
#endif

namespace OpenGlide {

// Minimal JSON helpers (XCSoar uses boost::json in some places;
// we keep it simple with the existing parser infrastructure).

class LiveClientImpl {
public:
  explicit LiveClientImpl(const ClientConfig &config)
    : config_(config) {}

  ~LiveClientImpl() { Stop(); }

  void Start() {
    if (running_.load()) return;
    running_ = true;
    thread_ = std::thread([this] { Run(); });
  }

  void Stop() {
    running_ = false;
    if (thread_.joinable())
      thread_.join();
  }

  bool IsRunning() const noexcept { return running_.load(); }
  bool IsConnected() const noexcept { return connected_.load(); }

  ServerResponse GetLatest() const noexcept {
    std::lock_guard lock(mutex_);
    return latest_;
  }

  // Called by the main thread to provide current state for the next poll.
  void SetState(double lat, double lon, double alt_m,
                double gs_ms, double hdg_deg,
                double wind_e, double wind_n,
                double mc, const std::string &task_json) {
    std::lock_guard lock(state_mutex_);
    state_lat_ = lat;
    state_lon_ = lon;
    state_alt_ = alt_m;
    state_gs_ = gs_ms;
    state_hdg_ = hdg_deg;
    state_wind_e_ = wind_e;
    state_wind_n_ = wind_n;
    state_mc_ = mc;
    state_task_json_ = task_json;
  }

private:
  void Run() {
    while (running_.load()) {
      try {
        Poll();
        connected_ = true;
      } catch (...) {
        connected_ = false;
      }

      // Sleep in small increments so Stop() is responsive.
      auto wake = std::chrono::steady_clock::now()
        + std::chrono::milliseconds(
            static_cast<int>(config_.poll_interval_s * 1000));
      while (running_.load() && std::chrono::steady_clock::now() < wake) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
      }
    }
  }

  void Poll() {
    // Build the JSON request body from the current state.
    std::string body;
    {
      std::lock_guard lock(state_mutex_);
      body = BuildRequestBody();
    }

    // POST to the server.
    std::string url = config_.server_url + "/api/live/route";
    // This is a placeholder — the actual HTTP call uses XCSoar's
    // Net::Session / CURL wrapper. The implementation connects to
    // the same endpoint that the Python routing service exposes.

    // Parse the response and store it.
    // For now: mark as connected. The full HTTP implementation
    // will use XCSoar's existing net/http infrastructure.
    ServerResponse response;
    response.valid = true;
    response.data_age_s = -1;

    {
      std::lock_guard lock(mutex_);
      latest_ = response;
    }
  }

  std::string BuildRequestBody() const {
    // Compact JSON — the server expects this format.
    char buf[1024];
    snprintf(buf, sizeof(buf),
      R"({"lat":%.6f,"lon":%.6f,"alt_m":%.0f,"groundspeed_ms":%.1f,)"
      R"("heading_deg":%.0f,"wind_east_ms":%.1f,"wind_north_ms":%.1f,)"
      R"("mc_setting":%.1f,"pilot_id":"%s","task_type":"aat"})",
      state_lat_, state_lon_, state_alt_, state_gs_, state_hdg_,
      state_wind_e_, state_wind_n_, state_mc_,
      config_.pilot_id.c_str());
    return std::string(buf);
  }

  ClientConfig config_;
  std::thread thread_;
  std::atomic<bool> running_{false};
  std::atomic<bool> connected_{false};

  mutable std::mutex mutex_;
  ServerResponse latest_;

  mutable std::mutex state_mutex_;
  double state_lat_ = 0, state_lon_ = 0, state_alt_ = 0;
  double state_gs_ = 0, state_hdg_ = 0;
  double state_wind_e_ = 0, state_wind_n_ = 0;
  double state_mc_ = 0;
  std::string state_task_json_;
};

// --- Public API (pImpl forwarding) ---

LiveClient::LiveClient(const ClientConfig &config)
  : impl(std::make_unique<LiveClientImpl>(config)) {}

LiveClient::~LiveClient() = default;

void LiveClient::Start() { impl->Start(); }
void LiveClient::Stop() { impl->Stop(); }
bool LiveClient::IsRunning() const noexcept { return impl->IsRunning(); }
bool LiveClient::IsConnected() const noexcept { return impl->IsConnected(); }
ServerResponse LiveClient::GetLatest() const noexcept { return impl->GetLatest(); }

} // namespace OpenGlide
