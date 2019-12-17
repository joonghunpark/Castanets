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

#ifndef XWALK_COMMON_RESOURCE_MANAGER_H_
#define XWALK_COMMON_RESOURCE_MANAGER_H_

#include <map>
#include <memory>
#include <string>

namespace common {

class LocaleManager;

class ResourceManager {
 public:
  ResourceManager(LocaleManager* locale_manager);
  ~ResourceManager() {}

  // input : file:///..... , app://[appid]/....
  // output : /[system path]/.../locales/.../
  std::string GetLocalizedPath(const std::string& origin);
  bool AllowNavigation(const std::string& url);
  bool AllowedResource(const std::string& url);

  bool IsEncrypted(const std::string& url);
  std::string DecryptResource(const std::string& path);

#if defined(OS_TIZEN_TV_PRODUCT)
  bool ResolveAlias(std::string* url) const;
#endif

  void set_base_resource_path(const std::string& base_path);

 private:
  // for localization
  bool Exists(const std::string& path);
  bool CheckWARP(const std::string& url);
  bool CheckAllowNavigation(const std::string& url);
  std::string RemoveLocalePath(const std::string& path);

  std::string resource_base_path_;
  std::string appid_;
  std::map<const std::string, bool> file_existed_cache_;
  std::map<const std::string, std::string> locale_cache_;
  std::map<const std::string, bool> warp_cache_;

  LocaleManager* locale_manager_;
  int security_model_version_;
};

}  // namespace common

#endif  // XWALK_COMMON_RESOURCE_MANAGER_H_
