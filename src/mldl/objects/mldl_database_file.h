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

#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <string>

namespace mldl {

class database_file;

class database_file_update_info {
public:
  database_file_update_info(const std::string &sVersionFrom, const std::string &sVersionTo,
                            const std::string &sDescription);
  const std::string &versionFrom() const;
  const std::string &versionTo() const;
  const std::string &description() const;

private:
  std::string m_sVersionFrom;
  std::string m_sVersionTo;
  std::string m_sDescription;
};

class database_file_update {
public:
  database_file_update(const std::string &sVersionFrom, const std::string &sVersionTo, const std::string &sDescription);
  const database_file_update_info &info();
  void setWeight(int nWeight);
  int getWeight();
  virtual bool applyUpdate(database_file *pDatabaseFile) = 0;

protected:
  std::string TAG;

private:
  database_file_update_info m_updateInfo;
  int m_nWeight;
};

extern std::map<std::string, database_file *> *g_opened_database_files;

class global_databases {
public:
  static void add_opened_database_file(const std::string &name, database_file *db);
  static bool init_driver_sqlite3(int &ret);
  static void shutdown_driver_sqlite3();
};

class database_select_rows {
public:
  virtual bool next() = 0;
  virtual std::string getString(int nColumnNumber) = 0;
  virtual long getLong(int nColumnNumber) = 0;
};

class database_file {
public:
  database_file(const std::string &db_dir, const std::string &filename);
  ~database_file();
  bool open();
  void close();
  bool executeQuery(std::string sSqlInsert);
  int selectSumOrCount(std::string sSqlSelectCount);
  std::shared_ptr<database_select_rows> selectRows(std::string sqlSelectRows);

private:
  void copy_database_to_backup();
  std::mutex m_mutex;

  std::string TAG;
  void *m_database_file_db;
  std::string m_filename;
  std::string m_sFileFullpath;
  std::string m_sBaseFileBackupFullpath;
  int m_nLastBackupTime;
};

} // namespace mldl