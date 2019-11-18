/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd All Rights Reserved
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

#include "wrt/common/platform_info.h"

#include <cstdlib>
#include <system_info.h>

#if defined(OS_TIZEN_TV_PRODUCT)
#include "base/logging.h"

#define DISKCACHE_BASE_SIZE (1 << 20)
#endif

namespace common {

enum _profile getProfile(void) {
  static enum _profile profile = kPROFILE_UNKNOWN;

  // This is false only for the first execution. Let's optimize it.
  if (__builtin_expect(profile != kPROFILE_UNKNOWN, 1))
    return profile;

  char* profileName;
  system_info_get_platform_string("http://tizen.org/feature/profile",
                                  &profileName);
  switch (*profileName) {
    case 'm':
    case 'M':
      profile = kPROFILE_MOBILE;
      break;
    case 'w':
    case 'W':
      profile = kPROFILE_WEARABLE;
      break;
    case 't':
    case 'T':
      profile = kPROFILE_TV;
      break;
    case 'i':
    case 'I':
      profile = kPROFILE_IVI;
      break;
    default:  // common or unknown ==> ALL ARE COMMON.
      profile = kPROFILE_COMMON;
  }
  free(profileName);

  return profile;
}

#if defined(OS_TIZEN_TV_PRODUCT)
bool hasSystemMultitaskingSupport(void) {
  bool systemSupport = true;
  if (SYSTEM_INFO_ERROR_NONE != system_info_get_value_bool(
      SYSTEM_INFO_KEY_MULTITASKING_SUPPORT, &systemSupport)) {
    LOG(INFO) << "Cannot get SYSTEM_INFO_KEY_MULTITASKING_SUPPORT";
  }
  return systemSupport;
}

int getProductDiskCacheSize(void) {
  int diskCacheValue = 0;
  if (SYSTEM_INFO_ERROR_NONE != system_info_get_value_int(
      SYSTEM_INFO_KEY_DISK_CACHE_APP_SIZE, &diskCacheValue)) {
    LOG(INFO) << "Cannot get SYSTEM_INFO_KEY_DISK_CACHE_APP_SIZE";
  }
  return diskCacheValue * DISKCACHE_BASE_SIZE;
}
#endif

}  // namespace common
