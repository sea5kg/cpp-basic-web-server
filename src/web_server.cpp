/**********************************************************************************
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
 ***********************************************************************************/

#include "web_server.h"

// #include "WebSocketServer.h"
#include "EventLoop.h"
#include "htime.h"
#include "hssl.h"
#include "hlog.h"
#include <regex>
#include <wsjcpp_core.h>
// #include <wsjcpp_jsonrpc20.h>


using namespace hv;


WebServer::WebServer() {
  TAG = "WebServer";
  m_pConfig = findWsjcppEmploy<EmployConfig>();
  // m_pEmployFlags = findWsjcppEmploy<EmployFlags>();
  // m_pEmployDatabase = findWsjcppEmploy<EmployDatabase>();
  // m_pTeamLogos = findWsjcppEmploy<EmployTeamLogos>();
  m_sHtmlFolder = m_pConfig->getHtmlFolder();

  // {
  //     logger_t* pLogger = hv_default_logger();
  //     // logger_set_max_filesize(pLogger, 102400);
  //     std::string sLogDirPath = m_pConfig->getWorkDir() + "/hv_logs";
  //     if (!wsjcpp::dir_exists(sLogDirPath)) {
  //         WsjcppCore::makeDir(sLogDirPath);
  //     }
  //     std::string sLogFilePath = sLogDirPath + "/http_" + WsjcppCore::getCurrentTimeForFilename() + ".log";
  //     logger_set_file(pLogger, sLogFilePath.c_str());
  // }

  m_pHttpService = new HttpService();

  m_web_sites = m_pConfig->web_sites();

  // static files
  m_pHttpService->document_root = "./html";

  m_pHttpService->GET("*", std::bind(&WebServer::httpHandleRequests, this, std::placeholders::_1, std::placeholders::_2));
  m_pHttpService->POST("*", std::bind(&WebServer::httpHandleRequests, this, std::placeholders::_1, std::placeholders::_2));
}

hv::HttpService *WebServer::getService() {
  return m_pHttpService;
}

// int WebServer::httpApiV1GetPaths(HttpRequest* req, HttpResponse* resp) {
//   return resp->Json(m_pHttpService->Paths());
// }

int WebServer::httpHandleRequests(HttpRequest* req, HttpResponse* resp) {
  std::string host = req->host;
  std::string html_folder = m_sHtmlFolder;
  if (m_web_sites.count(host) > 0) {
    html_folder = m_web_sites[host];
    std::cout << req->host << " -> " << html_folder  << std::endl;
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
    std::cout << "CALLED WebHook" << std::endl;
    return this->httpWebhook(req, resp, request_path);
  }

  // WsjcppLog::info(TAG, "request_path = " + request_path);
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
  WsjcppLog::info(TAG, "Request path: " + request_path);
  std::string sFilePath = wsjcpp::normalize_filepath(html_folder + "/" + request_path);
  if (wsjcpp::file_exists(sFilePath)) { // TODO check the file exists not dir
    return resp->File(sFilePath.c_str());
  }
  // cache
  WsjcppLog::info(TAG, "File from cache: " + request_path);
  std::string sResPath = "./data/html" + request_path;
  if (WsjcppResourcesManager::has(sResPath)) {
    WsjcppResourceFile *pFile = WsjcppResourcesManager::get(sResPath);
    resp->Data(
      (void *)pFile->getBuffer(),
      pFile->getBufferSize(),
      true // nocopy
    );
    resp->SetContentTypeByFilename(sResPath.c_str());
    return 200;
  }
  return 404; // Not found
}

int WebServer::httpApi(HttpRequest* req, HttpResponse* resp) {
  auto now = std::chrono::system_clock::now().time_since_epoch();
  int nCurrentTimeSec = std::chrono::duration_cast<std::chrono::seconds>(now).count();

  if (req->method != HTTP_POST) {
    return 403;
  }

  nlohmann::json req_json_body;
  try {
    req_json_body = nlohmann::json::parse(req->body);
  } catch (nlohmann::json::parse_error& error){
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
  // std::string sRequestIP_MsgSuffex = " (" + sRequestIP + ")";

  // // TODO light update scoreboard
  // int nPoints = m_pConfig->scoreboard()->incrementAttackScore(flag, sTeamId);
  // std::string sPoints = std::to_string(double(nPoints) / 10.0);

  // std::string sResponse = "Accepted: Recieved flag {" + sFlag + "} from {" + sTeamId + "} (Accepted + " + sPoints + ")";
  // WsjcppLog::ok(TAG, sResponse + sRequestIP_MsgSuffex);
  // resp->Data(
  //     (void *)(sResponse.c_str()),
  //     sResponse.size(),
  //     false // copy buffer
  // );
  resp->content_type = TEXT_PLAIN;
  // resp->SetContentTypeByFilename("scoreboard.json");
  return 200;
}

int WebServer::httpWebhook(HttpRequest* req, HttpResponse* resp, const std::string &request_path) {
  auto now = std::chrono::system_clock::now().time_since_epoch();
  int nCurrentTimeSec = std::chrono::duration_cast<std::chrono::seconds>(now).count();
  std::cout << "request_path: " << request_path << std::endl;

  if (req->method != HTTP_POST && req->method != HTTP_GET) {
    std::cout << "Expected POST or GET" << std::endl;
    return 403;
  }
  static const std::string webhook_prefix = "/webhook/";
  if (!wsjcpp::starts_with(request_path, webhook_prefix)) {
    std::cout << "wrong prefix" << std::endl;
    return 403;
  }
  std::string webhook_id = request_path.substr(webhook_prefix.size(), 100);
  std::cout << "substr: " << webhook_id << std::endl;
  std::map<std::string, std::shared_ptr<mldl::repository>> hooks = m_pConfig->webhooks();
  if (!hooks.count(webhook_id)) {
    std::cout << "Not found webhook: " << webhook_id << std::endl;
    return 404;
  }
  std::cout << "repo_folder: " << hooks[webhook_id]->repo_folder() << std::endl;

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
