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

#include "wrt/common/device_info.h"

#if defined(OS_TIZEN_TV_PRODUCT)
#include <usb-device.h>
#include "base/logging.h"
#endif

namespace common {

#if defined(OS_TIZEN_TV_PRODUCT)
std::string getFirstUSBPath() {
  std::list<std::string> usb_path_list = getAllUSBPath();
  if (0 < usb_path_list.size())
    return usb_path_list.front();

  return std::string();
}

int getNumberOfUSB() {
  int number = 0;
  usb_device_list_h usb_device_list;

  if (usb_device_init() != USB_ERROR_NO_ERROR) {
    LOG(ERROR) << "failed to initialize usb device";
    return number;
  }

  if (USB_ERROR_NO_ERROR !=
      usb_device_get_device_list(USB_MASS_STORAGE, &usb_device_list)) {
    LOG(ERROR) << "failed to get mass storage list from usb-server";
    usb_device_fini();
    return number;
  }

  number = usb_device_list_get_count(usb_device_list);

  if (0 > number) {
    LOG(ERROR) << "number of the connected USB is invalid " << number;
    number = 0;
  } else {
    LOG(INFO) << "number of the connected USB is " << number;
  }

  usb_device_free_device_list(usb_device_list);
  usb_device_fini();
  return number;
}

std::list<std::string> getAllUSBPath() {
  std::list<std::string> usb_path_list;
  std::string usb_path = "";
  usb_device_list_h usb_device_list;
  usb_device_h usb_device;

  if (usb_device_init() != USB_ERROR_NO_ERROR) {
    LOG(ERROR) << "failed to initialize usb device";
    return usb_path_list;
  }

  if (USB_ERROR_NO_ERROR !=
      usb_device_get_device_list(USB_MASS_STORAGE, &usb_device_list)) {
    LOG(ERROR) << "failed to get mass storage list from usb-server";
    usb_device_fini();
    return usb_path_list;
  }

  if (USB_ERROR_NO_ERROR !=
      usb_device_list_get_first(usb_device_list, &usb_device)) {
    LOG(ERROR) << "failed to get usb_device_list_get_first";
    usb_device_free_device_list(usb_device_list);
    usb_device_fini();
    return usb_path_list;
  }

  do {
    usb_path = usb_device_get_mountpath(usb_device);
    if (!strcmp(usb_path.c_str(), "n/a")) {
      LOG(INFO) << "path is n/a, not set to usbPathList";
    } else {
      LOG(INFO) << "find USB " << usb_path;
      usb_path_list.push_back(usb_path);
    }
  } while (USB_ERROR_NO_ERROR ==
           usb_device_list_get_next(usb_device_list, &usb_device));

  usb_device_free_device_list(usb_device_list);
  usb_device_fini();
  return usb_path_list;
}
#endif

}  // namespace common
