/**
 * AnshuBio Unlock - ICredentialProviderCredential Implementation Header
 * Publisher: AnshuCore
 */

#pragma once
#include <windows.h>
#include <credentialprovider.h>
#include <ntsecapi.h>
#include <cstdint>

// Field IDs for Credential Tile
enum ANSHUBIO_FIELD_ID
{
    ABFI_LOGO = 0,
    ABFI_LABEL = 1,
    ABFI_STATUS = 2,
    ABFI_SUBMIT_BUTTON = 3,
    ABFI_NUM_FIELDS = 4
};

class CAnshuBioCredential : public ICredentialProviderCredential
{
public:
    // IUnknown
    IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv);
    IFACEMETHODIMP_(ULONG) AddRef();
    IFACEMETHODIMP_(ULONG) Release();

    // ICredentialProviderCredential
    IFACEMETHODIMP Advise(ICredentialProviderCredentialEvents* pcpce);
    IFACEMETHODIMP UnAdvise();
    IFACEMETHODIMP SetSelected(BOOL* pbAutoLogon);
    IFACEMETHODIMP SetDeselected();
    IFACEMETHODIMP GetFieldState(DWORD dwFieldID, CREDENTIAL_PROVIDER_FIELD_STATE* pcpfs, CREDENTIAL_PROVIDER_FIELD_INTERACTIVE_STATE* pcpfis);
    IFACEMETHODIMP GetStringValue(DWORD dwFieldID, PWSTR* ppsz);
    IFACEMETHODIMP GetBitmapValue(DWORD dwFieldID, HBITMAP* phbmp);
    IFACEMETHODIMP GetCheckboxValue(DWORD dwFieldID, BOOL* pbChecked, PWSTR* ppszLabel);
    IFACEMETHODIMP GetSubmitButtonValue(DWORD dwFieldID, DWORD* pdwAdjacentTo);
    IFACEMETHODIMP GetComboBoxValueCount(DWORD dwFieldID, DWORD* pcItems, DWORD* pdwSelectedItem);
    IFACEMETHODIMP GetComboBoxValueAt(DWORD dwFieldID, DWORD dwItem, PWSTR* ppszItem);
    IFACEMETHODIMP SetStringValue(DWORD dwFieldID, PCWSTR psz);
    IFACEMETHODIMP SetCheckboxValue(DWORD dwFieldID, BOOL bChecked);
    IFACEMETHODIMP SetComboBoxSelectedValue(DWORD dwFieldID, DWORD dwSelectedItem);
    IFACEMETHODIMP CommandLinkClicked(DWORD dwFieldID);
    IFACEMETHODIMP GetSerialization(CREDENTIAL_PROVIDER_GET_SERIALIZATION_RESPONSE* pcpgsr, CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION* pcpcs, PWSTR* ppszOptionalStatusText, CREDENTIAL_PROVIDER_STATUS_ICON* pcpsiOptionalStatusIcon);
    IFACEMETHODIMP ReportResult(NTSTATUS ntsStatus, NTSTATUS ntsSubstatus, PWSTR* ppszOptionalStatusText, CREDENTIAL_PROVIDER_STATUS_ICON* pcpsiOptionalStatusIcon);

    CAnshuBioCredential();
    virtual ~CAnshuBioCredential();

    void UpdateStatusText(PCWSTR pszText);
    void NotifyAuthenticationSuccess(PCWSTR pszUser, PCWSTR pszDomain, PCWSTR pszToken);

private:
    LONG m_cRef;
    ICredentialProviderCredentialEvents* m_pcpce;
    WCHAR m_szStatusText[256];
    BOOL m_bIsAuthenticated;
    HANDLE m_hAuthThread;
    BOOL m_bStopAuthThread;

    // Authenticated credentials from secure local storage
    WCHAR m_szUsername[256];
    WCHAR m_szDomain[256];
    WCHAR m_szAuthToken[512];

    static DWORD WINAPI AuthMonitorThreadProc(LPVOID lpParam);
    void RunAuthMonitor();
    HRESULT CreateSerializedLogon(CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION* pcpcs);
};
