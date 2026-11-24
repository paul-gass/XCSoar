// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright OpenGlide Project

#include "PlanImport.hpp"

#include "net/client/SyncHttp.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "LogFile.hpp"

#include <curl/curl.h>

namespace OpenGlide {

bool
ImportLatestPlan(ProtectedTaskManager &task_manager,
                 const PlanImportConfig &config)
{
  // Build the URL
  std::string url = config.server_url + "/api/plans/latest/tsk";
  
  // Set up headers
  curl_slist* headers = nullptr;
  if (!config.auth_token.empty()) {
    std::string auth = "Authorization: Bearer " + config.auth_token;
    headers = curl_slist_append(headers, auth.c_str());
  }
  
  // Make the HTTP GET request
  Net::Client::SyncHttpResponse response;
  bool ok = Net::Client::SyncHttpGet(
    url.c_str(),
    headers,
    true,  // accept gzip
    response,
    nullptr,  // no cookie capture
    nullptr   // progress callback
  );
  
  curl_slist_free_all(headers);
  
  if (!ok || response.http_code != 200) {
    LogFormat("OpenGlide: plan import failed (code %ld)", response.http_code);
    return false;
  }
  
  if (response.body.empty()) {
    LogFormat("OpenGlide: plan import returned empty body");
    return false;
  }
  
  // TODO: Parse the .tsk XML and load it into the task manager.
  // This requires using XCSoar's TaskFile infrastructure.
  // For now, we just log success.
  
  LogFormat("OpenGlide: downloaded plan (%zu bytes)", response.body.size());
  
  // The actual task loading would be:
  // 1. Write response.body to a temp file
  // 2. Use TaskFile::Create() to parse it
  // 3. Use task_manager.TaskCommit() to load it
  
  (void)task_manager;  // Suppress unused warning for now
  
  return true;
}

} // namespace OpenGlide
