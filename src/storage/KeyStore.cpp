#include "KeyStore.h"
#include "SecurityLogger.h"
#include "../crypto/DPAPIVault.h"
#include "../crypto/CryptoEngine.h"
#include <windows.h>
#include <shlobj.h>
#include <fstream>
#include <sstream>

namespace AnshuBio {

KeyStore& KeyStore::Instance() {
    static KeyStore s_instance;
    return s_instance;
}

KeyStore::KeyStore() {
    EnsureDirectoryExists();
    Load();
}

KeyStore::~KeyStore() {
    Save();
}

std::string KeyStore::GetVaultFilePath() {
    wchar_t localAppData[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, localAppData))) {
        char pathA[MAX_PATH];
        WideCharToMultiByte(CP_UTF8, 0, localAppData, -1, pathA, MAX_PATH, nullptr, nullptr);
        return std::string(pathA) + "\\AnshuBio\\keys.vault";
    }
    return "keys.vault";
}

void KeyStore::EnsureDirectoryExists() {
    wchar_t localAppData[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, localAppData))) {
        std::wstring baseDir = std::wstring(localAppData) + L"\\AnshuBio";
        CreateDirectoryW(baseDir.c_str(), nullptr);
    }
}

void KeyStore::InitializeDefaultIdentity() {
    char computerName[MAX_COMPUTERNAME_LENGTH + 1] = { 0 };
    DWORD size = sizeof(computerName);
    GetComputerNameA(computerName, &size);

    std::string name = (size > 0) ? computerName : "Anshu-PC";
    m_settings.pcDisplayName = name;

    CryptoEngine engine;
    m_pcIdentity.pcId = CryptoEngine::GenerateUUID();
    m_pcIdentity.pcName = name;
    engine.GenerateECKeyPair(m_pcIdentity.publicKeyPem, m_pcIdentity.privateKeyPem, m_pcIdentity.fingerprint);
    m_pcIdentity.createdAt = "2026-08-15T00:00:00Z";

    SecurityLogger::Instance().Security("KEYSTORE", "Initialized brand new PC cryptographic identity: " + m_pcIdentity.pcId);
}

bool KeyStore::Load() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_isLoaded) return true;

    std::string vaultPath = GetVaultFilePath();
    std::ifstream ifs(vaultPath);
    if (ifs.is_open()) {
        std::stringstream buffer;
        buffer << ifs.rdbuf();
        ifs.close();

        std::string decrypted;
        if (DPAPIVault::Decrypt(buffer.str(), decrypted)) {
            std::istringstream iss(decrypted);
            std::string line;
            while (std::getline(iss, line)) {
                if (line.rfind("PC_ID=", 0) == 0) m_pcIdentity.pcId = line.substr(6);
                else if (line.rfind("PC_NAME=", 0) == 0) m_pcIdentity.pcName = line.substr(8);
                else if (line.rfind("PC_DISPLAY=", 0) == 0) m_settings.pcDisplayName = line.substr(11);
                else if (line.rfind("FINGERPRINT=", 0) == 0) m_pcIdentity.fingerprint = line.substr(12);
                else if (line.rfind("PHONE=", 0) == 0) {
                    std::string phoneData = line.substr(6);
                    std::istringstream pss(phoneData);
                    TrustedPhone phone;
                    std::getline(pss, phone.id, '|');
                    std::getline(pss, phone.name, '|');
                    std::getline(pss, phone.publicKeyPem, '|');
                    std::getline(pss, phone.status, '|');
                    phone.lastSeen = "Online";
                    phone.transport = "Wi-Fi/LAN";
                    if (!phone.id.empty() && m_trustedPhones.size() < 2) {
                        m_trustedPhones.push_back(phone);
                    }
                } else if (line.rfind("REVOKED=", 0) == 0) {
                    m_revokedPhoneIds.push_back(line.substr(8));
                } else if (line.rfind("REVOKED_KEY=", 0) == 0) {
                    m_revokedKeys.push_back(line.substr(12));
                } else if (line.rfind("WIN_CRED=", 0) == 0) {
                    std::string credData = line.substr(9);
                    std::istringstream css(credData);
                    std::string uA, dA, pA, upA;
                    std::getline(css, uA, '|');
                    std::getline(css, dA, '|');
                    std::getline(css, pA, '|');
                    std::getline(css, upA, '|');

                    wchar_t uW[256] = { 0 };
                    wchar_t dW[256] = { 0 };
                    wchar_t pW[512] = { 0 };
                    MultiByteToWideChar(CP_UTF8, 0, uA.c_str(), -1, uW, 256);
                    MultiByteToWideChar(CP_UTF8, 0, dA.c_str(), -1, dW, 256);
                    MultiByteToWideChar(CP_UTF8, 0, pA.c_str(), -1, pW, 512);

                    WindowsCredential cred;
                    cred.username = uW;
                    cred.domain = dW;
                    cred.password = pW;
                    cred.updatedAt = upA.empty() ? "2026-08-15" : upA;
                    m_windowsCredential = cred;

                    SecureZeroMemory(pW, sizeof(pW));
                    SecureZeroMemory(const_cast<char*>(pA.data()), pA.size());
                }
            }
            m_isLoaded = true;
            SecurityLogger::Instance().Info("KEYSTORE", "Vault decrypted and loaded successfully from machine DPAPI");
            return true;
        }
    }

    InitializeDefaultIdentity();
    m_isLoaded = true;
    return true;
}

bool KeyStore::Save() {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::stringstream ss;
    ss << "PC_ID=" << m_pcIdentity.pcId << "\n";
    ss << "PC_NAME=" << m_pcIdentity.pcName << "\n";
    ss << "PC_DISPLAY=" << m_settings.pcDisplayName << "\n";
    ss << "FINGERPRINT=" << m_pcIdentity.fingerprint << "\n";

    for (const auto& phone : m_trustedPhones) {
        ss << "PHONE=" << phone.id << "|" << phone.name << "|" << phone.publicKeyPem << "|" << phone.status << "\n";
    }
    for (const auto& revokedId : m_revokedPhoneIds) {
        ss << "REVOKED=" << revokedId << "\n";
    }
    for (const auto& revokedKey : m_revokedKeys) {
        ss << "REVOKED_KEY=" << revokedKey << "\n";
    }
    if (m_windowsCredential.has_value() && !m_windowsCredential->password.empty()) {
        char uA[256] = { 0 };
        char dA[256] = { 0 };
        char pA[512] = { 0 };
        WideCharToMultiByte(CP_UTF8, 0, m_windowsCredential->username.c_str(), -1, uA, sizeof(uA), nullptr, nullptr);
        WideCharToMultiByte(CP_UTF8, 0, m_windowsCredential->domain.c_str(), -1, dA, sizeof(dA), nullptr, nullptr);
        WideCharToMultiByte(CP_UTF8, 0, m_windowsCredential->password.c_str(), -1, pA, sizeof(pA), nullptr, nullptr);
        ss << "WIN_CRED=" << uA << "|" << dA << "|" << pA << "|" << m_windowsCredential->updatedAt << "\n";
        SecureZeroMemory(pA, sizeof(pA));
    }

    std::string plain = ss.str();
    std::string cipher;
    bool success = false;
    if (DPAPIVault::Encrypt(plain, cipher)) {
        std::ofstream ofs(GetVaultFilePath());
        if (ofs.is_open()) {
            ofs << cipher;
            ofs.close();
            success = true;
        }
    }
    SecureZeroMemory(const_cast<char*>(plain.data()), plain.size());
    return success;
}

PcIdentity KeyStore::GetPcIdentity() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_pcIdentity;
}

std::string KeyStore::GetPcDisplayName() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_settings.pcDisplayName.empty() ? m_pcIdentity.pcName : m_settings.pcDisplayName;
}

bool KeyStore::SetPcDisplayName(const std::string& name) {
    if (name.empty()) return false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_settings.pcDisplayName = name;
    }
    Save();
    SecurityLogger::Instance().Info("SETTINGS", "PC Display Name updated to: " + name);
    return true;
}

std::vector<TrustedPhone> KeyStore::GetTrustedPhones() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_trustedPhones;
}

std::optional<TrustedPhone> KeyStore::GetTrustedPhone(const std::string& phoneId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& phone : m_trustedPhones) {
        if (phone.id == phoneId) return phone;
    }
    return std::nullopt;
}

bool KeyStore::AddTrustedPhone(const TrustedPhone& phone) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_trustedPhones.size() >= 2) return false;
        for (const auto& rev : m_revokedPhoneIds) {
            if (rev == phone.id) return false;
        }
        for (const auto& revKey : m_revokedKeys) {
            if (revKey == phone.publicKeyPem || (!phone.publicKeyPem.empty() && revKey.find(phone.publicKeyPem) != std::string::npos)) {
                return false;
            }
        }
        for (auto& existing : m_trustedPhones) {
            if (existing.id == phone.id) {
                existing = phone;
                return true;
            }
        }
        m_trustedPhones.push_back(phone);
    }
    Save();
    SecurityLogger::Instance().Security("PHONE_MANAGER", "Paired trusted phone: " + phone.name + " (" + phone.id + ")");
    return true;
}

bool KeyStore::RemoveTrustedPhone(const std::string& phoneId) {
    bool removed = false;
    std::string name;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto it = m_trustedPhones.begin(); it != m_trustedPhones.end(); ++it) {
            if (it->id == phoneId) {
                name = it->name;
                m_trustedPhones.erase(it);
                removed = true;
                break;
            }
        }
    }
    if (removed) {
        Save();
        SecurityLogger::Instance().Security("PHONE_MANAGER", "Removed trusted phone: " + name + " (" + phoneId + ")");
    }
    return removed;
}

bool KeyStore::RevokeTrustedPhone(const std::string& phoneId) {
    bool revoked = false;
    std::string name;
    std::string pubKey;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto it = m_trustedPhones.begin(); it != m_trustedPhones.end(); ++it) {
            if (it->id == phoneId) {
                name = it->name;
                pubKey = it->publicKeyPem;
                m_trustedPhones.erase(it);
                break;
            }
        }
        m_revokedPhoneIds.push_back(phoneId);
        if (!pubKey.empty()) {
            m_revokedKeys.push_back(pubKey);
        }
        revoked = true;
    }
    if (revoked) {
        Save();
        SecurityLogger::Instance().Security("PHONE_MANAGER", "Permanently revoked phone: " + name + " (" + phoneId + ") with cryptographic key blacklist");
    }
    return revoked;
}

bool KeyStore::UpdatePhoneLastSeen(const std::string& phoneId, const std::string& transport) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& phone : m_trustedPhones) {
        if (phone.id == phoneId) {
            phone.lastSeen = "Just now";
            phone.transport = transport;
            return true;
        }
    }
    return false;
}

bool KeyStore::IsPhoneRevoked(const std::string& phoneId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& id : m_revokedPhoneIds) {
        if (id == phoneId) return true;
    }
    return false;
}

bool KeyStore::IsKeyRevoked(const std::string& publicKeyPem) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& key : m_revokedKeys) {
        if (key == publicKeyPem || (!publicKeyPem.empty() && key.find(publicKeyPem) != std::string::npos)) return true;
    }
    return false;
}

bool KeyStore::SetWindowsCredential(const std::wstring& username, const std::wstring& domain, const std::wstring& password) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        WindowsCredential cred;
        cred.username = username;
        cred.domain = domain;
        cred.password = password;
        cred.updatedAt = "2026-08-15";
        m_windowsCredential = cred;
    }
    Save();
    SecurityLogger::Instance().Security("CREDENTIAL_VAULT", "Updated and saved machine-bound DPAPI Windows credentials");
    return true;
}

std::optional<WindowsCredential> KeyStore::GetWindowsCredential() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_windowsCredential;
}

bool KeyStore::HasWindowsCredential() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_windowsCredential.has_value() && !m_windowsCredential->password.empty();
}

bool KeyStore::ClearWindowsCredential() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_windowsCredential.has_value()) {
            SecureZeroMemory(const_cast<wchar_t*>(m_windowsCredential->password.data()), m_windowsCredential->password.size() * sizeof(wchar_t));
            m_windowsCredential.reset();
        }
    }
    Save();
    SecurityLogger::Instance().Security("CREDENTIAL_VAULT", "Cleared Windows credentials from DPAPI vault");
    return true;
}

AppSettings KeyStore::GetSettings() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_settings;
}

bool KeyStore::UpdateSettings(const AppSettings& settings) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_settings = settings;
    }
    Save();
    return true;
}

} // namespace AnshuBio
