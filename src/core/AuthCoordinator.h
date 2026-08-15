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
    std::string confirmCode;
    TrustedPhone pendingPhone;
    bool pcConfirmed = false;
    bool phoneConfirmed = false;
    int64_t createdAt = 0;
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
    bool ConfirmPairingFromPc(const std::string& sessionId);
    void CancelPairing();

    std::optional<AuthChallenge> GetActiveChallenge() const;

private:
    AuthCoordinator();
    ~AuthCoordinator();

    mutable std::mutex m_mutex;
    CryptoEngine m_cryptoEngine;
    std::optional<AuthChallenge> m_activeChallenge;
    std::optional<PairingSession> m_pendingPairing;

    WiFiServer* m_wifiServer = nullptr;
    BluetoothServer* m_btServer = nullptr;
    bool m_isRunning = false;
};

} // namespace AnshuBio
