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

#ifndef DEVICE_INFO_H_
#define DEVICE_INFO_H_

#include <list>
#include <string>

namespace common {
#if defined(OS_TIZEN_TV_PRODUCT)
std::string getFirstUSBPath();
int getNumberOfUSB();
std::list<std::string> getAllUSBPath();
#endif

}  // namespace common
#endif  // DEVICE_INFO_H_