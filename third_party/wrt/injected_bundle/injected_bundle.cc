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

#include "injected_bundle.h"

// #include <Ecore.h>

#include "base/synchronization/lock.h"
// #include "wrt/common/locale_manager.h"
// #include "wrt/common/profiler.h"
// #include "wrt/common/resource_manager.h"
#include "third_party/wrt/common/string_utils.h"
// #include "wrt/src/common/application_data.h"
#include "third_party/wrt/xwalk_extensions/renderer/runtime_ipc_client.h"
#include "third_party/wrt/xwalk_extensions/renderer/widget_module.h"
#include "third_party/wrt/xwalk_extensions/renderer/xwalk_extension_renderer_controller.h"
#include "third_party/wrt/xwalk_extensions/renderer/xwalk_module_system.h"

#if defined(OS_TIZEN_TV_PRODUCT)
#include "wrt/common/privilege.h"
#include "wrt/src/renderer/encrypted_file_handler.h"
#endif

namespace runtime {
/*
class BundleGlobalData {
 public:
  static BundleGlobalData* GetInstance() {
    static BundleGlobalData instance;
    return &instance;
  }

  void Initialize(const std::string& app_id) {
    base::AutoLock lock(lock_);
    if (initialized_)
      return;
    initialized_ = true;

    auto& app_data = wrt::ApplicationData::GetInstance();
    app_data.Initialize();

    locale_manager_.reset(new common::LocaleManager);
    locale_manager_->EnableAutoUpdate(true);

    if (!app_data.widget_info().default_locale().empty()) {
      locale_manager_->SetDefaultLocale(
          app_data.widget_info().default_locale());
    }
    resource_manager_.reset(
        new common::ResourceManager(locale_manager_.get()));
    resource_manager_->set_base_resource_path(
        app_data.application_path());

    auto widgetdb = extensions::WidgetPreferenceDB::GetInstance();
    widgetdb->Initialize(locale_manager_.get());
  }

  common::ResourceManager* resource_manager() {
    return resource_manager_.get();
  }

#if defined(OS_TIZEN_TV_PRODUCT)
  void constructEncryptedFileHandler() {
    if (encryptedFileHandler_)
      return;
    encryptedFileHandler_.reset(new FileEncryption::EncryptedFileHandler());
  }
  FileEncryption::EncryptedFileHandler& getEncryptedFileHandler() {
    if (!encryptedFileHandler_) {
      LOG(ERROR) << "EncryptedFileHandler not constructed";
    }
    return *encryptedFileHandler_;
  }
#endif

 private:
  BundleGlobalData() : initialized_(false) {}
  ~BundleGlobalData() {}
  std::unique_ptr<common::ResourceManager> resource_manager_;
  std::unique_ptr<common::LocaleManager> locale_manager_;

  bool initialized_;
  base::Lock lock_;

#if defined(OS_TIZEN_TV_PRODUCT)
  std::unique_ptr<FileEncryption::EncryptedFileHandler> encryptedFileHandler_;
#endif
};
*/
}  //  namespace runtime

unsigned int DynamicPluginVersion(void) {
  return 1;
}

void DynamicSetWidgetInfo(const char* tizen_id) {
  // SCOPE_PROFILE();
  // ecore_init();

  // runtime::BundleGlobalData::GetInstance()->Initialize(tizen_id);
  extensions::XWalkExtensionRendererController& controller =
    extensions::XWalkExtensionRendererController::GetInstance();
  // auto& app_data = wrt::ApplicationData::GetInstance();
  // controller.LoadUserExtensions(app_data.application_path());
}

void DynamicPluginStartSession(const char* tizen_id,
                               v8::Handle<v8::Context> context,
                               int routing_handle,
                               const char* base_url) {
  // SCOPE_PROFILE();

  // Initialize context's aligned pointer in embedder data with null
  extensions::XWalkModuleSystem::SetModuleSystemInContext(
      std::unique_ptr<extensions::XWalkModuleSystem>(), context);

  if (!base_url || (common::utils::StartsWith(base_url, "http")
#if defined(OS_TIZEN_TV_PRODUCT)
      && !common::privilege::CheckHostedAppPrivilege(
          wrt::ApplicationData::GetInstance().GetPackageID())
#endif
    )) {
    LOG(ERROR) << "External url not allowed plugin loading.";
    return;
  }

  // Initialize RuntimeIPCClient
  extensions::RuntimeIPCClient* rc =
      extensions::RuntimeIPCClient::GetInstance();
  rc->SetRoutingId(context, routing_handle);

  extensions::XWalkExtensionRendererController& controller =
      extensions::XWalkExtensionRendererController::GetInstance();
  controller.DidCreateScriptContext(context);
}

void DynamicPluginStopSession(
    const char* tizen_id, v8::Handle<v8::Context> context) {
  // SCOPE_PROFILE();
  extensions::XWalkExtensionRendererController& controller =
      extensions::XWalkExtensionRendererController::GetInstance();
  controller.WillReleaseScriptContext(context);
}

void DynamicUrlParsing(
    std::string* old_url, std::string* new_url, const char* tizen_id) {
  LOG(INFO) << "DynamicUrlParsing";
  /*
  auto res_manager =
      runtime::BundleGlobalData::GetInstance()->resource_manager();
  if (res_manager == NULL) {
    LOG(ERROR) << "Widget Info was not set, Resource Manager is NULL";
    *new_url = *old_url;
    return;
  }
  // Check Access control
  if (!res_manager->AllowedResource(*old_url)) {
    // To maintain backward compatibility, we shoudn't explicitly set URL "about:blank"
    *new_url = std::string();
    LOG(ERROR) << "request was blocked by WARP";
    return;
  }
  // convert to localized path
  if (common::utils::StartsWith(*old_url, "file:/") ||
      common::utils::StartsWith(*old_url, "app:/")) {
    *new_url = res_manager->GetLocalizedPath(*old_url);
  } else {
    *new_url = *old_url;
  }
  // check encryption
  if (res_manager->IsEncrypted(*new_url)) {
    *new_url = res_manager->DecryptResource(*new_url);
  }
  */
}

void DynamicDatabaseAttach(int /*attach*/) {
  // LOG(INFO) << "InjectedBundle::DynamicDatabaseAttach !!";
}

void DynamicOnIPCMessage(const Ewk_Wrt_Message_Data& data) {
  // SCOPE_PROFILE();
  extensions::XWalkExtensionRendererController& controller =
    extensions::XWalkExtensionRendererController::GetInstance();
  controller.OnReceivedIPCMessage(&data);
}

