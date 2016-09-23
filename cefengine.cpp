#include "cefengine.h"

extern QVariantMap settingsData;

CefEngine::CefEngine() : CefApp()
{

}

void CefEngine::AddRef() const {
    CefRefCount::AddRef();
}

bool CefEngine::Release() const {
    return CefRefCount::Release();
}

bool CefEngine::HasOneRef() const {
    return CefRefCount::HasOneRef();
}

void CefEngine::OnContextInitialized() {
    CefRegisterSchemeHandlerFactory("theweb", "theweb", new theWebSchemeHandler());
}

void CefEngine::OnContextCreated(Browser browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context) {
    if (QUrl(QString::fromStdString(frame.get()->GetURL().ToString())).scheme() == "theweb") {
        //Register the theWebSettingsObject JavaScript object
        CefRefPtr<theWebSettingsAccessor> accessor = new theWebSettingsAccessor(browser);
        CefRefPtr<CefV8Value> JsObject = CefV8Value::CreateObject(accessor);
        JsObject.get()->SetValue("dnt", V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE);
        JsObject.get()->SetValue("home", V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE);
        JsObject.get()->SetValue("toolbar", V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE);
        JsObject.get()->SetValue("tabText", V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE);
        JsObject.get()->SetValue("resetBrowser", CefV8Value::CreateFunction("resetBrowser", new V8Function([=]() {
            CefRefPtr<CefV8Value> returnVal;
            CefRefPtr<CefV8Exception> exception;
            context.get()->Eval("confirm(\"Reset theWeb?:Here\'s what we\'ll do.\\n"
                                "\- Reset all settings back to defaults\\n"
                                "\- Erase all history items\\n"
                                "\- Clear the cache\\n"
                                "\- Erase all cookies\\n"
                                "\\n"
                                "Are you sure that you want to reset theWeb?:Reset:Cancel\")", returnVal, exception);
            if (returnVal.get()->GetBoolValue()) {
                context.get()->Eval("confirm(\"Reset theWeb?:We're ready to reset theWeb. Once this is done, theWeb will exit. Is this OK?:Reset:Cancel\")", returnVal, exception);
            }
        })), V8_PROPERTY_ATTRIBUTE_NONE);

        context.get()->GetGlobal()->SetValue("theWebSettingsObject", JsObject, V8_PROPERTY_ATTRIBUTE_NONE);
    }

    //Get browser settings
    CefRefPtr<CefProcessMessage> message = CefProcessMessage::Create("theWebSettings_get");
    browser.get()->SendProcessMessage(PID_BROWSER, message);
}

void CefEngine::OnContextReleased(Browser browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context) {

}

bool CefEngine::OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefProcessId source_process, CefRefPtr<CefProcessMessage> message) {
    if (message.get()->GetName() == "theWebSettings_reply") {
        CefRefPtr<CefListValue> args = message.get()->GetArgumentList();
        CefRefPtr<CefDictionaryValue> settingsDictionary = args.get()->GetDictionary(0);
        std::vector<CefString> keys;
        settingsDictionary.get()->GetKeys(keys);
        for (CefString key : keys) {
            //Convert CEF types into Qt types
            if (settingsDictionary.get()->GetType(key) == VTYPE_BOOL) {
                settingsData.insert(QString::fromStdString(key.ToString()), settingsDictionary.get()->GetBool(key));
            } else if (settingsDictionary.get()->GetType(key) == VTYPE_STRING) {
                settingsData.insert(QString::fromStdString(key.ToString()), QString::fromStdString(settingsDictionary.get()->GetString(key).ToString()));
            }
        }
    }
    return true;
}
