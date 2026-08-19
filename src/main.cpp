/**********************************************************************************
 *
 *        ▜ ▘▗ ▗ ▜      ▌      ▜   ▌
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
 *
 ***********************************************************************************/

#include "mldl/include/config.h"
#include "mldl/include/web_server.h"
#include <sea5kg_logger.h>
#include <unistd.h>
#include <wsjcpp_core.h>
#include <wsjcpp_employees.h>

bool try_apply_mldl_user(const std::string &work_dir, std::string &error) {
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
    if (wsjcpp::user_is_root()) {
      std::cout << " ...Try change owner for '" << work_dir << "' to '" << str_user << ":" << str_user << "'"
                << std::endl;
      std::string cmd = "chown -R " + std::to_string(user_id) + ":" + std::to_string(user_id) + " \"" + work_dir + "\"";
      if (system(cmd.c_str()) == 0) {
        std::cout << " -> OK. Successful changed owner for data." << std::endl;
      } else {
        std::cerr << " -> FAIL. Could not change owner for directory." << std::endl;
        return false;
      }
      return wsjcpp::change_current_process_privileges(user_id, error);
    } else if (geteuid() == user_id) {
      std::cout << " * OK. MLDL_USER is equal with current user" << std::endl;
    } else {
      return wsjcpp::change_current_process_privileges(user_id, error);
    }
    return true;
  }
  return true;
}

int main(int argc, const char *argv[]) {
  std::string TAG = "MAIN";
  std::string appName = std::string(WSJCPP_APP_NAME);
  std::string appVersion = std::string(WSJCPP_APP_VERSION);
  WsjcppLog::setEnableLogFile(false);

  std::cout << "" << std::endl;
  std::cout << "       ▜ ▘▗ ▗ ▜      ▌      ▜   ▌ " << std::endl;
  std::cout << "▛▛▌▌▌  ▐ ▌▜▘▜▘▐ █▌  ▛▌█▌▌▌  ▐ ▀▌▛▌" << std::endl;
  std::cout << "▌▌▌▙▌  ▐▖▌▐▖▐▖▐▖▙▖  ▙▌▙▖▚▘  ▐▖█▌▙▌" << std::endl;
  std::cout << "   ▄▌                             " << std::endl;
  std::cout << "" << std::endl;

  // try find config.yml
  const std::vector<std::string> vPossibleFolders = {"./data", "/root/data/"};
  std::string data_dir = "";
  for (int i = 0; i < vPossibleFolders.size(); i++) {
    data_dir = vPossibleFolders[i];
    if (data_dir[0] != '/') {
      data_dir = WsjcppCore::getCurrentDirectory() + "/" + data_dir;
    }
    data_dir = wsjcpp::normalize_filepath(data_dir);
    if (wsjcpp::file_exists(data_dir + "/config.yml")) {
      std::cout << "Automatically detected data-dir: " << data_dir << std::endl;
      break;
    }
  }

  if (data_dir == "") {
    sea5kg::log::critical(TAG, "Not found data-dir");
  }
  std::string error;
  if (!try_apply_mldl_user(data_dir, error)) {
    sea5kg::log::info(TAG, "try_apply_mldl_user: " + error);
  }

  std::string log_dir = data_dir + "/logs/%Y/%m/%d/";
  log_dir = wsjcpp::normalize_filepath(log_dir);
  sea5kg::log::set_log_level_file_output(sea5kg::log_level::DEBUG);
  sea5kg::log::set_log_dirpath(log_dir);
  sea5kg::log::set_log_filename_prefix("mldl_");

  WsjcppEmployeesInit employees({}, false);
  if (!employees.initialized) {
    return -1;
  }

  auto *pConfig = findWsjcppEmploy<mldl::config>();
  pConfig->set_data_dir(data_dir);

  sea5kg::log::success(TAG, "Starting scoreboard on http://localhost:" + std::to_string(pConfig->web_port()) + "/");

  auto *web = findWsjcppEmploy<mldl::web_server>();

  return web->start();
}
