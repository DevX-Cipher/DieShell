#include <wrl/module.h>
#include <wrl/implements.h>
#include <shobjidl_core.h>
#include <Shellapi.h>
#include <Shlwapi.h>
#include <Strsafe.h>
#include <VersionHelpers.h>
#include <Shobjidl.h>
#include <string>

using namespace Microsoft::WRL;
HMODULE g_hModule = nullptr;

/**
 * @brief The pragma directives below are used to instruct the linker to export the specified functions
 * with the correct names and calling conventions. This ensures that the functions are available for
 * external use without needing a .def file.
 */

#pragma comment(linker, "/export:DllCanUnloadNow=DllCanUnloadNow")
#pragma comment(linker, "/export:DllGetClassObject=DllGetClassObject")
#pragma comment(linker, "/export:DllGetActivationFactory=DllGetActivationFactory")

 /**
  * @brief DllGetActivationFactory retrieves the activation factory for the specified class.
  *
  * This function is called to get the activation factory for the specified class identifier (CLSID).
  *
  * @param activatableClassId The class identifier.
  * @param factory Output parameter to receive the activation factory.
  * @return HRESULT indicating success (S_OK) or failure.
  */

extern "C" HRESULT DllGetActivationFactory(HSTRING activatableClassId, IActivationFactory** factory) {
  return Module<ModuleType::InProc>::GetModule().GetActivationFactory(activatableClassId, factory);
}

/**
 * @brief DllCanUnloadNow checks if the DLL can be unloaded from memory.
 *
 * This function is called to check whether the DLL can be safely unloaded from memory.
 *
 * @return S_OK if the DLL can be unloaded, S_FALSE otherwise.
 */

extern "C" HRESULT DllCanUnloadNow() {
  return Module<InProc>::GetModule().GetObjectCount() == 0 ? S_OK : S_FALSE;
}

/**
 * @brief DllGetClassObject retrieves the class factory for the specified class.
 *
 * This function is called to get the class factory for the specified class identifier (CLSID).
 *
 * @param rclsid The class identifier.
 * @param riid The interface identifier.
 * @param instance Output parameter to receive the class factory instance.
 * @return HRESULT indicating success (S_OK) or failure.
 */

extern "C" HRESULT DllGetClassObject(REFCLSID rclsid, REFIID riid, void** instance) {
  return Module<InProc>::GetModule().GetClassObject(rclsid, riid, instance);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
  if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
    g_hModule = hModule;
  }
  return TRUE;
}

/**
 * @brief DieCommand class implements the IExplorerCommand and IObjectWithSite interfaces.
 */
class DieCommand : public RuntimeClass<RuntimeClassFlags<ClassicCom>, IExplorerCommand, IObjectWithSite> {
public:

  /**
  * @brief GetTitle retrieves the title of the context menu item.
  *
  * @param items The selected items in the shell.
  * @param name Output parameter to receive the title.
  * @return HRESULT indicating success or failure.
  */

  IFACEMETHODIMP GetTitle(_In_opt_ IShellItemArray* items, _Outptr_result_nullonfailure_ PWSTR* name) {
    *name = nullptr;
    size_t titleLen = wcslen(L"Detect It Easy") + 1;
    *name = (PWSTR)CoTaskMemAlloc(titleLen * sizeof(WCHAR));
    if (!*name) {
      return E_OUTOFMEMORY;
    }
    wcscpy_s(*name, titleLen, L"Detect It Easy");
    return S_OK;
  }

  /**
  * @brief GetIcon retrieves the icon path for the context menu item.
  *
  * This method obtains the icon path for the context menu item based on the selected items in the shell.
  *
  * @param items The selected items in the shell.
  * @param iconPath Output parameter to receive the icon path.
  * @return HRESULT indicating success (S_OK) or failure (E_FAIL).
  */

  IFACEMETHODIMP GetIcon(_In_opt_ IShellItemArray* items, _Outptr_result_nullonfailure_ PWSTR* iconPath) {
    *iconPath = nullptr;
    if (!items) return E_FAIL;

    DWORD count = 0;
    ComPtr<IShellItem> item;
    PWSTR itemPath = nullptr;

    if (FAILED(items->GetCount(&count)) || count == 0 ||
      FAILED(items->GetItemAt(0, &item)) ||
      FAILED(item->GetDisplayName(SIGDN_FILESYSPATH, &itemPath)))
      return E_FAIL;

    WCHAR modulePath[MAX_PATH];
    if (!GetModuleFileNameW(g_hModule, modulePath, ARRAYSIZE(modulePath))) {
      CoTaskMemFree(itemPath);
      return E_FAIL;
    }

    PathRemoveFileSpecW(modulePath);
    StringCchCatW(modulePath, ARRAYSIZE(modulePath), L"\\Die.exe");

    size_t len = wcslen(modulePath) + 1;
    *iconPath = (PWSTR)CoTaskMemAlloc(len * sizeof(WCHAR));
    if (*iconPath) {
      wcscpy_s(*iconPath, len, modulePath);
    }

    CoTaskMemFree(itemPath);
    return *iconPath ? S_OK : E_FAIL;
  }

  /**
  * @brief Retrieves the real Windows OS version using the undocumented RtlGetVersion function.
  *
  * This avoids the issues with GetVersionEx being affected by application manifests.
  * RtlGetVersion gives accurate OS version info, even on newer Windows versions.
  *
  * @param lpVersionInformation Pointer to an RTL_OSVERSIONINFOW structure that receives the version info.
  * @return Returns 0 (STATUS_SUCCESS) on success, or -1 on failure.
  */

  LONG GetRealOSVersion(PRTL_OSVERSIONINFOW lpVersionInformation) {
    HMODULE hMod = ::GetModuleHandleW(L"ntdll.dll");
    if (hMod) {
      RtlGetVersionPtr fn = (RtlGetVersionPtr)::GetProcAddress(hMod, "RtlGetVersion");
      if (fn != nullptr) {
        return fn(lpVersionInformation);
      }
    }
    return -1;
  }

  /**
  * @brief Checks if the foreground window is a common dialog (#32770).
  *
  * @param outSuccess Pointer to bool to receive if detection succeeded (true) or failed (false).
  * @return true if the foreground window is a dialog, false if not.
  *         If detection fails, returns false and sets *outSuccess to false.
  */

  bool DialogWindow(bool* success) {
    if (!success) return false;

    HWND hwnd = GetForegroundWindow();
    wchar_t cls[256];
    *success = hwnd && IsWindowVisible(hwnd) && GetClassName(hwnd, cls, ARRAYSIZE(cls));
    return *success && wcscmp(cls, L"#32770") == 0;
  }

  /**
  * @brief GetState retrieves the state of the context menu item.
  *
  * This method determines the state of the context menu item based on the provided selection and system conditions.
  *
  * @param selection The selected items in the shell.
  * @param okToBeSlow Indicates whether it's acceptable to be slow.
  * @param cmdState Output parameter to receive the state of the context menu item (EXPCMDSTATE).
  * @return HRESULT indicating success (S_OK) or failure.
  */

  IFACEMETHODIMP GetState(_In_opt_ IShellItemArray* selection, _In_ BOOL okToBeSlow, _Out_ EXPCMDSTATE* state) {
    if (!state) return E_POINTER;

    RTL_OSVERSIONINFOW osvi = { sizeof(osvi) };
    if (GetRealOSVersion(&osvi) != 0) {
      *state = ECS_ENABLED;
      return S_OK;
    }

    if (osvi.dwMajorVersion < 10 || osvi.dwBuildNumber < 22000) {
      *state = ECS_ENABLED;
    }
    else if (selection && okToBeSlow) {
      *state = ECS_ENABLED;
    }
    else {
      bool success = false;
      *state = (DialogWindow(&success) || !success) ? ECS_ENABLED : ECS_HIDDEN;
    }

    return S_OK;
  }

  /**
  * @brief Invoke executes the context menu item action.
  *
  * This method is called to perform the action associated with the context menu item. It displays debug messages
  * for invalid arguments or if there are no items to process. It retrieves the file path of the selected item,
  * extracts information about the file.
  *
  * @param selection The selected items in the shell.
  * @param bindContext The bind context.
  * @return HRESULT indicating success (S_OK) or failure.
  */

  IFACEMETHODIMP Invoke(_In_opt_ IShellItemArray* selection, _In_opt_ IBindCtx*) {
    if (!selection) {
      MessageBox(nullptr, L"Invalid argument", L"Debug Info", MB_OK);
      return E_INVALIDARG;
    }

    DWORD count = 0;
    ComPtr<IShellItem> item;
    PWSTR filePath = nullptr;

    if (FAILED(selection->GetCount(&count)) || count == 0 ||
      FAILED(selection->GetItemAt(0, &item)) ||
      FAILED(item->GetDisplayName(SIGDN_FILESYSPATH, &filePath))) {
      MessageBox(nullptr, L"No items to process", L"Debug Info", MB_OK);
      return S_OK;
    }

    WCHAR dllDir[MAX_PATH];
    if (!GetModuleFileName(g_hModule, dllDir, MAX_PATH)) {
      CoTaskMemFree(filePath);
      return E_FAIL;
    }
    PathRemoveFileSpec(dllDir);

    std::wstring dieExe = std::wstring(dllDir) + L"\\Die.exe";
    if (GetFileAttributes(dieExe.c_str()) == INVALID_FILE_ATTRIBUTES) {
      CoTaskMemFree(filePath);
      MessageBox(nullptr, L"Die.exe not found", L"Error", MB_OK | MB_ICONERROR);
      return E_FAIL;
    }

    std::wstring args = L"\"" + std::wstring(filePath) + L"\"";
    CoTaskMemFree(filePath);

    if (!ShellExecute(nullptr, L"open", dieExe.c_str(), args.c_str(), nullptr, SW_SHOWNORMAL)) {
      MessageBox(nullptr, L"Failed to execute Die.exe", L"Error", MB_OK | MB_ICONERROR);
      return E_FAIL;
    }

    return S_OK;
  }

  IFACEMETHODIMP GetToolTip(_In_opt_ IShellItemArray*, _Outptr_result_nullonfailure_ PWSTR* infoTip) { *infoTip = nullptr; return E_NOTIMPL; }
  IFACEMETHODIMP GetCanonicalName(_Out_ GUID* guidCommandName) { *guidCommandName = GUID_NULL; return S_OK; }

  IFACEMETHODIMP GetFlags(_Out_ EXPCMDFLAGS* flags) { *flags = ECF_DEFAULT; return S_OK; }
  IFACEMETHODIMP EnumSubCommands(_COM_Outptr_ IEnumExplorerCommand** enumCommands) { *enumCommands = nullptr; return E_NOTIMPL; }

  IFACEMETHODIMP SetSite(_In_ IUnknown* site) noexcept { m_site = site; return S_OK; }
  IFACEMETHODIMP GetSite(_In_ REFIID riid, _COM_Outptr_ void** site) noexcept { return m_site.CopyTo(riid, site); }

protected:
  ComPtr<IUnknown> m_site;
};

class __declspec(uuid("7A1E471F-0D43-4122-B1C4-D1AACE76CE9B")) DieCommand1 final : public DieCommand {};

CoCreatableClass(DieCommand1)