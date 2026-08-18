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

#include "mldl/include/config.h"
#include "mldl/include/webhooks.h"
#include <wsjcpp_core.h>
#include <wsjcpp_employees.h>
#include <wsjcpp_yaml.h>

class EmployConfig : public WsjcppEmployBase, public mldl::config {
public:
  EmployConfig();
  static std::string name() {
    return "EmployConfig";
  }

  virtual bool init(const std::string &sName, bool bSilent) override;
  virtual bool deinit(const std::string &sName, bool bSilent) override;

  // mldl::config
  virtual void set_data_dir(const std::string sConfigDir) override;
  virtual const std::string &html_folder() const override;
  virtual int web_port() const override;
  virtual std::map<std::string, std::string> web_sites() const override;
  virtual std::map<std::string, std::shared_ptr<mldl::repository>> repositories() const override;
  virtual std::map<std::string, std::shared_ptr<mldl::repository>> webhooks() const override;

private:
  std::string TAG;
  std::string m_sConfigDir;
  std::string m_sHtmlFolder;
  int m_nPort;
  std::map<std::string, std::string> m_web_sites;
  std::map<std::string, std::shared_ptr<mldl::repository>> m_repositories;
  std::map<std::string, std::shared_ptr<mldl::repository>> m_webhooks;
};

REGISTRY_WSJCPP_EMPLOY(EmployConfig)

EmployConfig::EmployConfig() : WsjcppEmployBase({mldl::config::name()}, {mldl::webhooks::name()}) {
  TAG = "EmployConfig";
}

bool EmployConfig::init(const std::string &sName, bool bSilent) {
  if (!bSilent) {
    WsjcppLog::info(TAG, "init");
  }
  return true;
}

bool EmployConfig::deinit(const std::string &sName, bool bSilent) {
  if (!bSilent) {
    WsjcppLog::info(TAG, "deinit");
  }
  return true;
}

void EmployConfig::set_data_dir(const std::string sConfigDir) {
  WsjcppLog::info(TAG, "setDataDir: " + sConfigDir);
  m_sConfigDir = sConfigDir;
  m_sHtmlFolder = "";
  std::string sConfigFile = sConfigDir + "/config.yml";
  if (!wsjcpp::file_exists(sConfigFile)) {
    WsjcppLog::throw_err(TAG, "File not found " + sConfigFile);
  }

  WsjcppYaml yaml;
  std::string sError;
  if (!yaml.loadFromFile(sConfigFile, sError)) {
    WsjcppLog::throw_err(TAG, "Failed parsing yaml: " + sError);
  }
  std::string sHtmlFolder = yaml["html-folder"].valStr();
  if (sHtmlFolder == "") {
    WsjcppLog::throw_err(TAG, "Missing option html-folder in " + sConfigFile);
  }
  if (sHtmlFolder != "/") {
    sHtmlFolder = wsjcpp::normalize_filepath(m_sConfigDir + "/" + sHtmlFolder);
  }
  if (!wsjcpp::dir_exists(sHtmlFolder)) {
    WsjcppLog::throw_err(TAG, "Folder not found " + sConfigFile);
  }
  m_sHtmlFolder = sHtmlFolder;
  WsjcppLog::info(TAG, "Html Folder: " + m_sHtmlFolder);

  m_nPort = yaml["port"].valInt();

  // repositories
  if (yaml["repositories"].isMap()) {
    std::string repos_dir = m_sConfigDir + "/repositories";
    if (!wsjcpp::dir_exists(repos_dir)) {
      WsjcppCore::makeDirsPath(repos_dir);
    }
    std::vector<std::string> repositories = yaml["repositories"].keys();
    for (int i = 0; i < repositories.size(); i++) {
      std::shared_ptr<mldl::repository> repo = std::make_shared<mldl::repository>();
      std::string key = repositories[i];
      WsjcppLog::info(TAG, "Registered repository key: " + key);
      WsjcppYamlCursor cur = yaml["repositories"][key];
      if (!repo->read_from_yaml(key, cur)) {
        WsjcppLog::info(TAG, "Skip repository: " + key);
        continue;
      }
      repo->set_repo_folder(repos_dir + "/" + key);
      std::string git_repo = cur["url"];
      WsjcppLog::info(TAG, "Registered repository: " + git_repo);
      if (!wsjcpp::dir_exists(repo->repo_folder())) {
        std::string command = "git clone " + repo->url() + " " + repo->repo_folder();
        system(command.c_str());
      }
      m_repositories[key] = repo;

      findWsjcppEmploy<mldl::webhooks>()->registry_webhook(std::make_shared<mldl::webhook>(repo->webhook_update()));
      m_webhooks[repo->webhook_update()] = repo;
    }
  }

  // web-sites
  std::string web_sites_dir = m_sConfigDir + "/web-sites";
  if (!wsjcpp::dir_exists(web_sites_dir)) {
    WsjcppCore::makeDirsPath(web_sites_dir);
  }
  std::vector<std::string> web_sites = yaml["web-sites"].keys();
  for (int i = 0; i < web_sites.size(); i++) {
    std::string key = web_sites[i];
    WsjcppYamlCursor cur = yaml["web-sites"][key];
    std::string web_site_folder = web_sites_dir + "/" + key;
    std::string git_repo = cur["git-repository"];
    if (!wsjcpp::dir_exists(web_site_folder)) {
      std::string command = "git clone " + git_repo + " " + web_site_folder;
      system(command.c_str());
    }
    m_web_sites[key] = web_site_folder + "/" + cur["html-folder"].valStr();
  }
}

const std::string &EmployConfig::html_folder() const {
  return m_sHtmlFolder;
}

int EmployConfig::web_port() const {
  return m_nPort;
}

std::map<std::string, std::string> EmployConfig::web_sites() const {
  return m_web_sites;
}

std::map<std::string, std::shared_ptr<mldl::repository>> EmployConfig::repositories() const {
  return m_repositories;
}

std::map<std::string, std::shared_ptr<mldl::repository>> EmployConfig::webhooks() const {
  return m_webhooks;
}

// void EmployMyImpl::doSomething() {
//     WsjcppLog::info(TAG, "doSomething");
// }

// void EmployMyImpl::doSomething2() {
//     WsjcppLog::info(TAG, "doSomething2");
// }
