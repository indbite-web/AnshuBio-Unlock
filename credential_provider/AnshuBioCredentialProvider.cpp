/**
 * AnshuBio Unlock - ICredentialProvider Implementation
 * Implements Microsoft Windows Credential Provider architecture
 * Coexists safely with Windows Password and PIN providers.
 */

#include "AnshuBioCredentialProvider.h"
#include "AnshuBioCredential.h"
#include <strsafe.h>
#include <new>

static const CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR s_rgFieldDescriptors[] = {
    { ABFI_LOGO, CPFT_TILE_IMAGE, const_cast<LPWSTR>(L"AnshuBio Logo"), GUID_NULL },
    { ABFI_LABEL, CPFT_LARGE_TEXT, const_cast<LPWSTR>(L"AnshuBio Unlock"), GUID_NULL },
    { ABFI_STATUS, CPFT_SMALL_TEXT, const_cast<LPWSTR>(L"Status"), GUID_NULL },
    { ABFI_SUBMIT_BUTTON, CPFT_SUBMIT_BUTTON, const_cast<LPWSTR>(L"Unlock"), GUID_NULL }
};

CAnshuBioCredentialProvider::CAnshuBioCredentialProvider() :
    m_cRef(1),
    m_cpus(CPUS_INVALID),
    m_pcpe(nullptr),
    m_upAdviseContext(0),
    m_pCredential(nullptr),
    m_bPipeConnected(FALSE)
{
    m_pCredential = new (std::nothrow) CAnshuBioCredential();
}

CAnshuBioCredentialProvider::~CAnshuBioCredentialProvider()
{
    if (m_pCredential)
    {
        m_pCredential->Release();
        m_pCredential = nullptr;
    }
    if (m_pcpe)
    {
        m_pcpe->Release();
        m_pcpe = nullptr;
    }
}

HRESULT CAnshuBioCredentialProvider::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv) return E_POINTER;
    if (riid == IID_IUnknown || riid == IID_ICredentialProvider)
    {
        *ppv = static_cast<ICredentialProvider*>(this);
        AddRef();
        return S_OK;
    }
    if (riid == IID_ICredentialProviderFilter)
    {
        *ppv = static_cast<ICredentialProviderFilter*>(this);
        AddRef();
        return S_OK;
    }
    *ppv = nullptr;
    return E_NOINTERFACE;
}

ULONG CAnshuBioCredentialProvider::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

ULONG CAnshuBioCredentialProvider::Release()
{
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (cRef == 0)
    {
        delete this;
    }
    return cRef;
}

HRESULT CAnshuBioCredentialProvider::SetUsageScenario(CREDENTIAL_PROVIDER_USAGE_SCENARIO cpus, DWORD dwFlags)
{
    m_cpus = cpus;
    switch (cpus)
    {
    case CPUS_LOGON:
    case CPUS_UNLOCK_WORKSTATION:
    case CPUS_CHANGE_PASSWORD:
        return S_OK;
    default:
        return E_NOTIMPL;
    }
}

HRESULT CAnshuBioCredentialProvider::SetSerialization(const CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION* pcpcs)
{
    return S_OK;
}

HRESULT CAnshuBioCredentialProvider::Advise(ICredentialProviderEvents* pcpe, UINT_PTR upAdviseContext)
{
    if (m_pcpe)
    {
        m_pcpe->Release();
        m_pcpe = nullptr;
    }
    m_pcpe = pcpe;
    if (m_pcpe)
    {
        m_pcpe->AddRef();
    }
    m_upAdviseContext = upAdviseContext;
    return S_OK;
}

HRESULT CAnshuBioCredentialProvider::UnAdvise()
{
    if (m_pcpe)
    {
        m_pcpe->Release();
        m_pcpe = nullptr;
    }
    m_upAdviseContext = 0;
    return S_OK;
}

HRESULT CAnshuBioCredentialProvider::GetFieldDescriptorCount(DWORD* pdwCount)
{
    if (!pdwCount) return E_POINTER;
    *pdwCount = ABFI_NUM_FIELDS;
    return S_OK;
}

HRESULT CAnshuBioCredentialProvider::GetFieldDescriptorAt(DWORD dwIndex, CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR** ppcpfd)
{
    if (!ppcpfd) return E_POINTER;
    if (dwIndex >= ABFI_NUM_FIELDS) return E_INVALIDARG;

    CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR* pcpfd = (CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR*)CoTaskMemAlloc(sizeof(CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR));
    if (!pcpfd) return E_OUTOFMEMORY;

    pcpfd->dwFieldID = s_rgFieldDescriptors[dwIndex].dwFieldID;
    pcpfd->cpft = s_rgFieldDescriptors[dwIndex].cpft;
    pcpfd->guidFieldType = s_rgFieldDescriptors[dwIndex].guidFieldType;

    size_t cbLabel = (wcslen(s_rgFieldDescriptors[dwIndex].pszLabel) + 1) * sizeof(WCHAR);
    pcpfd->pszLabel = (PWSTR)CoTaskMemAlloc(cbLabel);
    if (pcpfd->pszLabel)
    {
        StringCbCopyW(pcpfd->pszLabel, cbLabel, s_rgFieldDescriptors[dwIndex].pszLabel);
    }

    *ppcpfd = pcpfd;
    return S_OK;
}

HRESULT CAnshuBioCredentialProvider::GetCredentialCount(DWORD* pdwCount, DWORD* pdwDefault, BOOL* pbAutoLogonWithDefault)
{
    if (!pdwCount || !pdwDefault || !pbAutoLogonWithDefault) return E_POINTER;

    // Coexist safely: We provide 1 tile if credential exists
    *pdwCount = (m_pCredential != nullptr) ? 1 : 0;
    *pdwDefault = CREDENTIAL_PROVIDER_NO_DEFAULT;
    *pbAutoLogonWithDefault = FALSE;

    return S_OK;
}

HRESULT CAnshuBioCredentialProvider::GetCredentialAt(DWORD dwIndex, ICredentialProviderCredential** ppcpc)
{
    if (!ppcpc) return E_POINTER;
    if (dwIndex != 0 || !m_pCredential) return E_INVALIDARG;

    m_pCredential->AddRef();
    *ppcpc = m_pCredential;
    return S_OK;
}

HRESULT CAnshuBioCredentialProvider::Filter(CREDENTIAL_PROVIDER_USAGE_SCENARIO cpus, DWORD dwFlags, GUID* rgclsidProviders, BOOL* rgbAllow, DWORD cProviders)
{
    // CRITICAL: We NEVER filter out or disable Windows Password or PIN credential providers!
    // All existing providers are explicitly ALLOWED to ensure safe fallback.
    for (DWORD i = 0; i < cProviders; ++i)
    {
        rgbAllow[i] = TRUE;
    }
    return S_OK;
}

HRESULT CAnshuBioCredentialProvider::UpdateRemoteCredential(const CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION* pcpcsIn, CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION* pcpcsOut)
{
    return E_NOTIMPL;
}
