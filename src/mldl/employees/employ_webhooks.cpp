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

#include "mldl/include/webhooks.h"
#include <memory>
#include <mutex>
#include <sea5kg_logger.h>
#include <wsjcpp_core.h>
#include <wsjcpp_employees.h>
#include <wsjcpp_yaml.h>

class employ_webhooks : public WsjcppEmployBase, public mldl::webhooks {
public:
  employ_webhooks();

  // WsjcppEmployBase
  virtual bool init(const std::string &sName, bool bSilent) override;
  virtual bool deinit(const std::string &sName, bool bSilent) override;

  // webhooks
  virtual bool registry_webhook(std::shared_ptr<mldl::webhook> webhook) override;
  virtual bool contains(const std::string &webhook_id) const override;
  virtual bool call(const std::string &webhook_id) override;

private:
  std::string TAG;
  std::mutex m_mutex;
  std::map<std::string, std::shared_ptr<mldl::webhook>> m_webhooks;
};

REGISTRY_WSJCPP_EMPLOY(employ_webhooks)

employ_webhooks::employ_webhooks() : WsjcppEmployBase({employ_webhooks::name()}, {}) {
  TAG = "WEBHOOKS";
}

bool employ_webhooks::init(const std::string &sName, bool bSilent) {
  if (!bSilent) {
    sea5kg::log::info(TAG, "init");
  }
  return true;
}

bool employ_webhooks::deinit(const std::string &sName, bool bSilent) {
  if (!bSilent) {
    sea5kg::log::info(TAG, "deinit");
  }
  return true;
}

bool employ_webhooks::registry_webhook(std::shared_ptr<mldl::webhook> webhook) {
  std::lock_guard lock(m_mutex);
  if (m_webhooks.count(webhook->id()) > 0) {
    std::cerr << "Webhook already registered with id: " << webhook->id() << std::endl;
    return false;
  }
  m_webhooks[webhook->id()] = webhook;
  std::cout << "Webhook registered with id " << webhook->id() << std::endl;
  return true;
}

bool employ_webhooks::contains(const std::string &webhook_id) const {
  return m_webhooks.count(webhook_id) > 0;
}

bool employ_webhooks::call(const std::string &webhook_id) {
  std::lock_guard lock(m_mutex);
  if (m_webhooks.count(webhook_id) > 0) {
    return m_webhooks[webhook_id]->call();
  }
  return false;
}
