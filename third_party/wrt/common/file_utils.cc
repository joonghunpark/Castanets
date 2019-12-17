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

#include "wrt/common/file_utils.h"

#include <aul.h>
#include <sys/types.h>
#include <libgen.h>
#include <unistd.h>

#include <algorithm>
#include <sstream>
#include <string>

#include "url/url_constants.h"
#include "wrt/common/string_utils.h"

namespace common {
namespace utils {

bool Exists(const std::string& path) {
  return (access(path.c_str(), F_OK) != -1);
}

std::string BaseName(const std::string& path) {
  char* p = basename(const_cast<char*>(path.c_str()));
  return std::string(p);
}

std::string DirName(const std::string& path) {
  char* p = dirname(const_cast<char*>(path.c_str()));
  return std::string(p);
}

std::string SchemeName(const std::string& uri) {
  size_t pos = uri.find(":");
  if (pos != std::string::npos && pos < uri.length()) {
    return std::string(uri.substr(0, pos));
  } else {
    return uri;
  }
}

std::string ExtName(const std::string& path) {
  size_t last_dot = path.find_last_of(".");
  if (last_dot != 0 && last_dot != std::string::npos) {
    std::string ext = path.substr(last_dot);
    size_t end_of_ext = ext.find_first_of("?#");
    if (end_of_ext != std::string::npos)
      ext = ext.substr(0, end_of_ext);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
  } else {
    return std::string();
  }
}

std::string GetUserRuntimeDir() {
  uid_t uid = getuid();
  std::stringstream ss;
  ss << "/run/user/" << uid;
  std::string path = ss.str();
  if (!Exists(path)) {
    path = "/tmp";
  }
  return path;
}

std::string SkipFileScheme(const std::string& url, bool* is_file_scheme) {
  std::string file_scheme = std::string(url::kFileScheme) +
                              std::string(url::kStandardSchemeSeparator);
  bool has_file_scheme = StartsWith(url, file_scheme);
  if (is_file_scheme)
    *is_file_scheme = has_file_scheme;
  return has_file_scheme ? url.substr(file_scheme.length()) : url;
}

std::string GetMimeFromFile(const std::string& url) {
  int ret = AUL_R_EINVAL;
  char mimetype[128] = { 0, };
  bool is_file_scheme;
  std::string path = SkipFileScheme(url, &is_file_scheme);
  if (is_file_scheme || StartsWith(path, "/"))
    ret = aul_get_mime_from_file(path.c_str(), mimetype, sizeof(mimetype));
  return (ret == AUL_R_OK) ? std::string(mimetype) : std::string();
}

}  // namespace utils
}  // namespace common
