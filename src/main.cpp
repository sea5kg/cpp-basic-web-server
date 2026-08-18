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

#include "WebSocketServer.h" // libhv
#include "mldl/include/config.h"
#include "web_server.h"
#include <wsjcpp_core.h>
#include <wsjcpp_employees.h>

bool is_root() {
  // Root always has an Effective User ID (EUID) of 0
  return geteuid() == 0;
}

bool change_privileges(int user_id) {
  std::cout << " ...Trying change privileges (setgid)" << std::endl;
  if (setgid(user_id) != 0) {
    std::cerr << " -> FAIL. Failed to set GID" << std::endl;
    return false;
  }
  std::cout << " ...Trying change privileges (setuid)" << std::endl;
  if (setuid(user_id) != 0) {
    std::cerr << " -> FAIL. Failed to set UID" << std::endl;
    return false;
  }
  std::cout << " ...Trying change privileges (verify)" << std::endl;
  if (setuid(0) == 0) {
    std::cerr << " -> FAIL. Security Risk: Privileges were not permanently dropped!" << std::endl;
    return false;
  }
  std::cout << " ...Trying change privileges (test)" << std::endl;
  if (getuid() == user_id) {
    std::cout << "-> OK. Successful changed privileges." << std::endl;
  } else {
    std::cerr << " -> FAIL. NOT CHANGED." << std::endl;
    return false;
  }
  return true;
}

bool try_apply_mldl_user(const std::string &work_dir) {
  // std::cout << "work_dir = " << work_dir << std::endl;
  std::string str_user;
  int user_id = 0;
  if (WsjcppCore::getEnv("MLDL_USER", str_user)) {
    std::cout << "MLDL_USER='" << str_user << "'" << std::endl;
    try {
      user_id = std::stoi(str_user);
    } catch (const std::invalid_argument &e) {
      std::cerr << "Error: No conversion could be performed. MLDL_USER='" << str_user << "'" << std::endl;
      return false;
    } catch (const std::out_of_range &e) {
      std::cerr << "The converted value is too big for an int.. MLDL_USER='" << str_user << "'" << std::endl;
      return false;
    } catch (...) {
      std::cerr << "The converted value is too big for an int.. MLDL_USER='" << str_user << "'" << std::endl;
      return false;
    }
    if (is_root()) {
      std::cout << " ...Try change owner for '" << work_dir << "' to '" << str_user << ":" << str_user << "'"
                << std::endl;
      std::string cmd = "chown -R " + std::to_string(user_id) + ":" + std::to_string(user_id) + " \"" + work_dir + "\"";
      if (system(cmd.c_str()) == 0) {
        std::cout << " -> OK. Successful changed owner for data." << std::endl;
      } else {
        std::cerr << " -> FAIL. Could not change owner for directory." << std::endl;
        return false;
      }
      return change_privileges(user_id);
    } else if (geteuid() == user_id) {
      std::cout << " * OK. MLDL_USER is equal with current user" << std::endl;
    } else {
      return change_privileges(user_id);
    }
    return true;
  }
  return true;
}

int main(int argc, const char *argv[]) {
  std::string TAG = "MAIN";
  std::string appName = std::string(WSJCPP_APP_NAME);
  std::string appVersion = std::string(WSJCPP_APP_VERSION);

  // previous logs in current directory
  if (!wsjcpp::dir_exists(".logs")) {
    WsjcppCore::makeDir(".logs");
  }
  WsjcppLog::setPrefixLogFile("cpp_web_server");
  WsjcppLog::setLogDirectory(".logs");

  // try find config.yml
  const std::vector<std::string> vPossibleFolders = {"./data", "/root/data/"};
  WsjcppEmployeesInit employees({}, false);
  if (!employees.initialized) {
    return -1;
  }

  auto *pConfig = findWsjcppEmploy<mldl::config>();

  for (int i = 0; i < vPossibleFolders.size(); i++) {
    std::string sWorkDir = vPossibleFolders[i];
    if (sWorkDir[0] != '/') {
      sWorkDir = WsjcppCore::getCurrentDirectory() + "/" + sWorkDir;
    }
    sWorkDir = wsjcpp::normalize_filepath(sWorkDir);
    if (wsjcpp::file_exists(sWorkDir + "/config.yml")) {
      std::cout << "Automatically detected workdir: " << sWorkDir << std::endl;
      pConfig->set_data_dir(sWorkDir);
      try_apply_mldl_user(sWorkDir);
      break;
    }
  }

  WsjcppLog::ok(TAG, "Starting scoreboard on http://localhost:" + std::to_string(pConfig->web_port()) + "/");

  WebServer httpServer;
  hv::HttpService *pRouter = httpServer.getService();
  hv::HttpServer server(pRouter);
  server.setPort(pConfig->web_port());
  server.setThreadNum(4);
  server.run();

  // // websocket_server_t server;
  // // server.service = pRouter;
  // // server.port = 12345;
  // // // server.ws = pWs;
  // // websocket_server_run(&server);

  return 0;
}
