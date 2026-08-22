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
#include "mldl/include/webhooks.h"
#include "mldl/objects/mldl_webhook_git_repo_update.h"
#include <sea5kg_logger.h>
#include <wsjcpp_core.h>
#include <wsjcpp_employees.h>
#include <wsjcpp_yaml.h>

class employ_config : public WsjcppEmployBase, public mldl::config {
public:
  employ_config();
  static std::string name() {
    return "CONFIG";
  }

  virtual bool init(const std::string &sName, bool bSilent) override;
  virtual bool deinit(const std::string &sName, bool bSilent) override;

  // mldl::config
  virtual void set_data_dir(const std::string sConfigDir) override;
  virtual const std::string &data_dir() override;
  virtual int web_port() const override;
  virtual std::map<std::string, std::map<std::string, std::string>> mapping() const override;
  virtual std::map<std::string, std::shared_ptr<mldl::repository>> repositories() const override;

private:
  bool read_mapping_host_and_path_folders(WsjcppYaml &yaml);

  std::string TAG;
  std::string m_sConfigDir;
  int m_nPort;
  std::map<std::string, std::map<std::string, std::string>> m_mapping;
  std::map<std::string, std::shared_ptr<mldl::repository>> m_repositories;
};

REGISTRY_WSJCPP_EMPLOY(employ_config)

employ_config::employ_config() : WsjcppEmployBase({mldl::config::name()}, {mldl::webhooks::name()}) {
  TAG = "employ_config";
}

bool employ_config::init(const std::string &sName, bool bSilent) {
  if (!bSilent) {
    sea5kg::log::info(TAG, "init");
  }
  return true;
}

bool employ_config::deinit(const std::string &sName, bool bSilent) {
  if (!bSilent) {
    sea5kg::log::info(TAG, "deinit");
  }
  return true;
}

void employ_config::set_data_dir(const std::string data_dir) {
  sea5kg::log::info(TAG, "setDataDir: " + data_dir);
  m_sConfigDir = data_dir;
  std::string sConfigFile = data_dir + "/config.yml";
  if (!wsjcpp::file_exists(sConfigFile)) {
    sea5kg::log::critical(TAG, "File not found " + sConfigFile);
  }

  WsjcppYaml yaml;
  std::string sError;
  if (!yaml.loadFromFile(sConfigFile, sError)) {
    sea5kg::log::critical(TAG, "Failed parsing yaml: " + sError);
  }

  m_nPort = yaml["port"].valInt();

  // repositories
  if (yaml["repositories"].isMap()) {
    std::string repos_dir = m_sConfigDir + "/repositories";
    if (!wsjcpp::dir_exists(repos_dir)) {
      WsjcppCore::makeDirsPath(repos_dir);
    }
    std::vector<std::string> keys = yaml["repositories"].keys();
    for (int i = 0; i < keys.size(); i++) {
      std::shared_ptr<mldl::repository> repo = std::make_shared<mldl::repository>();
      std::string key = keys[i];
      sea5kg::log::info(TAG, "Registered repository key: " + key);
      WsjcppYamlCursor cur = yaml["repositories"][key];
      if (!repo->read_from_yaml(key, cur)) {
        sea5kg::log::info(TAG, "Skip repository: " + key);
        continue;
      }
      repo->set_repo_folder(repos_dir + "/" + key);
      std::string git_repo = cur["url"];
      sea5kg::log::info(TAG, "Registered repository: " + git_repo);
      if (!wsjcpp::dir_exists(repo->repo_folder())) {
        std::string command = "git clone " + repo->url() + " " + repo->repo_folder();
        if (system(command.c_str()) != 0) {
          sea5kg::log::critical(TAG, "Could not call command '" + command + "'");
        }
      }
      m_repositories[key] = repo;

      findWsjcppEmploy<mldl::webhooks>()->registry_webhook(std::make_shared<mldl::mldl_webhook_git_repo_update>(repo));
    }
  }

  if (!read_mapping_host_and_path_folders(yaml)) {
    sea5kg::log::critical(TAG, "Some problem with reading mapping");
  }
}

const std::string &employ_config::data_dir() {
  return m_sConfigDir;
}

int employ_config::web_port() const {
  return m_nPort;
}

std::map<std::string, std::map<std::string, std::string>> employ_config::mapping() const {
  return m_mapping;
}

std::map<std::string, std::shared_ptr<mldl::repository>> employ_config::repositories() const {
  return m_repositories;
}

bool employ_config::read_mapping_host_and_path_folders(WsjcppYaml &yaml) {

  // hardcoded root page for localhost
  m_mapping["localhost"] = std::map<std::string, std::string>();
  m_mapping["localhost"]["/"] = wsjcpp::normalize_filepath(m_sConfigDir + "/html/");;
  // m_mapping["localhost"]["/admin/"] = admin_path;

  std::string control_panel_path = wsjcpp::normalize_filepath(m_sConfigDir + "/html/control-panel");

  if (yaml["mapping-host-and-path-folders"].isMap()) {
    std::vector<std::string> keys = yaml["mapping-host-and-path-folders"].keys();
    for (int i = 0; i < keys.size(); i++) {
      std::string key = keys[i];
      std::string host = key;
      if (!wsjcpp::ends_with(host, "/")) {
        host += "/";
      }
      std::vector<std::string> vHost = wsjcpp::split(host, "/");
      host = vHost[0];
      std::string path = key.substr(host.length(), key.length());
      if (!wsjcpp::ends_with(path, "/")) {
        path += "/";
      }
      std::string real_path = yaml["mapping-host-and-path-folders"][key].valStr();
      if (wsjcpp::starts_with(real_path, "./")) {
        real_path = wsjcpp::normalize_filepath(m_sConfigDir + "/" + real_path);
      }
      if (!wsjcpp::ends_with(real_path, "/")) {
        real_path += "/";
      }
      for (auto it = m_repositories.begin(); it != m_repositories.end(); ++it) {
        std::string repo_var = "${" + it->first + "}";
        wsjcpp::replace_all_in(real_path, repo_var, it->second->repo_folder());
      }
      std::cout << "real_path = " << real_path << std::endl;
      if (!wsjcpp::dir_exists(real_path)) {
        sea5kg::log::warning(TAG, "Not found directory: " + real_path);
        continue;
      }

      if (m_mapping.count(host) == 0) {
        sea5kg::log::info(TAG, "Init host-name: " + host);
        m_mapping[host] = std::map<std::string, std::string>();
        // hardcoded admin for any host
        m_mapping[host]["/control-panel"] = control_panel_path;
      }
      sea5kg::log::info(TAG, "Registered mapping: " + host + path + " -> " + real_path);
      if (!wsjcpp::dir_exists(real_path)) {
        sea5kg::log::critical(TAG, "Folder not found " + real_path);
      }
      m_mapping[host][path] = real_path;
    }
    return true;
  }
  return false;
}