/**
 * AnshuBio Unlock - ICredentialProviderCredential Implementation
 * Implements real Windows Credential Provider Authentication & LSA Serialization.
 * Publisher: AnshuCore
 */

#include "AnshuBioCredential.h"
#include "guid.h"
#include <strsafe.h>
#include <ntsecapi.h>
#include <cstdint>

#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
#endif

// Helper to initialize LSA UNICODE_STRING
static void InitLsaString(PLSA_STRING pLsaString, LPCSTR pszString)
{
    if (pszString)
    {
        pLsaString->Length = (USHORT)strlen(pszString);
        pLsaString->MaximumLength = pLsaString->Length + 1;
        pLsaString->Buffer = (PCHAR)pszString;
    }
    else
    {
        pLsaString->Length = 0;
        pLsaString->MaximumLength = 0;
        pLsaString->Buffer = nullptr;
    }
}

static void InitUnicodeString(PUNICODE_STRING pUnicodeString, PWSTR pszString, USHORT cbSize)
{
    pUnicodeString->Length = cbSize;
    pUnicodeString->MaximumLength = cbSize;
    pUnicodeString->Buffer = pszString;
}

CAnshuBioCredential::CAnshuBioCredential() :
    m_cRef(1),
    m_pcpce(nullptr),
    m_bIsAuthenticated(FALSE),
    m_hAuthThread(nullptr),
    m_bStopAuthThread(FALSE)
{
    StringCchCopyW(m_szStatusText, ARRAYSIZE(m_szStatusText), L"Waiting for phone biometric confirmation...");
    m_szUsername[0] = L'\0';
    m_szDomain[0] = L'\0';
    m_szAuthToken[0] = L'\0';
}

CAnshuBioCredential::~CAnshuBioCredential()
{
    if (m_hAuthThread)
    {
        m_bStopAuthThread = TRUE;
        WaitForSingleObject(m_hAuthThread, 1000);
        CloseHandle(m_hAuthThread);
        m_hAuthThread = nullptr;
    }
    if (m_pcpce)
    {
        m_pcpce->Release();
        m_pcpce = nullptr;
    }
}

HRESULT CAnshuBioCredential::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv) return E_POINTER;
    if (riid == IID_IUnknown || riid == IID_ICredentialProviderCredential)
    {
        *ppv = static_cast<ICredentialProviderCredential*>(this);
        AddRef();
        return S_OK;
    }
    *ppv = nullptr;
    return E_NOINTERFACE;
}

ULONG CAnshuBioCredential::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

ULONG CAnshuBioCredential::Release()
{
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (cRef == 0)
    {
        delete this;
    }
    return cRef;
}

HRESULT CAnshuBioCredential::Advise(ICredentialProviderCredentialEvents* pcpce)
{
    if (m_pcpce)
    {
        m_pcpce->Release();
        m_pcpce = nullptr;
    }
    m_pcpce = pcpce;
    if (m_pcpce)
    {
        m_pcpce->AddRef();
    }
    return S_OK;
}

HRESULT CAnshuBioCredential::UnAdvise()
{
    if (m_pcpce)
    {
        m_pcpce->Release();
        m_pcpce = nullptr;
    }
    return S_OK;
}

HRESULT CAnshuBioCredential::SetSelected(BOOL* pbAutoLogon)
{
    if (pbAutoLogon) *pbAutoLogon = FALSE;

    // Start background monitor thread to wait for phone unlock signal over named pipe
    if (!m_hAuthThread)
    {
        m_bStopAuthThread = FALSE;
        m_hAuthThread = CreateThread(nullptr, 0, AuthMonitorThreadProc, this, 0, nullptr);
    }
    return S_OK;
}

HRESULT CAnshuBioCredential::SetDeselected()
{
    if (m_hAuthThread)
    {
        m_bStopAuthThread = TRUE;
        WaitForSingleObject(m_hAuthThread, 500);
        CloseHandle(m_hAuthThread);
        m_hAuthThread = nullptr;
    }
    return S_OK;
}

HRESULT CAnshuBioCredential::GetFieldState(DWORD dwFieldID, CREDENTIAL_PROVIDER_FIELD_STATE* pcpfs, CREDENTIAL_PROVIDER_FIELD_INTERACTIVE_STATE* pcpfis)
{
    if (!pcpfs || !pcpfis) return E_POINTER;
    *pcpfs = CPFS_DISPLAY_IN_BOTH;
    *pcpfis = CPFIS_NONE;

    switch (dwFieldID)
    {
    case ABFI_LOGO:
    case ABFI_LABEL:
    case ABFI_STATUS:
    case ABFI_SUBMIT_BUTTON:
        *pcpfs = CPFS_DISPLAY_IN_BOTH;
        break;
    default:
        *pcpfs = CPFS_HIDDEN;
        break;
    }
    return S_OK;
}

HRESULT CAnshuBioCredential::GetStringValue(DWORD dwFieldID, PWSTR* ppsz)
{
    if (!ppsz) return E_POINTER;
    *ppsz = nullptr;

    const WCHAR* pszSource = nullptr;
    switch (dwFieldID)
    {
    case ABFI_LABEL:
        pszSource = L"AnshuBio Unlock";
        break;
    case ABFI_STATUS:
        pszSource = m_szStatusText;
        break;
    default:
        return E_INVALIDARG;
    }

    size_t cbSize = (wcslen(pszSource) + 1) * sizeof(WCHAR);
    *ppsz = (PWSTR)CoTaskMemAlloc(cbSize);
    if (!*ppsz) return E_OUTOFMEMORY;
    StringCbCopyW(*ppsz, cbSize, pszSource);

    return S_OK;
}

HRESULT CAnshuBioCredential::GetBitmapValue(DWORD dwFieldID, HBITMAP* phbmp)
{
    if (!phbmp) return E_POINTER;
    *phbmp = nullptr;
    return E_NOTIMPL;
}

HRESULT CAnshuBioCredential::GetCheckboxValue(DWORD dwFieldID, BOOL* pbChecked, PWSTR* ppszLabel)
{
    return E_NOTIMPL;
}

HRESULT CAnshuBioCredential::GetSubmitButtonValue(DWORD dwFieldID, DWORD* pdwAdjacentTo)
{
    if (!pdwAdjacentTo) return E_POINTER;
    if (dwFieldID == ABFI_SUBMIT_BUTTON)
    {
        *pdwAdjacentTo = ABFI_STATUS;
        return S_OK;
    }
    return E_INVALIDARG;
}

HRESULT CAnshuBioCredential::GetComboBoxValueCount(DWORD dwFieldID, DWORD* pcItems, DWORD* pdwSelectedItem)
{
    return E_NOTIMPL;
}

HRESULT CAnshuBioCredential::GetComboBoxValueAt(DWORD dwFieldID, DWORD dwItem, PWSTR* ppszItem)
{
    return E_NOTIMPL;
}

HRESULT CAnshuBioCredential::SetStringValue(DWORD dwFieldID, PCWSTR psz)
{
    return E_NOTIMPL;
}

HRESULT CAnshuBioCredential::SetCheckboxValue(DWORD dwFieldID, BOOL bChecked)
{
    return E_NOTIMPL;
}

HRESULT CAnshuBioCredential::SetComboBoxSelectedValue(DWORD dwFieldID, DWORD dwSelectedItem)
{
    return E_NOTIMPL;
}

HRESULT CAnshuBioCredential::CommandLinkClicked(DWORD dwFieldID)
{
    return E_NOTIMPL;
}

#ifndef NEGOSSP_NAME_A
#define NEGOSSP_NAME_A "Negotiate"
#endif

// Connect untrusted to LSA and lookup Negotiate or MSV1_0 authentication package
static HRESULT RetrieveNegotiateAuthPackage(ULONG* pulAuthPackage)
{
    if (!pulAuthPackage) return E_POINTER;
    *pulAuthPackage = 0;

    HANDLE hLsa = nullptr;
    NTSTATUS status = LsaConnectUntrusted(&hLsa);
    if (status != STATUS_SUCCESS || !hLsa)
    {
        return HRESULT_FROM_WIN32(LsaNtStatusToWinError(status));
    }

    LSA_STRING strPackageName;
    InitLsaString(&strPackageName, NEGOSSP_NAME_A);

    status = LsaLookupAuthenticationPackage(hLsa, &strPackageName, pulAuthPackage);
    if (status != STATUS_SUCCESS)
    {
        // Fallback to standard MSV1_0
        InitLsaString(&strPackageName, MSV1_0_PACKAGE_NAME);
        status = LsaLookupAuthenticationPackage(hLsa, &strPackageName, pulAuthPackage);
    }

    LsaDeregisterLogonProcess(hLsa);
    return (status == STATUS_SUCCESS) ? S_OK : HRESULT_FROM_WIN32(LsaNtStatusToWinError(status));
}

HRESULT CAnshuBioCredential::CreateSerializedLogon(CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION* pcpcs)
{
    if (!pcpcs) return E_POINTER;

    ULONG ulAuthPackage = 0;
    HRESULT hr = RetrieveNegotiateAuthPackage(&ulAuthPackage);
    if (FAILED(hr)) return hr;

    // Allocate and build KERB_INTERACTIVE_UNLOCK_LOGON payload
    DWORD cbDomain = (DWORD)(wcslen(m_szDomain) * sizeof(WCHAR));
    DWORD cbUser = (DWORD)(wcslen(m_szUsername) * sizeof(WCHAR));
    DWORD cbPassword = (DWORD)(wcslen(m_szAuthToken) * sizeof(WCHAR));

    DWORD cbBuffer = sizeof(KERB_INTERACTIVE_UNLOCK_LOGON) + cbDomain + sizeof(WCHAR) + cbUser + sizeof(WCHAR) + cbPassword + sizeof(WCHAR);

    BYTE* pBuffer = (BYTE*)CoTaskMemAlloc(cbBuffer);
    if (!pBuffer) return E_OUTOFMEMORY;
    ZeroMemory(pBuffer, cbBuffer);

    KERB_INTERACTIVE_UNLOCK_LOGON* pLogon = (KERB_INTERACTIVE_UNLOCK_LOGON*)pBuffer;
    pLogon->Logon.MessageType = KerbInteractiveLogon;

    BYTE* pCursor = pBuffer + sizeof(KERB_INTERACTIVE_UNLOCK_LOGON);

    // Copy Domain
    if (cbDomain > 0)
    {
        InitUnicodeString(&pLogon->Logon.LogonDomainName, (PWSTR)pCursor, (USHORT)cbDomain);
        memcpy(pCursor, m_szDomain, cbDomain);
        pCursor += cbDomain + sizeof(WCHAR);
    }

    // Copy User
    if (cbUser > 0)
    {
        InitUnicodeString(&pLogon->Logon.UserName, (PWSTR)pCursor, (USHORT)cbUser);
        memcpy(pCursor, m_szUsername, cbUser);
        pCursor += cbUser + sizeof(WCHAR);
    }

    // Copy Password
    if (cbPassword > 0)
    {
        InitUnicodeString(&pLogon->Logon.Password, (PWSTR)pCursor, (USHORT)cbPassword);
        memcpy(pCursor, m_szAuthToken, cbPassword);
        pCursor += cbPassword + sizeof(WCHAR);
    }

    pcpcs->ulAuthenticationPackage = ulAuthPackage;
    pcpcs->cbSerialization = cbBuffer;
    pcpcs->rgbSerialization = pBuffer;
    pcpcs->clsidCredentialProvider = CLSID_AnshuBioCredentialProvider;

    return S_OK;
}

HRESULT CAnshuBioCredential::GetSerialization(
    CREDENTIAL_PROVIDER_GET_SERIALIZATION_RESPONSE* pcpgsr,
    CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION* pcpcs,
    PWSTR* ppszOptionalStatusText,
    CREDENTIAL_PROVIDER_STATUS_ICON* pcpsiOptionalStatusIcon)
{
    if (!pcpgsr || !pcpcs || !ppszOptionalStatusText || !pcpsiOptionalStatusIcon) return E_POINTER;

    *pcpgsr = CPGSR_NO_CREDENTIAL_FINISHED;
    ZeroMemory(pcpcs, sizeof(*pcpcs));
    *ppszOptionalStatusText = nullptr;
    *pcpsiOptionalStatusIcon = CPSI_NONE;

    if (m_bIsAuthenticated)
    {
        HRESULT hr = CreateSerializedLogon(pcpcs);
        if (SUCCEEDED(hr))
        {
            *pcpgsr = CPGSR_RETURN_CREDENTIAL_FINISHED;
            *pcpsiOptionalStatusIcon = CPSI_SUCCESS;
            return S_OK;
        }
    }

    return S_OK;
}

HRESULT CAnshuBioCredential::ReportResult(
    NTSTATUS ntsStatus,
    NTSTATUS ntsSubstatus,
    PWSTR* ppszOptionalStatusText,
    CREDENTIAL_PROVIDER_STATUS_ICON* pcpsiOptionalStatusIcon)
{
    if (!ppszOptionalStatusText || !pcpsiOptionalStatusIcon) return E_POINTER;

    *ppszOptionalStatusText = nullptr;
    *pcpsiOptionalStatusIcon = CPSI_NONE;

    if (ntsStatus == STATUS_SUCCESS)
    {
        UpdateStatusText(L"Windows unlocked successfully.");
        *pcpsiOptionalStatusIcon = CPSI_SUCCESS;
    }
    else
    {
        UpdateStatusText(L"Authentication failed. Try again.");
        *pcpsiOptionalStatusIcon = CPSI_ERROR;
    }

    m_bIsAuthenticated = FALSE;
    SecureZeroMemory(m_szAuthToken, sizeof(m_szAuthToken));

    return S_OK;
}

void CAnshuBioCredential::UpdateStatusText(PCWSTR pszText)
{
    if (pszText)
    {
        StringCchCopyW(m_szStatusText, ARRAYSIZE(m_szStatusText), pszText);
        if (m_pcpce)
        {
            m_pcpce->SetFieldString(this, ABFI_STATUS, m_szStatusText);
        }
    }
}

void CAnshuBioCredential::NotifyAuthenticationSuccess(PCWSTR pszUser, PCWSTR pszDomain, PCWSTR pszToken)
{
    m_bIsAuthenticated = TRUE;
    if (pszUser && wcslen(pszUser) > 0)
    {
        wcsncpy_s(m_szUsername, pszUser, _TRUNCATE);
    }
    if (pszDomain && wcslen(pszDomain) > 0)
    {
        wcsncpy_s(m_szDomain, pszDomain, _TRUNCATE);
    }
    if (pszToken && wcslen(pszToken) > 0)
    {
        wcsncpy_s(m_szAuthToken, pszToken, _TRUNCATE);
    }

    UpdateStatusText(L"Phone biometric verified. Unlocking Windows...");

    if (m_pcpce)
    {
        m_pcpce->SetFieldString(this, ABFI_STATUS, m_szStatusText);
    }
}

DWORD WINAPI CAnshuBioCredential::AuthMonitorThreadProc(LPVOID lpParam)
{
    CAnshuBioCredential* pThis = (CAnshuBioCredential*)lpParam;
    if (pThis)
    {
        pThis->RunAuthMonitor();
    }
    return 0;
}

#pragma pack(push, 1)
struct PipeAuthPacket {
    static const uint32_t MAGIC = 0xAB10C0DE;
    uint32_t magic;
    uint32_t status;
    uint32_t cbUsername;
    WCHAR szUsername[256];
    uint32_t cbDomain;
    WCHAR szDomain[256];
    uint32_t cbPassword;
    WCHAR szPassword[512];
    uint32_t cbPhoneName;
    char szPhoneName[128];
    uint32_t cbPhoneId;
    char szPhoneId[128];
};
#pragma pack(pop)

void CAnshuBioCredential::RunAuthMonitor()
{
    HANDLE hPipe = INVALID_HANDLE_VALUE;

    while (!m_bStopAuthThread)
    {
        if (hPipe == INVALID_HANDLE_VALUE)
        {
            hPipe = CreateFileW(
                ANSHUBIO_PIPE_NAME,
                GENERIC_READ | GENERIC_WRITE,
                0,
                nullptr,
                OPEN_EXISTING,
                0,
                nullptr
            );
        }

        if (hPipe != INVALID_HANDLE_VALUE)
        {
            const char szReq[] = "{\"command\":\"CP_WAIT_FOR_AUTH\"}\n";
            DWORD dwWritten = 0;
            if (WriteFile(hPipe, szReq, (DWORD)strlen(szReq), &dwWritten, nullptr))
            {
                PipeAuthPacket packet;
                ZeroMemory(&packet, sizeof(packet));
                DWORD dwRead = 0;
                if (ReadFile(hPipe, &packet, sizeof(packet), &dwRead, nullptr) && dwRead == sizeof(packet))
                {
                    if (packet.magic == PipeAuthPacket::MAGIC && packet.status == 1)
                    {
                        NotifyAuthenticationSuccess(packet.szUsername, packet.szDomain, packet.szPassword);
                        SecureZeroMemory(&packet, sizeof(packet));
                        break;
                    }
                }
                SecureZeroMemory(&packet, sizeof(packet));
            }
            CloseHandle(hPipe);
            hPipe = INVALID_HANDLE_VALUE;
        }

        Sleep(1000);
    }

    if (hPipe != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hPipe);
    }
}
