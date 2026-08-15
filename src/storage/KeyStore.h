#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <optional>
#include "../core/Models.h"

namespace AnshuBio {

class KeyStore {
public:
    static KeyStore& Instance();

    bool Load();
    bool Save();

    // PC Identity
    PcIdentity GetPcIdentity();
    std::string GetPcDisplayName();
    bool SetPcDisplayName(const std::string& name);

    // Trusted Phones
    std::vector<TrustedPhone> GetTrustedPhones();
    std::optional<TrustedPhone> GetTrustedPhone(const std::string& phoneId);
    bool AddTrustedPhone(const TrustedPhone& phone);
    bool RemoveTrustedPhone(const std::string& phoneId);
    bool RevokeTrustedPhone(const std::string& phoneId);
    bool UpdatePhoneLastSeen(const std::string& phoneId, const std::string& transport);
    bool IsPhoneRevoked(const std::string& phoneId);
    bool IsKeyRevoked(const std::string& publicKeyPem);

    // Windows Credential Vault
    bool SetWindowsCredential(const std::wstring& username, const std::wstring& domain, const std::wstring& password);
    std::optional<WindowsCredential> GetWindowsCredential();
    bool HasWindowsCredential();
    bool ClearWindowsCredential();

    // Settings
    AppSettings GetSettings();
    bool UpdateSettings(const AppSettings& settings);

private:
    KeyStore();
    ~KeyStore();

    void InitializeDefaultIdentity();
    std::string GetVaultFilePath();
    void EnsureDirectoryExists();

    std::mutex m_mutex;
    bool m_isLoaded = false;
    PcIdentity m_pcIdentity;
    std::vector<TrustedPhone> m_trustedPhones;
    std::vector<std::string> m_revokedPhoneIds;
    std::vector<std::string> m_revokedKeys;
    std::optional<WindowsCredential> m_windowsCredential;
    AppSettings m_settings;
};

} // namespace AnshuBio
