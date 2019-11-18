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

#include "third_party/wrt/common/resource_manager.h"

#include <sys/types.h>
#include <sys/stat.h>
// #include <pkgmgr-info.h>
#include <stdio.h>
#include <unistd.h>
#include <web_app_enc.h>

#include <algorithm>
#include <functional>
#include <memory>
#include <set>
#include <sstream>
#include <vector>

#include "wrt/common/file_utils.h"
#include "wrt/common/locale_manager.h"
#include "wrt/common/string_utils.h"
#include "wrt/common/url.h"
#include "wrt/src/common/application_data.h"

#if defined(OS_TIZEN_TV_PRODUCT)
#include <app_common.h>
#include <tzplatform_config.h>
#include <tzplatform_variables.h>
#endif

namespace common {

namespace {

// Scheme type
const char* kSchemeTypeApp = "app://";
const char* kSchemeTypeFile = "file://";
const char* kSchemeTypeHttp = "http://";
const char* kSchemeTypeHttps = "https://";
// TODO(wy80.choi): comment out below unused const variables if needed.
// const char* kSchemeTypeWidget = "widget://";

#if defined(OS_TIZEN_TV_PRODUCT)
std::string GetWebApisPath() {
  return std::string{tzplatform_getenv(TZ_SYS_RW_APP)} + "/pepper";  // NOLINT
}

std::string GetB2BApisPath() {
  return std::string{tzplatform_getenv(TZ_SYS_RW_APP)} + "/pepper/b2b";  // NOLINT
}

std::string GetUsbDirPath() {
  return std::string{tzplatform_getenv(TZ_SYS_STORAGE)};  // NOLINT
}

std::string GetTempPath() {
  return std::string{tzplatform_getenv(TZ_SYS_VAR)} + "/tmp";  // NOLINT
}

std::string GetDataPath() {
  return std::string{app_get_data_path()};  // NOLINT
}

// Aliases
const std::vector<std::pair<std::string,
                            std::function<std::string()>>> kAliases {
  { "/$WEBAPIS"  , GetWebApisPath },
  { "/$B2BAPIS"  , GetB2BApisPath },
  { "/$USB_DIR"  , GetUsbDirPath  },
  { "/$TEMP"     , GetTempPath    },
  { "/$STD_WRITE", GetDataPath    },
  { "/$SHARE"    , GetDataPath    }
};
#endif

// EncryptedFileExtensions
const std::set<std::string> kEncryptedFileExtensions{".html", ".htm", ".css",
                                                     ".js"};

}  // namespace

ResourceManager::ResourceManager(LocaleManager* locale_manager)
    : locale_manager_(locale_manager),
      security_model_version_(0) {
  auto& app_data = wrt::ApplicationData::GetInstance();
  appid_ = app_data.tizen_application_info().id();
  if (app_data.csp_info().HasImpl() || app_data.csp_report_info().HasImpl() ||
      app_data.allowed_navigation_info().HasImpl()) {
    security_model_version_ = 2;
  } else {
    security_model_version_ = 1;
  }
  auto& widget_info = app_data.widget_info();
  if (!widget_info.default_locale().empty()) {
    locale_manager_->SetDefaultLocale(widget_info.default_locale());
  }
}

std::string ResourceManager::GetLocalizedPath(const std::string& origin) {
  std::string file_scheme = std::string() + kSchemeTypeFile + "/";
  std::string app_scheme = std::string() + kSchemeTypeApp;
  std::string locale_path = "locales/";
  auto find = locale_cache_.find(origin);
  if (find != locale_cache_.end()) {
    return find->second;
  }
  std::string& result = locale_cache_[origin];
  result = origin;
  std::string url = origin;

  std::string suffix;
  size_t pos = url.find_first_of("#?");
  if (pos != std::string::npos) {
    suffix = url.substr(pos);
    url.resize(pos);
  }

  if (utils::StartsWith(url, app_scheme)) {
    // remove "app://"
    url.erase(0, app_scheme.length());

    // remove app id + /
    std::string check = appid_ + "/";
    if (utils::StartsWith(url, check)) {
      url.erase(0, check.length());
    } else {
      LOG(ERROR) << "Invalid uri: {scheme:app} uri=" << origin;
      return result;
    }
  } else if (utils::StartsWith(url, file_scheme)) {
    // remove "file:///"
    url.erase(0, file_scheme.length());
  }

  if (!url.empty() && url[url.length() - 1] == '/') {
    url.erase(url.length() - 1, 1);
  }

  if (url.empty()) {
    LOG(ERROR) << "Invalid uri: uri=" << origin;
    return result;
  }

  std::string file_path = utils::UrlDecode(RemoveLocalePath(url));
  for (auto& locales : locale_manager_->system_locales()) {
    // check ../locales/
    std::string app_locale_path = resource_base_path_ + locale_path;
    if (!Exists(app_locale_path)) {
      break;
    }

    // check locale path ../locales/en_us/
    std::string app_localized_path = app_locale_path + locales + "/";
    if (!Exists(app_localized_path)) {
      continue;
    }
    std::string resource_path = app_localized_path + file_path;
    if (Exists(resource_path)) {
      result = "file://" + resource_path + suffix;
      return result;
    }
  }

  std::string default_locale = resource_base_path_ + file_path;
  if (Exists(default_locale)) {
    result = "file://" + default_locale + suffix;
    return result;
  }

#if defined(OS_TIZEN_TV_PRODUCT)
  // For TMG apps, |origin| resources will not be available and
  // should not be recognized as an error
  LOG(INFO) << "uri=" << origin << ", decoded=" << file_path;
#else
  LOG(ERROR) << "Invalid uri: uri=" << origin << ", decoded=" << file_path;
#endif
  return result;
}

std::string ResourceManager::RemoveLocalePath(const std::string& path) {
  std::string locale_path = "locales/";
  std::string result_path = path.at(0) == '/' ? path : "/" + path;
  if (!utils::StartsWith(result_path, resource_base_path_)) {
    return path;
  }

  result_path = result_path.substr(resource_base_path_.length());
  if (!utils::StartsWith(result_path, locale_path)) {
    return result_path;
  }

  size_t found = result_path.find_first_of('/', locale_path.length());
  if (found != std::string::npos) {
    result_path = result_path.substr(found + 1);
  }
  return result_path;
}

void ResourceManager::set_base_resource_path(const std::string& path) {
  if (path.empty()) {
    return;
  }

  resource_base_path_ = path;
  if (resource_base_path_[resource_base_path_.length() - 1] != '/') {
    resource_base_path_ += "/";
  }
}

bool ResourceManager::Exists(const std::string& path) {
  auto find = file_existed_cache_.find(path);
  if (find != file_existed_cache_.end()) {
    return find->second;
  }
  bool ret = file_existed_cache_[path] = utils::Exists(path);
  return ret;
}

bool ResourceManager::AllowNavigation(const std::string& url) {
  if (security_model_version_ == 2)
    return CheckAllowNavigation(url);
  return CheckWARP(url);
}

bool ResourceManager::AllowedResource(const std::string& url) {
  if (security_model_version_ == 2)
    return true;
  return CheckWARP(url);
}

bool ResourceManager::CheckWARP(const std::string& url) {
  // allow non-external resource
  if (!utils::StartsWith(url, kSchemeTypeHttp) &&
      !utils::StartsWith(url, kSchemeTypeHttps)) {
    return true;
  }

  auto& app_data = wrt::ApplicationData::GetInstance();
  auto& warp = app_data.warp_info();
  if (!warp.HasImpl())
    return false;

  auto find = warp_cache_.find(url);
  if (find != warp_cache_.end()) {
    return find->second;
  }

  bool& result = warp_cache_[url];
  result = true;

  URL url_info(url);

  // if didn't have a scheme, it means local resource
  if (url_info.scheme().empty()) {
    return true;
  }

  for (auto& allow : warp.access_map()) {
    if (allow.first == "*") {
      return true;
    } else if (allow.first.empty()) {
      continue;
    }

    URL allow_url(allow.first);

    // should be match the scheme and port
    if (allow_url.scheme() != url_info.scheme() ||
        allow_url.port() != url_info.port()) {
      continue;
    }

    // if domain alos was matched, allow resource
    if (allow_url.domain() == url_info.domain()) {
      return true;
    } else if (allow.second) {
      // if does not match domain, should be check sub domain

      // filter : test.com , subdomain=true
      // url : aaa.test.com
      // check url was ends with ".test.com"
      if (utils::EndsWith(url_info.domain(), "." + allow_url.domain())) {
        return true;
      }
    }
  }

  return result = false;
}

bool ResourceManager::CheckAllowNavigation(const std::string& url) {
  // allow non-external resource
  if (!utils::StartsWith(url, kSchemeTypeHttp) &&
      !utils::StartsWith(url, kSchemeTypeHttps)) {
    return true;
  }

  auto& app_data = wrt::ApplicationData::GetInstance();
  auto& allow_domains = app_data.allowed_navigation_info().GetAllowedDomains();
  if (allow_domains.empty())
    return false;

  auto find = warp_cache_.find(url);
  if (find != warp_cache_.end()) {
    return find->second;
  }

  bool& result = warp_cache_[url];
  result = true;

  URL url_info(url);

  // if didn't have a scheme, it means local resource
  if (url_info.scheme().empty()) {
    return true;
  }

  for (auto& allow_domain : allow_domains) {
    URL a_domain_info(allow_domain);

    // check wildcard *
    if (a_domain_info.domain() == "*") {
      return true;
    }

    bool prefix_wild = false;
    bool suffix_wild = false;
    std::string a_domain = a_domain_info.domain();
    if (utils::StartsWith(a_domain, "*")) {
      prefix_wild = true;
      // *.domain.com -> .domain.com
      a_domain = a_domain.substr(1);
    }
    if (utils::EndsWith(a_domain, "*")) {
      suffix_wild = true;
      // domain.* -> domain.
      a_domain = a_domain.substr(0, a_domain.length() - 1);
    }

    if (!prefix_wild && !suffix_wild) {
      // if no wildcard, should be exactly matched
      if (url_info.domain() == a_domain) {
        return true;
      }
    } else if (prefix_wild && !suffix_wild) {
      // *.domain.com : it shoud be "domain.com" or end with ".domain.com"
      if (url_info.domain() == a_domain.substr(1) ||
          utils::EndsWith(url_info.domain(), a_domain)) {
        return true;
      }
    } else if (!prefix_wild && suffix_wild) {
      // www.sample.* : it should be starts with "www.sample."
      if (utils::StartsWith(url_info.domain(), a_domain)) {
        return true;
      }
    } else if (prefix_wild && suffix_wild) {
      // *.sample.* : it should be starts with sample. or can find ".sample."
      // in url
      if (utils::StartsWith(url_info.domain(), a_domain.substr(1)) ||
          std::string::npos != url_info.domain().find(a_domain)) {
        return true;
      }
    }
  }

  return result = false;
}

bool ResourceManager::IsEncrypted(const std::string& path) {
  auto& app_data = wrt::ApplicationData::GetInstance();
  if (app_data.setting_info().encryption_enabled()) {
    std::string ext = utils::ExtName(path);
    if (kEncryptedFileExtensions.count(ext) > 0) {
      return true;
    }
  }
  return false;
}

#if defined(OS_TIZEN_TV_PRODUCT)
bool ResourceManager::ResolveAlias(std::string* url) const {
  bool is_alias_present = false;

  if (nullptr == url)
    return is_alias_present;

  std::string original(*url);
  const int start_position = utils::StartsWith(original, kSchemeTypeFile)
                                 ? strlen(kSchemeTypeFile)
                                 : 0;

  for (const auto& elem : kAliases) {
    auto pos = url->find(elem.first);
    while (std::string::npos != pos) {
      is_alias_present = true;
      url->replace(start_position, pos + elem.first.length() - start_position,
                   elem.second());
      pos = url->find(elem.first);
    }
  }

  if (utils::StartsWith(*url, "/")) {
    url->insert(0, kSchemeTypeFile);
    LOG(INFO) << "add [" << kSchemeTypeFile << "], url [" << *url << "]";
  }

  if (original != *url)
    LOG(INFO) << "Url [" << original << "] resolved to [" << *url << "]";
  return is_alias_present;
}
#endif

std::string ResourceManager::DecryptResource(const std::string& path) {
  // read file and make a buffer
  std::string src_path(path);
  if (utils::StartsWith(src_path, kSchemeTypeFile)) {
    src_path.erase(0, strlen(kSchemeTypeFile));
  }

  // Remove the parameters at the end of an href attribute
  size_t end_of_path = src_path.find_first_of("?#");
  if (end_of_path != std::string::npos)
    src_path = src_path.substr(0, end_of_path);

  // checking web app type
  static bool inited = false;
  static bool is_global = false;
  static bool is_preload = false;

  auto& app_data = wrt::ApplicationData::GetInstance();
  std::string pkg_id = app_data.GetPackageID();
  if (!inited) {
    inited = true;
    pkgmgrinfo_pkginfo_h handle;
    int ret =
        pkgmgrinfo_pkginfo_get_usr_pkginfo(pkg_id.c_str(), getuid(), &handle);
    if (ret != PMINFO_R_OK) {
      LOG(ERROR) << "Could not get handle for pkginfo : pkg_id = " << pkg_id;
      return path;
    } else {
      ret = pkgmgrinfo_pkginfo_is_global(handle, &is_global);
      if (ret != PMINFO_R_OK) {
        LOG(ERROR) << "Could not check is_global : pkg_id = " << pkg_id;
        pkgmgrinfo_pkginfo_destroy_pkginfo(handle);
        return path;
      }
      ret = pkgmgrinfo_pkginfo_is_preload(handle, &is_preload);
      if (ret != PMINFO_R_OK) {
        LOG(ERROR) << "Could not check is_preload : pkg_id = " << pkg_id;
        pkgmgrinfo_pkginfo_destroy_pkginfo(handle);
        return path;
      }
      pkgmgrinfo_pkginfo_destroy_pkginfo(handle);
    }
  }

  struct stat buf;
  memset(&buf, 0, sizeof(buf));
  if (stat(src_path.c_str(), &buf) == 0) {
    const std::size_t file_size = buf.st_size;
    std::unique_ptr<unsigned char[]> in_chunk;

    if (0 == file_size) {
      LOG(ERROR) << src_path.c_str()
                    << " size is 0, so decryption is skiped";
      return path;
    }

    FILE* src = fopen(src_path.c_str(), "rb");
    if (src == NULL) {
      LOG(ERROR) << "Cannot open file for decryption: " << path;
      return path;
    }

    // Read buffer from the source file
    std::unique_ptr<unsigned char[]> decrypted_str(
        new unsigned char[file_size]);
    int decrypted_size = 0;

    do {
      unsigned char get_dec_size[5];
      memset(get_dec_size, 0x00, sizeof(get_dec_size));

      size_t read_size = fread(get_dec_size, 1, 4, src);
      if (0 != read_size) {
        unsigned int read_buf_size = 0;
        std::istringstream(std::string((char*)get_dec_size)) >> read_buf_size;
        if (read_buf_size == 0) {
          LOG(ERROR) << "Failed to read resource";
          fclose(src);
          return path;
        }
        in_chunk.reset(new unsigned char[read_buf_size]);

        size_t dec_read_size = fread(in_chunk.get(), 1, read_buf_size, src);
        if (0 != dec_read_size) {
          unsigned char* decrypted_data = nullptr;
          size_t decrypted_len = 0;
          int ret;
          if (is_global) {
            ret = wae_decrypt_global_web_application(
                pkg_id.c_str(), is_preload, in_chunk.get(), (int)dec_read_size,
                &decrypted_data, &decrypted_len);
          } else {
            ret = wae_decrypt_web_application(
                getuid(), pkg_id.c_str(), in_chunk.get(), (int)dec_read_size,
                &decrypted_data, &decrypted_len);
          }

          if (WAE_ERROR_NONE != ret) {
            LOG(ERROR) << "Error during decryption: ";
            switch (ret) {
              case WAE_ERROR_INVALID_PARAMETER:
                LOG(ERROR) << "WAE_ERROR_INVALID_PARAMETER";
                break;
              case WAE_ERROR_PERMISSION_DENIED:
                LOG(ERROR) << "WAE_ERROR_PERMISSION_DENIED";
                break;
              case WAE_ERROR_NO_KEY:
                LOG(ERROR) << "WAE_ERROR_NO_KEY";
                break;
              case WAE_ERROR_KEY_MANAGER:
                LOG(ERROR) << "WAE_ERROR_KEY_MANAGER";
                break;
              case WAE_ERROR_CRYPTO:
                LOG(ERROR) << "WAE_ERROR_CRYPTO";
                break;
              case WAE_ERROR_UNKNOWN:
                LOG(ERROR) << "WAE_ERROR_UNKNOWN";
                break;
              default:
                LOG(ERROR) << "UNKNOWN";
                break;
            }
            fclose(src);
            return path;
          }

          memcpy(decrypted_str.get() + decrypted_size, decrypted_data,
                 decrypted_len);
          decrypted_size += decrypted_len;
          std::free(decrypted_data);
        }
      }
    } while (0 == std::feof(src));
    fclose(src);
    memset(decrypted_str.get() + decrypted_size, '\n',
           file_size - decrypted_size);

    // change to data schem
    std::stringstream dst_str;
    std::string content_type = utils::GetMimeFromFile(path);
    std::string encoded =
        utils::Base64Encode(decrypted_str.get(), decrypted_size);
    dst_str << "data:" << content_type << ";base64," << encoded;

    decrypted_str.reset(new unsigned char[file_size]);

    return dst_str.str();
  }
  return path;
}

}  // namespace common
