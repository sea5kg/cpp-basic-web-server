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

#pragma once

#include <wsjcpp_employees.h>
#include "employ_config.h"
#include "mldl/objects/mldl_repository.h"

class EmployConfig : public WsjcppEmployBase { //, public IMyImpl, public IMyImpl2 {
public:
  EmployConfig();
  static std::string name() { return "EmployConfig"; }

  virtual bool init(const std::string &sName, bool bSilent) override;
  virtual bool deinit(const std::string &sName, bool bSilent) override;

  void setDataDir(const std::string sConfigDir);
  const std::string &getHtmlFolder() const;
  int getPort() const;

  std::map<std::string, std::string> web_sites() const;
  std::map<std::string, mldl::repository> repositories() const;

  // // IMyImpl
  // virtual void doSomething() override;

  // // IMyImpl2
  // virtual void doSomething2() override;

private:
  std::string TAG;
  std::string m_sConfigDir;
  std::string m_sHtmlFolder;
  int m_nPort;
  std::map<std::string, std::string> m_web_sites;
  std::map<std::string, mldl::repository> m_repositories;
};
