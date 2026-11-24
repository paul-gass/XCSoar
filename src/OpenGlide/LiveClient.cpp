// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright OpenGlide Project

#include "LiveClient.hpp"

#include "net/client/SyncHttp.hpp"
#include "LogFile.hpp"

#include <atomic>
#include <chrono>
#include <cmath>
#include <mutex>
#include <thread>

#include <curl/curl.h>

namespace OpenGlide {

// Simple JSON parsing helpers (XCSoar doesn't have a full JSON library
// in all builds; we parse the compact response format manually).

namespace {

// Skip whitespace
const char* skip_ws(const char* p) {
  while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') ++p;
  return p;
}

// Parse a double from JSON
bool parse_double(const char*& p, double& out) {
  char* end;
  out = strtod(p, &end);
  if (end == p) return false;
  p = end;
  return true;
}

// Parse a string from JSON (returns pointer to content, sets len)
bool parse_string(const char*& p, const char*& start, size_t& len) {
  p = skip_ws(p);
  if (*p != '"') return false;
  ++p;
  start = p;
  while (*p && *p != '"') {
    if (*p == '\\' && *(p+1)) p += 2;
    else ++p;
  }
  len = p - start;
  if (*p == '"') ++p;
  return true;
}

// Find a key in a JSON object (very simple, no nesting)
const char* find_key(const char* json, const char* key) {
  const char* p = json;
  size_t klen = strlen(key);
  while ((p = strstr(p, key)) != nullptr) {
    // Check it's a proper key (preceded by " and followed by ":)
    if (p > json && *(p-1) == '"') {
      const char* after = p + klen;
      if (*after == '"') {
        after = skip_ws(after + 1);
        if (*after == ':') return skip_ws(after + 1);
      }
    }
    ++p;
  }
  return nullptr;
}

// Parse the server response JSON
ServerResponse parse_response(const std::string& body) {
  ServerResponse resp;
  resp.valid = false;
  
  if (body.empty() || body[0] != '{') return resp;
  
  const char* json = body.c_str();
  
  // Parse data_age_s
  if (const char* p = find_key(json, "data_age_s")) {
    parse_double(p, resp.data_age_s);
  }
  
  // Parse suggestion if present
  if (const char* p = find_key(json, "suggestion")) {
    p = skip_ws(p);
    if (*p == '{') {
      Suggestion sug;
      
      // Parse kind
      if (const char* k = find_key(p, "kind")) {
        const char* start; size_t len;
        if (parse_string(k, start, len)) {
          std::string kind(start, len);
          if (kind == "aat_target") sug.kind = Suggestion::Kind::AATTarget;
          else if (kind == "detour") sug.kind = Suggestion::Kind::Detour;
          else if (kind == "reroute") sug.kind = Suggestion::Kind::Reroute;
        }
      }
      
      // Parse sector_index
      if (const char* k = find_key(p, "sector_index")) {
        double v;
        if (parse_double(k, v)) sug.sector_index = (int)v;
      }
      
      // Parse target_lat, target_lon
      if (const char* k = find_key(p, "target_lat")) parse_double(k, sug.target_lat);
      if (const char* k = find_key(p, "target_lon")) parse_double(k, sug.target_lon);
      
      // Parse reason
      if (const char* k = find_key(p, "reason")) {
        const char* start; size_t len;
        if (parse_string(k, start, len)) sug.reason = std::string(start, len);
      }
      
      // Parse gain_s
      if (const char* k = find_key(p, "gain_s")) parse_double(k, sug.gain_s);
      
      // Parse valid_until
      if (const char* k = find_key(p, "valid_until")) parse_double(k, sug.valid_until);
      
      if (sug.kind != Suggestion::Kind::None) {
        resp.suggestion = sug;
      }
    }
  }
  
  // TODO: Parse weather_polygons and thermals arrays
  // For now, we focus on the suggestion which is the core feature
  
  resp.valid = true;
  return resp;
}

} // anonymous namespace

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

    // Build the URL
    std::string url = config_.server_url + "/api/live/route";
    
    // Set up headers
    curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    if (!config_.auth_token.empty()) {
      std::string auth = "Authorization: Bearer " + config_.auth_token;
      headers = curl_slist_append(headers, auth.c_str());
    }
    
    // Make the HTTP POST request
    Net::Client::SyncHttpResponse response;
    bool ok = Net::Client::SyncHttpPost(
      url.c_str(),
      headers,
      body,
      response,
      nullptr,  // no cookie capture
      nullptr   // cookie to send
    );
    
    curl_slist_free_all(headers);
    
    if (!ok || response.http_code != 200) {
      connected_ = false;
      LogFormat("OpenGlide: HTTP request failed (code %ld)", response.http_code);
      return;
    }
    
    connected_ = true;
    
    // Parse the response
    ServerResponse parsed = parse_response(response.body);
    
    {
      std::lock_guard lock(mutex_);
      latest_ = parsed;
    }
    
    if (parsed.valid && parsed.suggestion.has_value()) {
      LogFormat("OpenGlide: received suggestion — %s (gain %.0f s)",
                parsed.suggestion->reason.c_str(),
                parsed.suggestion->gain_s);
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
