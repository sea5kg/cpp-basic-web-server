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
#include "mldl/include/globals.h"
#include "mldl/include/uuid.h"
#include "mldl/objects/mldl_database_file.h"
#include <algorithm>
#include <sea5kg_logger.h>
#include <wsjcpp_core.h>
#include <wsjcpp_employees.h>

class employ_uuid : public WsjcppEmployBase, public mldl::uuid {
public:
  employ_uuid();

  virtual bool init(const std::string &sName, bool bSilent) override;
  virtual bool deinit(const std::string &sName, bool bSilent) override;

  // mldl::uuid
  virtual void add_allowed_type(const std::string &type_of_object) override;
  virtual const std::vector<std::string> &allowed_types() override;
  virtual std::string generate_new_uuid(const std::string &type_of_object) override;
  virtual bool has(const std::string &sUuid) override;
  virtual std::string type_of_object(const std::string &sUuid) override;
  virtual bool remove_uuid(const std::string &sUuid) override;

private:
  std::string TAG;
  std::mutex m_mutex;
  std::vector<std::string> m_allowed_types;
  std::map<std::string, std::string> m_global_uuids;
  std::shared_ptr<mldl::database_file> m_db;
};

REGISTRY_WSJCPP_EMPLOY(employ_uuid)

employ_uuid::employ_uuid() : WsjcppEmployBase({mldl::uuid::name()}, {mldl::config::name()}) {
  TAG = "employ_uuid";
}

bool employ_uuid::init(const std::string &sName, bool silent) {
  if (!silent) {
    sea5kg::log::info(TAG, "init");
  }
  auto config = findWsjcppEmploy<mldl::config>();
  m_db = std::make_shared<mldl::database_file>(config->database_dir(), "uuids.db");
  return m_db->open();
}

bool employ_uuid::deinit(const std::string &sName, bool bSilent) {
  if (!bSilent) {
    sea5kg::log::info(TAG, "deinit");
  }
  return true;
}

void employ_uuid::add_allowed_type(const std::string &sTypeOfObject) {
  if (std::find(m_allowed_types.begin(), m_allowed_types.end(), sTypeOfObject) == m_allowed_types.end()) {
    WsjcppLog::info(TAG, "Registered new type of object '" + sTypeOfObject + "'");
    m_allowed_types.push_back(sTypeOfObject);
  } else {
    WsjcppLog::info(TAG, "Already registered type of object '" + sTypeOfObject + "'");
  }
}

const std::vector<std::string> &employ_uuid::allowed_types() {
  return m_allowed_types;
}

std::string employ_uuid::generate_new_uuid(const std::string &type_of_object) {
  std::lock_guard<std::mutex> lock(m_mutex);
  std::string uuid = wsjcpp::generate_uuid();
  while (m_global_uuids.count(uuid)) {
    WsjcppLog::warn(TAG, "Regenerate uuid again");
    uuid = wsjcpp::generate_uuid();
  }
  m_global_uuids[uuid] = type_of_object;
  // auto *pDb = findWsjcppEmploy<EmployDatabase>();
  // if (!pDb->dbUuids()->insertUuid(sUuid, sTypeOfObject)) {
  //   WsjcppLog::throw_err(TAG, "Problem with insert to database");
  // }
  return uuid;
}

bool employ_uuid::has(const std::string &uuid) {
  return m_global_uuids.count(uuid);
}

std::string employ_uuid::type_of_object(const std::string &uuid) {
  if (m_global_uuids.count(uuid)) {
    return m_global_uuids[uuid];
  }
  return "unknown";
}

bool employ_uuid::remove_uuid(const std::string &uuid) {
  if (m_global_uuids.count(uuid)) {
    m_global_uuids.erase(uuid);
    // auto *pDb = findWsjcppEmploy<EmployDatabase>();
    // if (!pDb->dbUuids()->deleteUuid(sUuid)) {
    //   return false;
    //   // WsjcppLog::throw_err(TAG, "Problem with delete uuid");
    // }
  }
  return true;
}
