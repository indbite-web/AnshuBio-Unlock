#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace AnshuBio {

struct PcIdentity {
    std::string pcId;
    std::string pcName;
    std::string publicKeyPem;
    std::string privateKeyPem;
    std::string fingerprint;
    std::string createdAt;
};

struct TrustedPhone {
    std::string id;
    std::string name;
    std::string publicKeyPem;
    std::string pairedAt;
    std::string lastSeen;
    std::string transport; // "Wi-Fi/LAN" or "Bluetooth/RFCOMM"
    std::string status;    // "PAIRED" or "REVOKED"
};

struct AppSettings {
    std::string pcDisplayName;
    bool startWithWindows = true;
    bool protectionEnabled = true;
    bool wifiEnabled = true;
    bool bluetoothEnabled = true;
    bool soundEnabled = true;
    bool vibrationEnabled = true;
    bool firstLaunchComplete = false;
};

struct WindowsCredential {
    std::wstring username;
    std::wstring domain;
    std::wstring password;
    std::string updatedAt;
};

struct AuthChallenge {
    std::string challengeId;
    std::string challengeNonceHex;
    int64_t timestamp = 0;
    std::string pcId;
    std::string pcName;
    std::string state; // "PENDING", "COMPLETED", "EXPIRED"
};

struct LogEntry {
    int64_t timestamp = 0;
    std::string level; // "INFO", "SECURITY", "WARN", "ERROR"
    std::string tag;
    std::string message;
};

// Binary structured payload for secure Named Pipe IPC between Windows Service & Credential Provider
#pragma pack(push, 1)
struct PipeAuthPacket {
    static constexpr uint32_t MAGIC = 0xAB10C0DE;
    uint32_t magic = MAGIC;
    uint32_t status = 0; // 1 = AUTHENTICATED, 0 = TIMEOUT / FAILED
    uint32_t cbUsername = 0;
    wchar_t szUsername[256] = { 0 };
    uint32_t cbDomain = 0;
    wchar_t szDomain[256] = { 0 };
    uint32_t cbPassword = 0;
    wchar_t szPassword[512] = { 0 };
    uint32_t cbPhoneName = 0;
    char szPhoneName[128] = { 0 };
    uint32_t cbPhoneId = 0;
    char szPhoneId[128] = { 0 };
};
#pragma pack(pop)

} // namespace AnshuBio
