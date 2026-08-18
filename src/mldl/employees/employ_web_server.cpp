/*
 *
 *         ▜ ▘▗ ▗ ▜      ▌      ▜   ▌
 * ▛▛▌▌▌  ▐ ▌▜▘▜▘▐ █▌  ▛▌█▌▌▌  ▐ ▀▌▛▌
 * ▌▌▌▙▌  ▐▖▌▐▖▐▖▐▖▙▖  ▙▌▙▖▚▘  ▐▖█▌▙▌
 *    ▄▌
 *
 * MIT License
 *
 * Copyright (c) 2025-2026 Evgenii Sopov
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Original repository: https://github.com/sea5kg/my-little-dev-lab
 */

#include "HttpService.h"
#include "mldl/include/config.h"
#include "mldl/include/webhooks.h"
#include "mldl/include/web_server.h"
#include <json.hpp>
#include <memory>
#include <mutex>
#include <sea5kg_logger.h>
#include <string>
#include <wsjcpp_core.h>
#include <wsjcpp_employees.h>
// #include "WebSocketServer.h"
#include "EventLoop.h"
#include "WebSocketServer.h" // libhv
#include "hlog.h" // libhv
#include "hssl.h" // libhv
#include "htime.h" // libhv
#include <regex>

class employ_web_server : public WsjcppEmployBase, public mldl::web_server {
public:
  employ_web_server();

  // WsjcppEmployBase
  virtual bool init(const std::string &sName, bool bSilent) override;
  virtual bool deinit(const std::string &sName, bool bSilent) override;

  // mldl::web_server
  virtual int start() override;

  // other
  hv::HttpService *getService();
  int httpHandleRequests(HttpRequest *req, HttpResponse *resp);
  int httpApi(HttpRequest *req, HttpResponse *resp);
  int httpWebhook(HttpRequest *req, HttpResponse *resp, const std::string &request_path);

private:
  std::string TAG;
  std::mutex m_mutex;
  hv::HttpService *m_pHttpService;
  mldl::config *m_pConfig;
  std::string m_sIndexHtml;
  std::string m_sHtmlFolder;
  std::map<std::string, std::string> m_web_sites;
};

REGISTRY_WSJCPP_EMPLOY(employ_web_server)

employ_web_server::employ_web_server() : WsjcppEmployBase({mldl::web_server::name()}, {mldl::config::name(), mldl::webhooks::name()}) {
  TAG = "WEB_SERVER";
  m_pHttpService = new hv::HttpService();
}

static std::shared_ptr<sea5kg::logger> g_http_logger = std::shared_ptr<sea5kg::logger>(sea5kg::logger::create());

void EmployWebServer_custom_logger(int level, const char *msg, int len) {
  static const std::string TAG = "http-hv";
  std::string message(msg, len - 1); // remove last '\n' character
  switch (level) {
  case LOG_LEVEL_DEBUG:
    g_http_logger->debug(TAG, message);
    break;
  case LOG_LEVEL_INFO:
    g_http_logger->info(TAG, message);
    break;
  case LOG_LEVEL_WARN:
    sea5kg::log::warning(TAG, message);
    g_http_logger->warning(TAG, message);
    break;
  case LOG_LEVEL_ERROR:
    sea5kg::log::error(TAG, message);
    g_http_logger->error(TAG, message);
    break;
  case LOG_LEVEL_FATAL:
    sea5kg::log::error(TAG, message);
    g_http_logger->critical(TAG, message);
    break;
  default:
    sea5kg::log::error(TAG, "Unknow level: " + message);
    g_http_logger->error(TAG, message);
  }
}

bool employ_web_server::init(const std::string &name, bool bSilent) {
  if (!bSilent) {
    sea5kg::log::info(TAG, "init " + name);
  }
  return true;
}

bool employ_web_server::deinit(const std::string &sName, bool bSilent) {
  if (!bSilent) {
    sea5kg::log::info(TAG, "deinit");
  }
  return true;
}

int employ_web_server::start() {
  m_pConfig = findWsjcppEmploy<mldl::config>();
  // m_pEmployFlags = findWsjcppEmploy<EmployFlags>();
  // m_pEmployDatabase = findWsjcppEmploy<EmployDatabase>();
  // m_pTeamLogos = findWsjcppEmploy<EmployTeamLogos>();
  m_sHtmlFolder = m_pConfig->html_folder();

  // logger
  g_http_logger->set_log_filename_prefix("http_hv_");
  g_http_logger->set_log_dirpath(sea5kg::log::log_dirpath());
  g_http_logger->set_rotation_period_in_seconds(sea5kg::log::rotation_period_in_seconds());
  g_http_logger->set_log_level_file_output(sea5kg::log_level::DEBUG);
  g_http_logger->set_log_level_console_output(sea5kg::log_level::DEBUG);

  // std::string starting_message = "Starting web on http://localhost:" + std::to_string(bna_server::WEB_TCP_PORT) +
  // "/";
  g_http_logger->success(TAG, "init");

  {
    logger_t *pLogger = hv_default_logger();
    logger_set_handler(pLogger, EmployWebServer_custom_logger);
    logger_set_format(pLogger, "%s"); // removing time and log level

    // Test the log
    hlogi("This is an info message.");
  }

  m_web_sites = m_pConfig->web_sites();

  // static files
  m_pHttpService->document_root = "./html";

  m_pHttpService->GET(
      "*", std::bind(&employ_web_server::httpHandleRequests, this, std::placeholders::_1, std::placeholders::_2));
  m_pHttpService->POST(
      "*", std::bind(&employ_web_server::httpHandleRequests, this, std::placeholders::_1, std::placeholders::_2));

  hv::HttpServer server(m_pHttpService);
  server.setPort(m_pConfig->web_port());
  server.setThreadNum(4);
  server.run();

  // // websocket_server_t server;
  // // server.service = pRouter;
  // // server.port = 12345;
  // // // server.ws = pWs;
  // // websocket_server_run(&server);

  return 0;
}

hv::HttpService *employ_web_server::getService() {
  return m_pHttpService;
}

// int WebServer::httpApiV1GetPaths(HttpRequest* req, HttpResponse* resp) {
//   return resp->Json(m_pHttpService->Paths());
// }

int employ_web_server::httpHandleRequests(HttpRequest *req, HttpResponse *resp) {
  std::string host = req->host;
  std::string html_folder = m_sHtmlFolder;
  if (m_web_sites.count(host) > 0) {
    html_folder = m_web_sites[host];
    std::cout << req->host << " -> " << html_folder << std::endl;
  }

  std::string sOriginalRequestPath = req->path;
  std::string request_path;

  // remove get params from path
  std::size_t nFoundGetParams = sOriginalRequestPath.rfind("?");
  if (nFoundGetParams != std::string::npos) {
    request_path = sOriginalRequestPath.substr(0, nFoundGetParams);
  } else {
    request_path = sOriginalRequestPath;
  }
  request_path = wsjcpp::normalize_filepath(request_path);

  if (wsjcpp::starts_with(request_path, "/webhook/")) {
    return this->httpWebhook(req, resp, request_path);
  }

  // sea5kg::log::info(TAG, "request_path = " + request_path);
  if (wsjcpp::starts_with(request_path, "/api/")) {
    std::cout << "CALLED API" << std::endl;
    return this->httpApi(req, resp);
  }

  if (request_path == "/") {
    request_path = "/index.html";
  }

  if (request_path == "/admin" || request_path == "/admin/") {
    request_path = "/index.html";
  }

  // TODO
  sea5kg::log::info(TAG, "Request path: " + request_path);
  std::string sFilePath = wsjcpp::normalize_filepath(html_folder + "/" + request_path);
  if (wsjcpp::file_exists(sFilePath)) { // TODO check the file exists not dir
    return resp->File(sFilePath.c_str());
  }
  // cache
  sea5kg::log::info(TAG, "File from cache: " + request_path);
  std::string sResPath = "./data/html" + request_path;
  if (WsjcppResourcesManager::has(sResPath)) {
    WsjcppResourceFile *pFile = WsjcppResourcesManager::get(sResPath);
    resp->Data((void *)pFile->getBuffer(), pFile->getBufferSize(),
               true // nocopy
    );
    resp->SetContentTypeByFilename(sResPath.c_str());
    return 200;
  }
  return 404; // Not found
}

int employ_web_server::httpApi(HttpRequest *req, HttpResponse *resp) {
  auto now = std::chrono::system_clock::now().time_since_epoch();
  int nCurrentTimeSec = std::chrono::duration_cast<std::chrono::seconds>(now).count();

  if (req->method != HTTP_POST) {
    return 403;
  }

  nlohmann::json req_json_body;
  try {
    req_json_body = nlohmann::json::parse(req->body);
  } catch (nlohmann::json::parse_error &error) {
    std::cerr << "Parse error at byte: " << error.byte << std::endl;
    return 400;
  }

  if (!req_json_body.is_object()) {
    return 400;
  }

  if (!req_json_body["method"].is_string()) {
    std::cerr << "Not found field method " << std::endl;
    return 400;
  }

  std::string sMethod = req_json_body["method"];

  // auto *pHandler = WsjcppJsonRpc20::findJsonRpc20Handler(sMethod);

  // auto *pClient = new WsjcppJsonRpc20WebSocketClient();
  // pClient->set

  // auto *request = new WsjcppJsonRpc20Request(pClient, nullptr);
  // request->parseIncomeData(req_json_body.dump());

  // public:
  //     WsjcppJsonRpc20Request(
  //         WsjcppJsonRpc20WebSocketClient *pClient,
  //         WsjcppJsonRpc20WebSocketServer *pWebSocketServer
  //     );

  // pHandler->handle(WsjcppJsonRpc20Request *pRequest);

  // if (!pHandler) {
  //     return 400;
  // }

  // WsjcppJsonRpc20 {
  // public:
  //     static void initGlobalVariables();
  //     static void addHandler(const std::string &sMethodName, WsjcppJsonRpc20HandlerBase* pHandler);
  //     static WsjcppJsonRpc20HandlerBase *findJsonRpc20Handler(const std::string &sMethodName);

  std::cout << req->body << ", req_json_body.dump(): " << req_json_body.dump() << ", sMethod: " << sMethod << std::endl;

  // std::string sRequestIP = req->client_addr.ip;
  // std::string sRequestIP_MsgSuffix = " (" + sRequestIP + ")";

  // // TODO light update scoreboard
  // int nPoints = m_pConfig->scoreboard()->incrementAttackScore(flag, sTeamId);
  // std::string sPoints = std::to_string(double(nPoints) / 10.0);

  // std::string sResponse = "Accepted: Received flag {" + sFlag + "} from {" + sTeamId + "} (Accepted + " + sPoints +
  // ")"; sea5kg::log::ok(TAG, sResponse + sRequestIP_MsgSuffix); resp->Data(
  //     (void *)(sResponse.c_str()),
  //     sResponse.size(),
  //     false // copy buffer
  // );
  resp->content_type = TEXT_PLAIN;
  // resp->SetContentTypeByFilename("scoreboard.json");
  return 200;
}

int employ_web_server::httpWebhook(HttpRequest *req, HttpResponse *resp, const std::string &request_path) {
  // auto now = std::chrono::system_clock::now().time_since_epoch();
  // int nCurrentTimeSec = std::chrono::duration_cast<std::chrono::seconds>(now).count();

  if (req->method != HTTP_POST && req->method != HTTP_GET) {
    std::cout << "Expected POST or GET" << std::endl;
    return 403;
  }
  static const std::string webhook_prefix = "/webhook/";
  if (!wsjcpp::starts_with(request_path, webhook_prefix)) {
    sea5kg::log::error(TAG, "wrong prefix in request_path: " + request_path);
    return 403;
  }
  std::string webhook_id = request_path.substr(webhook_prefix.size(), 100);
  sea5kg::log::info(TAG, "webhook_id: " + webhook_id);

  auto webhooks = findWsjcppEmploy<mldl::webhooks>();

  if (!webhooks->contains(webhook_id)) {
    sea5kg::log::error(TAG, "Not found webhook: " + webhook_id);
    return 404;
  }

  if (!webhooks->call(webhook_id)) {
    sea5kg::log::error(TAG, "Could not call webhook: " + webhook_id);
    return 500;
  }

  return 200;
}

// int WebServer::httpApiV1Scoreboard(HttpRequest* req, HttpResponse* resp) {
//     // m_pTeamLogos->updateLastWriteTime();
//     // nlohmann::json jsonScoreboard = m_pConfig->scoreboard()->toJson();
//     // m_pTeamLogos->updateScorebordJson(jsonScoreboard);
//     // std::string sScoreboardJson = jsonScoreboard.dump();
//     // resp->Data(
//     //     (void *)(sScoreboardJson.c_str()),
//     //     sScoreboardJson.length(),
//     //     false // nocopy - force copy
//     // );
//     // resp->SetContentTypeByFilename("scoreboard.json");
//     return 200;
// }

// int WebServer::httpApiV1Game(HttpRequest* req, HttpResponse* resp) {
//     // std::cout << m_sCacheResponseGameJson << std::endl;
//     resp->Data(
//         (void *)(m_sCacheResponseGameJson.c_str()),
//         m_sCacheResponseGameJson.length(),
//         true // nocopy
//     );
//     resp->SetContentTypeByFilename("game.json");
//     return 200;
// }

// int WebServer::httpApiV1MyIp(HttpRequest* req, HttpResponse* resp) {
//     resp->json["myip"] = req->client_addr.ip;
//     return 200;
// }
