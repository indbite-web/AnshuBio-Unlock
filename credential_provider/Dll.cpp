/**
 * AnshuBio Unlock - Credential Provider DLL Entrypoints & Registration
 * Implements COM ClassFactory, DllRegisterServer, and DllUnregisterServer.
 */

#include <windows.h>
#include <unknwn.h>
#include <new>
#include "guid.h"
#include "AnshuBioCredentialProvider.h"

HINSTANCE g_hInst = nullptr;
LONG g_cRefDll = 0;

class CClassFactory : public IClassFactory
{
public:
    IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv)
    {
        if (!ppv) return E_POINTER;
        if (riid == IID_IUnknown || riid == IID_IClassFactory)
        {
            *ppv = static_cast<IClassFactory*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }

    IFACEMETHODIMP_(ULONG) AddRef()
    {
        return InterlockedIncrement(&m_cRef);
    }

    IFACEMETHODIMP_(ULONG) Release()
    {
        LONG cRef = InterlockedDecrement(&m_cRef);
        if (cRef == 0)
        {
            delete this;
        }
        return cRef;
    }

    IFACEMETHODIMP CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppv)
    {
        if (pUnkOuter != nullptr) return CLASS_E_NOAGGREGATION;

        CAnshuBioCredentialProvider* pProvider = new (std::nothrow) CAnshuBioCredentialProvider();
        if (!pProvider) return E_OUTOFMEMORY;

        HRESULT hr = pProvider->QueryInterface(riid, ppv);
        pProvider->Release();
        return hr;
    }

    IFACEMETHODIMP LockServer(BOOL fLock)
    {
        if (fLock)
        {
            InterlockedIncrement(&g_cRefDll);
        }
        else
        {
            InterlockedDecrement(&g_cRefDll);
        }
        return S_OK;
    }

    CClassFactory() : m_cRef(1) {}
    virtual ~CClassFactory() {}

private:
    LONG m_cRef;
};

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        g_hInst = hinstDLL;
        DisableThreadLibraryCalls(hinstDLL);
        break;
    }
    return TRUE;
}

STDAPI DllCanUnloadNow()
{
    return (g_cRefDll == 0) ? S_OK : S_FALSE;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppv)
{
    if (IsEqualCLSID(rclsid, CLSID_AnshuBioCredentialProvider))
    {
        CClassFactory* pFactory = new (std::nothrow) CClassFactory();
        if (!pFactory) return E_OUTOFMEMORY;

        HRESULT hr = pFactory->QueryInterface(riid, ppv);
        pFactory->Release();
        return hr;
    }
    return CLASS_E_CLASSNOTAVAILABLE;
}

STDAPI DllRegisterServer()
{
    WCHAR szModule[MAX_PATH];
    if (!GetModuleFileNameW(g_hInst, szModule, ARRAYSIZE(szModule)))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    // Register CLSID in InProcServer32
    HKEY hKey;
    LONG lRes = RegCreateKeyExW(
        HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Classes\\CLSID\\{B36E9B9A-5827-463F-8C37-67AB12E09B10}\\InprocServer32",
        0, nullptr, REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr
    );
    if (lRes == ERROR_SUCCESS)
    {
        RegSetValueExW(hKey, nullptr, 0, REG_SZ, (BYTE*)szModule, (DWORD)((wcslen(szModule) + 1) * sizeof(WCHAR)));
        const WCHAR szModel[] = L"Apartment";
        RegSetValueExW(hKey, L"ThreadingModel", 0, REG_SZ, (BYTE*)szModel, (DWORD)((wcslen(szModel) + 1) * sizeof(WCHAR)));
        RegCloseKey(hKey);
    }

    // Register under Windows Authentication Credential Providers
    lRes = RegCreateKeyExW(
        HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Authentication\\Credential Providers\\{B36E9B9A-5827-463F-8C37-67AB12E09B10}",
        0, nullptr, REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr
    );
    if (lRes == ERROR_SUCCESS)
    {
        const WCHAR szName[] = ANSHUBIO_CP_NAME;
        RegSetValueExW(hKey, nullptr, 0, REG_SZ, (BYTE*)szName, (DWORD)((wcslen(szName) + 1) * sizeof(WCHAR)));
        RegCloseKey(hKey);
    }

    return S_OK;
}

STDAPI DllUnregisterServer()
{
    // Cleanly unregister without removing or touching any default providers
    RegDeleteKeyW(
        HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Authentication\\Credential Providers\\{B36E9B9A-5827-463F-8C37-67AB12E09B10}"
    );

    RegDeleteKeyW(
        HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Classes\\CLSID\\{B36E9B9A-5827-463F-8C37-67AB12E09B10}\\InprocServer32"
    );

    RegDeleteKeyW(
        HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Classes\\CLSID\\{B36E9B9A-5827-463F-8C37-67AB12E09B10}"
    );

    return S_OK;
}
