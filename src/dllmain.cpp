#include <windows.h>

#include "ColumnReplaceDialog.h"
#include "Localization.h"
#include "PluginInterface.h"
#include "PluginState.h"

NppData g_nppData;
HINSTANCE g_hInstance = nullptr;

namespace {

FuncItem g_funcItems[1];
bool g_funcItemsInitialized = false;

void commandShowDialog() { showColumnReplaceDialog(); }

void initializeFuncItems() {
    if (g_funcItemsInitialized) {
        return;
    }
    columnreplace::setLanguage(columnreplace::detectLanguage());
    lstrcpynW(g_funcItems[0]._itemName,
              columnreplace::tr(columnreplace::Text::MenuItem), menuItemSize);
    g_funcItems[0]._pFunc = commandShowDialog;
    g_funcItems[0]._cmdID = 0;
    g_funcItems[0]._init2Check = false;
    g_funcItems[0]._pShKey = nullptr;
    g_funcItemsInitialized = true;
}

} // namespace

HWND currentScintilla() {
    if (g_nppData._nppHandle == nullptr) {
        return nullptr;
    }
    const int which = static_cast<int>(
        SendMessageW(g_nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, 0));
    if (which == 0) {
        return g_nppData._scintillaMainHandle;
    }
    if (which == 1) {
        return g_nppData._scintillaSecondHandle;
    }
    return g_nppData._scintillaMainHandle;
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        g_hInstance = module;
        DisableThreadLibraryCalls(module);
    }
    return TRUE;
}

extern "C" __declspec(dllexport) void setInfo(NppData data) {
    g_nppData = data;
}

extern "C" __declspec(dllexport) const wchar_t* getName() {
    columnreplace::setLanguage(columnreplace::detectLanguage());
    return columnreplace::tr(columnreplace::Text::PluginName);
}

extern "C" __declspec(dllexport) FuncItem* getFuncsArray(int* count) {
    initializeFuncItems();
    if (count != nullptr) {
        *count = static_cast<int>(sizeof(g_funcItems) / sizeof(g_funcItems[0]));
    }
    return g_funcItems;
}

extern "C" __declspec(dllexport) void beNotified(SCNotification*) {
    // Nothing to do; the plugin has no notification-driven features.
}

extern "C" __declspec(dllexport) LRESULT messageProc(UINT, WPARAM, LPARAM) {
    return TRUE;
}

extern "C" __declspec(dllexport) BOOL isUnicode() {
    return TRUE;
}
