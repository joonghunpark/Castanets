#ifndef INJECTED_BUNDLE_H_
#define INJECTED_BUNDLE_H_

#include <string>
#include <v8.h>

#include "public/ewk_ipc_message_internal.h"

#define INJECTED_BUNDLE_EXPORT extern "C" __attribute__((visibility("default")))

INJECTED_BUNDLE_EXPORT unsigned int DynamicPluginVersion(void);
INJECTED_BUNDLE_EXPORT void DynamicSetWidgetInfo(const char* tizen_id);
INJECTED_BUNDLE_EXPORT void DynamicPluginStartSession(
    const char* tizen_id, v8::Handle<v8::Context> context,
    int routing_handle, const char* base_url);
INJECTED_BUNDLE_EXPORT void DynamicPluginStopSession(
    const char* tizen_id, v8::Handle<v8::Context> context);
INJECTED_BUNDLE_EXPORT void DynamicUrlParsing(
    std::string* old_url, std::string* new_url, const char* tizen_id);
INJECTED_BUNDLE_EXPORT void DynamicDatabaseAttach(int /*attach*/);
INJECTED_BUNDLE_EXPORT void DynamicOnIPCMessage(const Ewk_Wrt_Message_Data& data);

#endif  // INJECTED_BUNDLE_H_
