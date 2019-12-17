/*
 * Copyright (c) 2019 Samsung Electronics Co., Ltd All Rights Reserved
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

#include "wrt/common/privilege.h"

#include <cynara-client.h>
#include <cynara-error.h>
#include <cynara-session.h>
#include <pkgmgr-info.h>
#include <unistd.h>
#include <fstream>
#include <memory>

#include "base/logging.h"
#include "wrt/src/common/application_data.h"

namespace common {

namespace privilege {

namespace {

enum {
  STOP_PRIVILEGE_SEARCH = -1,
  KEEP_PRIVILEGE_SEARCH = 0
};

static bool allowed = false;
static constexpr char kSmackLabelFilePath[] = "/proc/self/attr/current";
static constexpr auto hostedapp_privilege =
    "http://developer.samsung.com/privilege/hostedapp_deviceapi_allow";

} // namespace

bool CheckHostedAppPrivilege(const std::string& pkg_id) {
  int ret = 0;

  pkgmgrinfo_pkginfo_h handle;
  ret = pkgmgrinfo_pkginfo_get_pkginfo(pkg_id.c_str(), &handle);
  if (ret != PMINFO_R_OK) {
    LOG(ERROR) << "pkgmgrinfo_pkginfo_get_pkginfo failed";
    return allowed;
  }

  auto find_privilege = [](const char *name, void *hostedapp_privilege) -> int {
    if (strcmp(name, reinterpret_cast<char*>(hostedapp_privilege)) == 0) {
      // The privilege is exist in the config.
      cynara *p_cynara = nullptr;
      if (cynara_initialize(&p_cynara, nullptr) != CYNARA_API_SUCCESS) {
        LOG(ERROR) << "Cynara initialization failed";
        return STOP_PRIVILEGE_SEARCH;
      }

      std::stringstream smacklabel;
      std::ifstream fileopen(kSmackLabelFilePath);
      if (fileopen.good()) {
        smacklabel << fileopen.rdbuf();
        fileopen.close();
      } else {
        LOG(ERROR) << "File open failed";
      }

      if (!smacklabel.str().empty()) {
        std::unique_ptr<char, decltype(std::free)*>
          session {cynara_session_from_pid(getpid()), std::free};

        if (cynara_check(p_cynara,
                         smacklabel.str().c_str(),
                         session.get(),
                         std::to_string(getuid()).c_str(),
                         reinterpret_cast<char*>(hostedapp_privilege))
                             == CYNARA_API_ACCESS_ALLOWED) {
          allowed = true;
        }
      }

      if (p_cynara) {
        cynara_finish(p_cynara);
        p_cynara = nullptr;
      }

      return STOP_PRIVILEGE_SEARCH;
    } else {
      return KEEP_PRIVILEGE_SEARCH;
    }
  };

  // find the hostedapp privilege
  ret = pkgmgrinfo_pkginfo_foreach_privilege(handle,
                                             find_privilege,
                                             const_cast<char*>(
                                                 hostedapp_privilege));
  if (ret != PMINFO_R_OK)
    LOG(ERROR) << "pkgmgrinfo_pkginfo_foreach_privilege failed";

  if (!allowed)
    LOG(ERROR) << "check 'hostedapp_deviceapi_allow' privilege!";

  pkgmgrinfo_pkginfo_destroy_pkginfo(handle);
  return allowed;
}

bool FindPrivilegeFromCynara(const std::string& privilege_name) {
  LOG(INFO) << "Finding privilege from cynara db";
  std::ifstream file(kSmackLabelFilePath);
  if (!file.is_open()) {
    LOG(ERROR) << "Failed to open " << kSmackLabelFilePath;
    return false;
  }

  int ret;
  cynara* cynara_h = NULL;
  ret = cynara_initialize(&cynara_h, 0);
  if (CYNARA_API_SUCCESS != ret) {
    LOG(ERROR) << "Failed. The result of cynara_initialize() : " << ret;
    return false;
  }

  std::string uid = std::to_string(getuid());
  std::string smack_label{std::istreambuf_iterator<char>(file),
                          std::istreambuf_iterator<char>()};

  bool result = false;
  ret = cynara_check(cynara_h, smack_label.c_str(), "", uid.c_str(), privilege_name.c_str());
  if (CYNARA_API_ACCESS_ALLOWED != ret) {
    LOG(ERROR) << "Access denied. The result of cynara_check() : " << ret;
  } else {
    LOG(INFO) << "Access allowed! The result of cynara_check() : " << ret;
    result = true;
  }

  ret = cynara_finish(cynara_h);
  if (CYNARA_API_SUCCESS != ret) {
    LOG(ERROR) << "Failed. The result of cynara_finish() : " << ret;
  }

  return result;
}

// Looking for added privilege by Application developer in config.xml.
bool FindPrivilegeFromConfig(const std::string& privilege_name) {
  auto& info = wrt::ApplicationData::GetInstance().permissions_info().GetAPIPermissions();
  for (auto it = info.begin(); it != info.end(); ++it) {
    if (*it == privilege_name)
      return true;
  }
  return false;
}

}  // namespace privilege

}  // namespace common
