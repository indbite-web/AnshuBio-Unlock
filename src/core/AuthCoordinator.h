#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <optional>
#include "Models.h"
#include "../crypto/CryptoEngine.h"

namespace AnshuBio {

class WiFiServer;
class BluetoothServer;

struct PairingSession {
    std::string sessionId;
    std::string nonce;
    std::string confirmCode;
    std::string qrPayload;
    std::string ipAddress;
    int port = 42425;
    std::string btUuid = "00001101-0000-1000-8000-00805F9B34FB";
    std::string pcId;
    std::string pcName;
    std::string fingerprint;
    int64_t createdAt = 0;
    int64_t expiresAt = 0;
    TrustedPhone pendingPhone;
    bool pcConfirmed = false;
    bool phoneConfirmed = false;
    std::string status = "WAITING_FOR_PHONE"; // "WAITING_FOR_PHONE", "PHONE_FOUND", "CONFIRMATION_REQUIRED", "PAIRED", "EXPIRED", "CANCELLED"
};

class AuthCoordinator {
public:
    static AuthCoordinator& Instance();

    bool Start();
    void Stop();

    void SetTransports(WiFiServer* wifi, BluetoothServer* bt);

    // Session event handlers
    void OnPcLocked();
    void OnPcUnlocked();
    void OnPcSleep();
    void OnPcWake();

    // Protocol message processing
    std::string ProcessIncomingJsonMessage(const std::string& jsonStr, const std::string& transportName);
    std::string GetPendingChallengeJson();

    // Direct handlers
    std::string HandlePairingRequest(const std::string& phoneId, const std::string& phoneName, const std::string& publicKeyPem, const std::string& confirmCode);
    std::string HandlePairingConfirmFromPhone(const std::string& sessionId, const std::string& phoneId);
    std::string HandleAuthResponse(const std::string& challengeId, const std::string& phoneId, int64_t timestamp, const std::string& signatureHex);
    std::string HandleManualLock(const std::string& phoneId, int64_t timestamp, const std::string& signatureHex);

    // Pairing workflow
    std::optional<PairingSession> InitiatePairingSession();
    std::optional<PairingSession> GetActivePairingSession() const;
    bool ConfirmPairingFromPc(const std::string& sessionId);
    void CancelPairing();

    std::optional<AuthChallenge> GetActiveChallenge() const;

private:
    AuthCoordinator();
    ~AuthCoordinator();

    mutable std::mutex m_mutex;
    CryptoEngine m_cryptoEngine;
    WiFiServer* m_wifiServer = nullptr;
    BluetoothServer* m_btServer = nullptr;

    std::optional<AuthChallenge> m_activeChallenge;
    std::optional<PairingSession> m_pendingPairing;
    bool m_isRunning = false;
};

} // namespace AnshuBio
