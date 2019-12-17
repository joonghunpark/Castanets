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

#ifndef XWALK_COMMON_PLATFORM_INFO_H_
#define XWALK_COMMON_PLATFORM_INFO_H_

namespace common {

enum _profile {
  kPROFILE_UNKNOWN = 0,
  kPROFILE_MOBILE = 0x1,
  kPROFILE_WEARABLE = 0x2,
  kPROFILE_TV = 0x4,
  kPROFILE_IVI = 0x8,
  kPROFILE_COMMON = 0x10,
};

// To optimize for GBMs, you may define the following values based on profile
// (e.g., #define TIZEN_FEATURE_blahblah (1))

#define TIZEN_FEATURE_web_ime_support \
  (common::getProfile() & ((common::kPROFILE_WEARABLE) | (common::kPROFILE_TV)))
#define TIZEN_FEATURE_manual_rotate_support \
  (common::getProfile() & ((common::kPROFILE_MOBILE)))
#define TIZEN_FEATURE_watch_face_support \
  (common::getProfile() & ((common::kPROFILE_WEARABLE)))
#define TIZEN_FEATURE_rotary_event_support \
  (common::getProfile() & ((common::kPROFILE_WEARABLE)))

extern enum _profile getProfile(void);

#if defined(OS_TIZEN_TV_PRODUCT)
extern bool hasSystemMultitaskingSupport(void);
extern int getProductDiskCacheSize(void);
#endif

}  // namespace common
#endif  // XWALK_COMMON_PLATFORM_INFO_H_
