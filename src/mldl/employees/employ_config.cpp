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
#include "mldl/include/globals.h"
#include "mldl/objects/mldl_webhook_git_repo_update.h"
#include "third_party/smallsha1/smallsha1.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include <sea5kg_logger.h>
// #include <sys/stat.h>
#include <wsjcpp_core.h>
#include <wsjcpp_employees.h>
#include <wsjcpp_yaml.h>

class employ_config : public WsjcppEmployBase, public mldl::config {
public:
  employ_config();

  virtual bool init(const std::string &sName, bool bSilent) override;
  virtual bool deinit(const std::string &sName, bool bSilent) override;

  // mldl::config
  virtual void set_data_dir(const std::string sConfigDir) override;
  virtual const std::string &data_dir() override;
  virtual const std::string &database_dir() override;
  virtual int web_port() const override;
  virtual std::map<std::string, std::map<std::string, std::string>> mapping() const override;
  virtual std::map<std::string, std::shared_ptr<mldl::repository>> repositories() const override;

private:
  bool read_mapping_host_and_path_folders(WsjcppYaml &yaml);
  nlohmann::json load_files_sha1();
  void save_files_sha1(nlohmann::json &files);
  void update_data_html(nlohmann::json &files);
  void update_files_in_data();
  bool apply_port_from_env();

  std::string TAG;
  std::string m_sConfigDir;
  std::string m_database_dir;
  int m_web_port;
  std::map<std::string, std::map<std::string, std::string>> m_mapping;
  std::map<std::string, std::shared_ptr<mldl::repository>> m_repositories;
};

REGISTRY_WSJCPP_EMPLOY(employ_config)

employ_config::employ_config() : WsjcppEmployBase({mldl::config::name()}, {mldl::webhooks::name()}) {
  TAG = "employ_config";
  m_web_port = mldl::DEFAULT_WEB_PORT;
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

  this->update_files_in_data();

  std::string sConfigFile = data_dir + "/config.yml";
  if (!wsjcpp::file_exists(sConfigFile)) {
    sea5kg::log::critical(TAG, "File not found " + sConfigFile);
  }

  WsjcppYaml yaml;
  std::string sError;
  if (!yaml.loadFromFile(sConfigFile, sError)) {
    sea5kg::log::critical(TAG, "Failed parsing yaml: " + sError);
  }

  m_web_port = yaml[mldl::keys::PORT].valInt();
  apply_port_from_env();

  // repositories
  if (yaml[mldl::keys::REPOSITORIES].isMap()) {
    std::string repos_dir = m_sConfigDir + "/" + mldl::keys::REPOSITORIES;
    if (!wsjcpp::dir_exists(repos_dir)) {
      WsjcppCore::makeDirsPath(repos_dir);
    }
    std::vector<std::string> keys = yaml[mldl::keys::REPOSITORIES].keys();
    for (int i = 0; i < keys.size(); i++) {
      std::shared_ptr<mldl::repository> repo = std::make_shared<mldl::repository>();
      std::string key = keys[i];
      sea5kg::log::info(TAG, "Registered repository key: " + key);
      WsjcppYamlCursor cur = yaml[mldl::keys::REPOSITORIES][key];
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

  m_database_dir = wsjcpp::normalize_filepath(m_sConfigDir + "/db");
  if (!wsjcpp::dir_exists(m_database_dir)) {
    WsjcppCore::makeDirsPath(m_database_dir);
  }
}

const std::string &employ_config::data_dir() {
  return m_sConfigDir;
}

const std::string &employ_config::database_dir() {
  return m_database_dir;
}

int employ_config::web_port() const {
  return m_web_port;
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
  m_mapping["localhost"]["/"] = wsjcpp::normalize_filepath(m_sConfigDir + "/html/");
  ;
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

nlohmann::json employ_config::load_files_sha1() {
  nlohmann::json files_sha1;
  if (wsjcpp::file_exists(m_sConfigDir + "/files_sha1.json")) {
    std::ifstream ifs(m_sConfigDir + "/files_sha1.json");
    files_sha1 = nlohmann::json::parse(ifs);
  }
  return files_sha1;
}

void employ_config::save_files_sha1(nlohmann::json &files) {
  std::ofstream output(m_sConfigDir + "/files_sha1.json");
  output << std::setw(2) << files << std::endl;
}

std::string sha1_by_data(const char *data, int len) {
  char hexstring[41]; // 40 chars + a zero
  std::memset(hexstring, 0, sizeof hexstring);

  unsigned char hash[20];
  sha1::calc(data, len, hash);
  sha1::toHexString(hash, hexstring);
  return std::string(hexstring);
}

std::string sha1_by_file(const std::string &sFilename) {
  std::ifstream f(sFilename, std::ifstream::binary);
  if (!f) {
    return "Could not open file";
  }
  // get length of file:
  f.seekg(0, f.end);
  int nBufferSize = f.tellg();
  f.seekg(0, f.beg);
  char *pBuffer = new char[nBufferSize];
  // read data as a block:
  f.read(pBuffer, nBufferSize);
  if (!f) {
    delete[] pBuffer;
    // f.close();
    sea5kg::log::critical("sha1_by_file", "Could not read file. Only " + std::to_string(f.gcount()) + " could be read");
    return "";
  }
  f.close();
  char hexstring[41]; // 40 chars + a zero
  std::memset(hexstring, 0, sizeof hexstring);
  unsigned char hash[20];
  sha1::calc(pBuffer, nBufferSize, hash);
  sha1::toHexString(hash, hexstring);
  delete[] pBuffer;
  return std::string(hexstring);
}

void employ_config::update_data_html(nlohmann::json &previous_files_sha1) {
  sea5kg::log::warning(TAG, "Updating files in data/html");
  if (!wsjcpp::dir_exists(m_sConfigDir + "/html")) {
    WsjcppCore::makeDir(m_sConfigDir + "/html");
  }

  const std::vector<WsjcppResourceFile *> &vFiles = WsjcppResourcesManager::list();
  for (int i = 0; i < vFiles.size(); i++) {
    std::string source_filepath = vFiles[i]->getFilename();
    if (source_filepath.rfind("./data/html/", 0) != 0) {
      continue;
    }
    // remove base folder
    std::vector<std::string> vPath = wsjcpp::split(source_filepath, "/");
    vPath.erase(vPath.begin(), vPath.begin() + 3);
    std::string target_filepath = wsjcpp::join(vPath, "/");
    target_filepath = wsjcpp::normalize_filepath(m_sConfigDir + "/html/" + target_filepath);

    // prepare folders
    if (!wsjcpp::file_exists(target_filepath)) {
      std::string dirpath = wsjcpp::normalize_filepath(m_sConfigDir + "/html/");
      for (int p = 0; p < vPath.size() - 1; p++) {
        dirpath = wsjcpp::normalize_filepath(dirpath + "/" + vPath[p]);
        if (!wsjcpp::dir_exists(dirpath)) {
          if (!WsjcppCore::makeDir(dirpath)) {
            std::cout << "ERROR. Could not create: " << dirpath << std::endl;
            continue;
          }
        }
      }
    }

    std::string previous_sha1 = "";
    if (previous_files_sha1.contains(source_filepath)) {
      previous_sha1 = previous_files_sha1[source_filepath];
    }

    if (wsjcpp::file_exists(target_filepath) && previous_sha1 != "") {
      if (previous_sha1 != sha1_by_file(target_filepath)) {
        // Skip. file has changes by user. Skip.
        std::cout << "Warning. Could not override file, because has changes: " << target_filepath << std::endl;
        continue;
      }
    }

    std::string new_sha1 = sha1_by_data(vFiles[i]->getBuffer(), vFiles[i]->getBufferSize());
    if (wsjcpp::file_exists(target_filepath) && new_sha1 == previous_sha1) {
      // Skip. file has same content
      continue;
    }

    if (!WsjcppCore::writeFile(target_filepath, vFiles[i]->getBuffer(), vFiles[i]->getBufferSize())) {
      std::cout << "ERROR. Could not write/override file. " << std::endl;
      continue;
    }

    std::cout << "Successfully created/updated file: " << target_filepath << std::endl;
    std::string err;
    if (!WsjcppCore::setFilePermissions(target_filepath, WsjcppFilePermissions(0x644), err)) {
      sea5kg::log::critical(TAG, err);
    }
    previous_files_sha1[source_filepath] = new_sha1;
  }
}

void employ_config::update_files_in_data() {
  std::string sError;
  if (!wsjcpp::dir_exists(m_sConfigDir + "/logs")) {
    WsjcppCore::makeDir(m_sConfigDir + "/logs");
    if (!WsjcppCore::setFilePermissions(m_sConfigDir + "/logs", WsjcppFilePermissions(0x755), sError)) {
      sea5kg::log::critical(TAG, sError);
    }
  }

  nlohmann::json previous_files_sha1 = load_files_sha1();

  if (!wsjcpp::file_exists(m_sConfigDir + "/config.yml")) {
    sea5kg::log::info(TAG, "Extracting config.yml");
    // const std::vector<WsjcppResourceFile *> &vFiles = WsjcppResourcesManager::list();
    // std::vector<std::string> vExecutableFiles;
    // for (int i = 0; i < vFiles.size(); i++) {
    //   std::string filepath = vFiles[i]->getFilename();
    // }

    WsjcppResourceFile *pConfigYml = WsjcppResourcesManager::get("./data/config.yml");
    std::string sNewFilepath = wsjcpp::normalize_filepath(m_sConfigDir + "/config.yml");
    sea5kg::log::info(TAG, "write file: " + sNewFilepath);
    if (!WsjcppCore::writeFile(sNewFilepath, pConfigYml->getBuffer(), pConfigYml->getBufferSize())) {
      sea5kg::log::error(TAG, "ERROR. Could not write file. file=" + sNewFilepath);
    } else {
      sea5kg::log::success(TAG, "Successfully created file. file=" + sNewFilepath);
    }
  }
  if (!wsjcpp::file_exists(m_sConfigDir + ".gitignore")) {
    sea5kg::log::info(TAG, ".gitignore");
    WsjcppResourceFile *pConfigYml = WsjcppResourcesManager::get("./data/.gitignore");
    std::string sNewFilepath = wsjcpp::normalize_filepath(m_sConfigDir + "/.gitignore");
    sea5kg::log::info(TAG, "write file: " + sNewFilepath);
    if (!WsjcppCore::writeFile(sNewFilepath, pConfigYml->getBuffer(), pConfigYml->getBufferSize())) {
      sea5kg::log::error(TAG, "ERROR. Could not write file. file=" + sNewFilepath);
    } else {
      sea5kg::log::success(TAG, "Successfully created file. file=" + sNewFilepath);
    }
  }

  update_data_html(previous_files_sha1);
  save_files_sha1(previous_files_sha1);
}

bool employ_config::apply_port_from_env() {
  std::string str_port;

  if (WsjcppCore::getEnv(mldl::keys::MLDL_PORT, str_port)) {
    sea5kg::log::warning(TAG, mldl::keys::MLDL_PORT + "='" + str_port + "'");
    try {
      int port = std::stoi(str_port);
      std::string err;
      m_web_port = port;
      // if (!m_scoreboard_port->set_value(port, err)) {
      //   sea5kg::log::err(TAG, mldl::keys::MLDL_PORT + "='" + str_port + "' is wrong. " + err);
      //   return false;
      // }
    } catch (const std::invalid_argument& e) {
      sea5kg::log::error(TAG, "No conversion could be performed. " + mldl::keys::MLDL_PORT + "='" + str_port + "'");
      std::cerr << "Error: \n";
      return false;
    } catch (const std::out_of_range& e) {
      sea5kg::log::error(TAG, "The converted value is too big for an int.. " + mldl::keys::MLDL_PORT + "='" + str_port + "'");
      return false;
    } catch (...) {
      sea5kg::log::error(TAG, "The converted value is too big for an int.. " + mldl::keys::MLDL_PORT + "='" + str_port + "'");
      return false;
    }
    sea5kg::log::info(TAG, "scoreboard.port will be overridden from environment variable. " + mldl::keys::MLDL_PORT + "='" + str_port + "'");
    return true;
  }
  return true;
}