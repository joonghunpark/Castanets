/*
 * Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "wrt/common/app_db.h"

#include <app_common.h>
#include <unistd.h>
#include <fstream>
#include <memory>

#include "base/logging.h"
#include "wrt/common/app_db_sqlite.h"
#include "wrt/common/file_utils.h"
#include "wrt/common/picojson.h"
#include "wrt/common/string_utils.h"

namespace common {

const char* kCreateDbQuery =
    "CREATE TABLE IF NOT EXISTS appdb ("
    "section TEXT, "
    "key TEXT, "
    "value TEXT,"
    "PRIMARY KEY(section, key));";

SqliteDB::SqliteDB(const std::string& app_data_path)
    : app_data_path_(app_data_path), sqldb_(NULL) {
  if (app_data_path_.empty()) {
    std::unique_ptr<char, decltype(std::free)*> path{app_get_data_path(),
                                                     std::free};
    if (path.get() != NULL)
      app_data_path_ = path.get();
  }
  Initialize();
}

SqliteDB::~SqliteDB() {
  if (sqldb_ != NULL) {
    sqlite3_close(sqldb_);
    sqldb_ = NULL;
  }
}

void SqliteDB::MigrationAppdb() {
  // file check
  std::string migration_path = app_data_path_ + ".runtime.migration";
  if (!common::utils::Exists(migration_path)) {
    return;
  } else {
    LOG(INFO) << "Migration file found : " << migration_path;
  }

  std::ifstream migration_file(migration_path);
  if (!migration_file.is_open()) {
    LOG(ERROR) << "Fail to open file";
    return;
  }
  picojson::value v;
  std::string err;
  err = picojson::parse(v, migration_file);
  if (!err.empty()) {
    LOG(ERROR) << "Fail to parse file :" << err;
    return;
  }

  LOG(INFO) << "Start to get list data";

  picojson::array data_list;

  if (!v.get("preference").is<picojson::null>()) {
    picojson::array preference_list =
        v.get("preference").get<picojson::array>();
    data_list.insert(data_list.end(), preference_list.begin(),
                     preference_list.end());
  }
  if (!v.get("certificate").is<picojson::null>()) {
    picojson::array certificate_list =
        v.get("certificate").get<picojson::array>();
    data_list.insert(data_list.end(), certificate_list.begin(),
                     certificate_list.end());
  }
  if (!v.get("security_origin").is<picojson::null>()) {
    picojson::array security_origin_list =
        v.get("security_origin").get<picojson::array>();
    data_list.insert(data_list.end(), security_origin_list.begin(),
                     security_origin_list.end());
  }

  for (auto it = data_list.begin(); it != data_list.end(); ++it) {
    if (!it->is<picojson::object>())
      continue;
    std::string section = it->get("section").to_str();
    std::string key = it->get("key").to_str();
    std::string value = it->get("value").to_str();

    LOG(INFO) << "INPUT[" << section << "][" << key << "][" << value << "]";
    Set(section, key, value);
  }

  LOG(INFO) << "Migration complete";

  if (0 != remove(migration_path.c_str())) {
    LOG(ERROR) << "Fail to remove migration file";
  }
}

void SqliteDB::Initialize() {
  if (app_data_path_.empty()) {
    LOG(ERROR) << "app data path was empty";
    return;
  }
  std::string db_path = app_data_path_ + "/.appdb.db";
  int ret = sqlite3_open(db_path.c_str(), &sqldb_);
  if (ret != SQLITE_OK) {
    LOG(ERROR) << "Fail to open app db :" << sqlite3_errmsg(sqldb_);
    sqldb_ = NULL;
    return;
  }
  sqlite3_busy_handler(sqldb_,
                       [](void*, int count) {
                         if (count < 5) {
                           LOG(ERROR) << "App db was busy, Wait the lock count("
                                      << count << ")";
                           usleep(100000 * (count + 1));
                           return 1;
                         } else {
                           LOG(ERROR) << "App db was busy, Fail to access";
                           return 0;
                         }
                       },
                       NULL);

  char* errmsg = NULL;
  ret = sqlite3_exec(sqldb_, kCreateDbQuery, NULL, NULL, &errmsg);
  if (ret != SQLITE_OK) {
    LOG(ERROR) << "Error to create appdb : " << (errmsg ? errmsg : "");
    if (errmsg)
      sqlite3_free(errmsg);
  }
  MigrationAppdb();
}

bool SqliteDB::HasKey(const std::string& section,
                      const std::string& key) const {
  char* buffer = NULL;
  sqlite3_stmt* stmt = NULL;
  bool result = false;

  int ret = 0;
  buffer = sqlite3_mprintf(
      "select count(*) from appdb where section = %Q and key = %Q",
      section.c_str(), key.c_str());
  if (buffer == NULL) {
    LOG(ERROR) << "error to make query";
    return false;
  }

  std::unique_ptr<char, decltype(sqlite3_free)*> scoped_data{buffer,
                                                             sqlite3_free};

  ret = sqlite3_prepare(sqldb_, buffer, strlen(buffer), &stmt, NULL);
  if (ret != SQLITE_OK) {
    LOG(ERROR) << "Fail to prepare query : " << sqlite3_errmsg(sqldb_);
    return false;
  }

  ret = sqlite3_step(stmt);
  if (ret == SQLITE_ROW) {
    int value = sqlite3_column_int(stmt, 0);
    result = value > 0;
  }

  sqlite3_finalize(stmt);
  return result;
}

std::string SqliteDB::Get(const std::string& section,
                          const std::string& key) const {
  char* buffer = NULL;
  sqlite3_stmt* stmt = NULL;
  std::string result;

  int ret = 0;
  buffer =
      sqlite3_mprintf("select value from appdb where section = %Q and key = %Q",
                      section.c_str(), key.c_str());
  if (buffer == NULL) {
    LOG(ERROR) << "error to make query";
    return result;
  }

  std::unique_ptr<char, decltype(sqlite3_free)*> scoped_data{buffer,
                                                             sqlite3_free};

  ret = sqlite3_prepare(sqldb_, buffer, strlen(buffer), &stmt, NULL);
  if (ret != SQLITE_OK) {
    LOG(ERROR) << "Fail to prepare query : " << sqlite3_errmsg(sqldb_);
    return result;
  }

  ret = sqlite3_step(stmt);
  if (ret == SQLITE_ROW) {
    result = std::string(
        reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
  }

  sqlite3_finalize(stmt);
  return result;
}

void SqliteDB::Set(const std::string& section,
                   const std::string& key,
                   const std::string& value) {
  char* buffer = NULL;
  sqlite3_stmt* stmt = NULL;

  int ret = 0;
  buffer = sqlite3_mprintf(
      "replace into appdb (section, key, value) values (?, ?, ?);");
  if (buffer == NULL) {
    LOG(ERROR) << "error to make query";
    return;
  }

  std::unique_ptr<char, decltype(sqlite3_free)*> scoped_data{buffer,
                                                             sqlite3_free};

  ret = sqlite3_prepare(sqldb_, buffer, strlen(buffer), &stmt, NULL);
  if (ret != SQLITE_OK) {
    LOG(ERROR) << "Fail to prepare query : " << sqlite3_errmsg(sqldb_);
    return;
  }

  std::unique_ptr<sqlite3_stmt, decltype(sqlite3_finalize)*> scoped_stmt{
      stmt, sqlite3_finalize};

  ret = sqlite3_bind_text(stmt, 1, section.c_str(), section.length(),
                          SQLITE_STATIC);
  if (ret != SQLITE_OK) {
    LOG(ERROR) << "Fail to prepare query bind argument : "
               << sqlite3_errmsg(sqldb_);
    return;
  }
  ret = sqlite3_bind_text(stmt, 2, key.c_str(), key.length(), SQLITE_STATIC);
  if (ret != SQLITE_OK) {
    LOG(ERROR) << "Fail to prepare query bind argument : "
               << sqlite3_errmsg(sqldb_);
    return;
  }
  ret =
      sqlite3_bind_text(stmt, 3, value.c_str(), value.length(), SQLITE_STATIC);
  if (ret != SQLITE_OK) {
    LOG(ERROR) << "Fail to prepare query bind argument : "
               << sqlite3_errmsg(sqldb_);
    return;
  }
  ret = sqlite3_step(stmt);
  if (ret != SQLITE_DONE) {
    LOG(ERROR) << "Fail to insert data : " << sqlite3_errmsg(sqldb_);
  }
}

void SqliteDB::Remove(const std::string& section, const std::string& key) {
  char* buffer = NULL;

  buffer = sqlite3_mprintf("delete from appdb where section = %Q and key = %Q",
                           section.c_str(), key.c_str());

  if (buffer == NULL) {
    LOG(ERROR) << "error to make query";
    return;
  }

  std::unique_ptr<char, decltype(sqlite3_free)*> scoped_data{buffer,
                                                             sqlite3_free};

  char* errmsg = NULL;
  int ret = sqlite3_exec(sqldb_, buffer, NULL, NULL, &errmsg);
  if (ret != SQLITE_OK) {
    LOG(ERROR) << "Error to delete value : " << (errmsg ? errmsg : "");
    if (errmsg)
      sqlite3_free(errmsg);
  }
}

void SqliteDB::GetKeys(const std::string& section,
                       std::list<std::string>* keys) const {
  char* buffer = NULL;
  sqlite3_stmt* stmt = NULL;

  int ret = 0;
  buffer = sqlite3_mprintf("select key from appdb where section = %Q",
                           section.c_str());
  if (buffer == NULL) {
    LOG(ERROR) << "error to make query";
    return;
  }

  std::unique_ptr<char, decltype(sqlite3_free)*> scoped_data{buffer,
                                                             sqlite3_free};

  ret = sqlite3_prepare(sqldb_, buffer, strlen(buffer), &stmt, NULL);
  if (ret != SQLITE_OK) {
    LOG(ERROR) << "Fail to prepare query : " << sqlite3_errmsg(sqldb_);
    return;
  }

  ret = sqlite3_step(stmt);
  while (ret == SQLITE_ROW) {
    const char* value =
        reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    keys->push_back(std::string(value));
    ret = sqlite3_step(stmt);
  }

  sqlite3_finalize(stmt);
  return;
}

AppDB* AppDB::GetInstance() {
  static SqliteDB instance;
  return &instance;
}

}  // namespace common
