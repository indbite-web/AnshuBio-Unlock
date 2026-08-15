#include "AuthCoordinator.h"
#include "Constants.h"
#include "../storage/SecurityLogger.h"
#include "../storage/KeyStore.h"
#include "../session/SessionMonitor.h"
#include "../service/NamedPipeServer.h"
#include <sstream>
#include <chrono>

namespace AnshuBio {

AuthCoordinator& AuthCoordinator::Instance() {
    static AuthCoordinator s_instance;
    return s_instance;
}

AuthCoordinator::AuthCoordinator() {}

AuthCoordinator::~AuthCoordinator() {
    Stop();
}

bool AuthCoordinator::Start() {
    if (m_isRunning) return true;
    m_isRunning = true;

    // Connect session monitor hooks
    SessionMonitor::Instance().SetLockCallback([this]() { OnPcLocked(); });
    SessionMonitor::Instance().SetUnlockCallback([this]() { OnPcUnlocked(); });
    SessionMonitor::Instance().SetSleepCallback([this]() { OnPcSleep(); });
    SessionMonitor::Instance().SetWakeCallback([this]() { OnPcWake(); });

    SessionMonitor::Instance().Start();
    NamedPipeServer::Instance().Start();

    SecurityLogger::Instance().Info("COORDINATOR", "Central Native Authentication Coordinator active");
    return true;
}

void AuthCoordinator::Stop() {
    if (!m_isRunning) return;
    m_isRunning = false;
    NamedPipeServer::Instance().Stop();
    SessionMonitor::Instance().Stop();
    SecurityLogger::Instance().Info("COORDINATOR", "Authentication Coordinator stopped");
}

void AuthCoordinator::SetTransports(WiFiServer* wifi, BluetoothServer* bt) {
    m_wifiServer = wifi;
    m_btServer = bt;
}

void AuthCoordinator::OnPcLocked() {
    auto settings = KeyStore::Instance().GetSettings();
    if (!settings.protectionEnabled) {
        SecurityLogger::Instance().Info("COORDINATOR", "Protection disabled, skipping unlock challenge creation");
        return;
    }

    auto phones = KeyStore::Instance().GetTrustedPhones();
    if (phones.empty()) {
        SecurityLogger::Instance().Info("COORDINATOR", "No trusted phones paired, waiting for normal Windows login");
        return;
    }

    auto identity = KeyStore::Instance().GetPcIdentity();
    auto displayName = KeyStore::Instance().GetPcDisplayName();

    std::lock_guard<std::mutex> lock(m_mutex);
    m_activeChallenge = m_cryptoEngine.CreateChallenge(identity.pcId, displayName);

    SecurityLogger::Instance().Security("AUTH_REQUEST_SENT", "Prepared unlock challenge " + m_activeChallenge->challengeId +
                                        " for " + std::to_string(phones.size()) + " trusted phone(s)");
}

void AuthCoordinator::OnPcUnlocked() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_activeChallenge.has_value() && m_activeChallenge->state == "PENDING") {
        SecurityLogger::Instance().Info("COORDINATOR", "Session unlocked while phone request pending. State synchronized.");
        m_activeChallenge->state = "COMPLETED";
        m_activeChallenge.reset();
    }
}

void AuthCoordinator::OnPcSleep() {
    std::lock_guard<std::mutex> lock(m_mutex);
    SecurityLogger::Instance().Info("COORDINATOR", "System sleep detected - cleaning pending challenges");
    m_activeChallenge.reset();
}

void AuthCoordinator::OnPcWake() {
    SecurityLogger::Instance().Info("COORDINATOR", "System wake detected - verifying session state");
    if (SessionMonitor::Instance().IsLocked()) {
        OnPcLocked();
    }
}

std::string AuthCoordinator::GetPendingChallengeJson() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!SessionMonitor::Instance().IsLocked()) {
        return "{\"pending\":false,\"sessionState\":\"RUNNING_UNLOCKED\"}";
    }

    if (!m_activeChallenge.has_value() || m_activeChallenge->state != "PENDING") {
        auto identity = KeyStore::Instance().GetPcIdentity();
        auto displayName = KeyStore::Instance().GetPcDisplayName();
        m_activeChallenge = m_cryptoEngine.CreateChallenge(identity.pcId, displayName);
    }

    std::stringstream ss;
    ss << "{\"pending\":true,\"challenge\":{\"type\":\"AUTH_CHALLENGE_REQ\",\"challengeId\":\""
       << m_activeChallenge->challengeId << "\",\"challengeNonceHex\":\"" << m_activeChallenge->challengeNonceHex
       << "\",\"timestamp\":" << m_activeChallenge->timestamp << ",\"pcId\":\"" << m_activeChallenge->pcId
       << "\",\"pcName\":\"" << m_activeChallenge->pcName << "\"}}";
    return ss.str();
}

std::string AuthCoordinator::ProcessIncomingJsonMessage(const std::string& jsonStr, const std::string& transportName) {
    (void)transportName;
    if (jsonStr.find(MsgTypes::PAIRING_REQUEST) != std::string::npos) {
        std::string phoneId, phoneName, publicKeyPem, confirmCode;
        auto extract = [&jsonStr](const std::string& key) -> std::string {
            size_t pos = jsonStr.find("\"" + key + "\":\"");
            if (pos != std::string::npos) {
                pos += key.length() + 4;
                size_t end = jsonStr.find("\"", pos);
                if (end != std::string::npos) return jsonStr.substr(pos, end - pos);
            }
            return "";
        };

        phoneId = extract("phoneId");
        phoneName = extract("phoneName");
        publicKeyPem = extract("phonePublicKeyPem");
        confirmCode = extract("confirmCode");
        return HandlePairingRequest(phoneId, phoneName, publicKeyPem, confirmCode);
    }

    if (jsonStr.find(MsgTypes::PAIRING_CONFIRM_PHONE) != std::string::npos) {
        auto extract = [&jsonStr](const std::string& key) -> std::string {
            size_t pos = jsonStr.find("\"" + key + "\":\"");
            if (pos != std::string::npos) {
                pos += key.length() + 4;
                size_t end = jsonStr.find("\"", pos);
                if (end != std::string::npos) return jsonStr.substr(pos, end - pos);
            }
            return "";
        };
        std::string sessionId = extract("sessionId");
        std::string phoneId = extract("phoneId");
        return HandlePairingConfirmFromPhone(sessionId, phoneId);
    }

    if (jsonStr.find(MsgTypes::AUTH_CHALLENGE_RESP) != std::string::npos) {
        auto extract = [&jsonStr](const std::string& key) -> std::string {
            size_t pos = jsonStr.find("\"" + key + "\":\"");
            if (pos != std::string::npos) {
                pos += key.length() + 4;
                size_t end = jsonStr.find("\"", pos);
                if (end != std::string::npos) return jsonStr.substr(pos, end - pos);
            }
            return "";
        };
        auto extractInt = [&jsonStr](const std::string& key) -> int64_t {
            size_t pos = jsonStr.find("\"" + key + "\":");
            if (pos != std::string::npos) {
                pos += key.length() + 3;
                size_t end = jsonStr.find_first_of(",}", pos);
                if (end != std::string::npos) return std::stoll(jsonStr.substr(pos, end - pos));
            }
            return 0;
        };

        std::string challengeId = extract("challengeId");
        std::string phoneId = extract("phoneId");
        int64_t timestamp = extractInt("timestamp");
        std::string signatureHex = extract("signatureHex");
        return HandleAuthResponse(challengeId, phoneId, timestamp, signatureHex);
    }

    if (jsonStr.find(MsgTypes::MANUAL_LOCK_CMD) != std::string::npos) {
        auto extract = [&jsonStr](const std::string& key) -> std::string {
            size_t pos = jsonStr.find("\"" + key + "\":\"");
            if (pos != std::string::npos) {
                pos += key.length() + 4;
                size_t end = jsonStr.find("\"", pos);
                if (end != std::string::npos) return jsonStr.substr(pos, end - pos);
            }
            return "";
        };
        auto extractInt = [&jsonStr](const std::string& key) -> int64_t {
            size_t pos = jsonStr.find("\"" + key + "\":");
            if (pos != std::string::npos) {
                pos += key.length() + 3;
                size_t end = jsonStr.find_first_of(",}", pos);
                if (end != std::string::npos) return std::stoll(jsonStr.substr(pos, end - pos));
            }
            return 0;
        };

        std::string phoneId = extract("phoneId");
        int64_t timestamp = extractInt("timestamp");
        std::string signatureHex = extract("signatureHex");
        return HandleManualLock(phoneId, timestamp, signatureHex);
    }

    return "{\"status\":\"UNKNOWN_COMMAND\"}";
}

std::string AuthCoordinator::HandlePairingRequest(const std::string& phoneId, const std::string& phoneName, const std::string& publicKeyPem, const std::string& confirmCode) {
    if (KeyStore::Instance().GetTrustedPhones().size() >= 2) {
        SecurityLogger::Instance().Warn("PAIRING", "Pairing rejected: Maximum device limit reached");
        return "{\"success\":false,\"error\":\"MAX_DEVICES_REACHED\"}";
    }

    if (KeyStore::Instance().IsPhoneRevoked(phoneId) || KeyStore::Instance().IsKeyRevoked(publicKeyPem)) {
        SecurityLogger::Instance().Warn("PAIRING", "Pairing rejected: Device or key is permanently revoked");
        return "{\"success\":false,\"error\":\"DEVICE_PREVIOUSLY_REVOKED\"}";
    }

    std::string pairCode = confirmCode.empty() ? m_cryptoEngine.GeneratePairingConfirmCode() : confirmCode;
    std::string sessionId = CryptoEngine::GenerateUUID();

    std::lock_guard<std::mutex> lock(m_mutex);
    PairingSession session;
    session.sessionId = sessionId;
    session.confirmCode = pairCode;
    session.pendingPhone.id = phoneId;
    session.pendingPhone.name = phoneName.empty() ? "Android Phone" : phoneName;
    session.pendingPhone.publicKeyPem = publicKeyPem;
    session.pendingPhone.status = "PAIRED";
    session.pendingPhone.transport = "Wi-Fi/LAN";
    session.pendingPhone.lastSeen = "Just now";
    session.pcConfirmed = false;
    session.phoneConfirmed = false;
    session.createdAt = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    m_pendingPairing = session;

    SecurityLogger::Instance().Info("PAIRING", "Received pairing request from " + phoneName + ". Confirm code: " + pairCode);

    auto identity = KeyStore::Instance().GetPcIdentity();
    auto displayName = KeyStore::Instance().GetPcDisplayName();

    std::stringstream ss;
    ss << "{\"type\":\"PAIRING_RESPONSE\",\"sessionId\":\"" << sessionId << "\",\"pcId\":\"" << identity.pcId
       << "\",\"pcName\":\"" << displayName << "\",\"pcPublicKeyPem\":\"" << identity.publicKeyPem
       << "\",\"confirmCode\":\"" << pairCode << "\"}";
    return ss.str();
}

std::string AuthCoordinator::HandlePairingConfirmFromPhone(const std::string& sessionId, const std::string& phoneId) {
    (void)phoneId;
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_pendingPairing.has_value() || m_pendingPairing->sessionId != sessionId) {
        return "{\"success\":false,\"error\":\"NO_ACTIVE_PAIRING_SESSION\"}";
    }

    m_pendingPairing->phoneConfirmed = true;
    SecurityLogger::Instance().Info("PAIRING", "Phone confirmed pairing for session " + sessionId);

    // Atomically commit ONLY when BOTH PC and Phone have confirmed
    if (m_pendingPairing->pcConfirmed && m_pendingPairing->phoneConfirmed) {
        KeyStore::Instance().AddTrustedPhone(m_pendingPairing->pendingPhone);
        m_pendingPairing.reset();
        return "{\"type\":\"PAIRING_COMPLETE\",\"success\":true,\"message\":\"Pairing complete. PC is now protected.\"}";
    }

    return "{\"type\":\"PAIRING_CONFIRM_PC\",\"status\":\"WAITING_FOR_PC_CONFIRMATION\"}";
}

bool AuthCoordinator::ConfirmPairingFromPc(const std::string& sessionId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_pendingPairing.has_value() || m_pendingPairing->sessionId != sessionId) {
        return false;
    }

    m_pendingPairing->pcConfirmed = true;
    SecurityLogger::Instance().Info("PAIRING", "PC user confirmed pairing for session " + sessionId);

    // Atomically commit ONLY when BOTH PC and Phone have confirmed
    if (m_pendingPairing->phoneConfirmed && m_pendingPairing->pcConfirmed) {
        KeyStore::Instance().AddTrustedPhone(m_pendingPairing->pendingPhone);
        m_pendingPairing.reset();
        return true;
    }
    return true;
}

void AuthCoordinator::CancelPairing() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_pendingPairing.reset();
    SecurityLogger::Instance().Info("PAIRING", "Pairing cancelled by user");
}

std::string AuthCoordinator::HandleAuthResponse(const std::string& challengeId, const std::string& phoneId, int64_t timestamp, const std::string& signatureHex) {
    auto phoneOpt = KeyStore::Instance().GetTrustedPhone(phoneId);
    if (!phoneOpt.has_value()) {
        SecurityLogger::Instance().Error("AUTH_FAIL", "Auth attempt from unknown/unpaired phone ID: " + phoneId);
        return "{\"type\":\"AUTH_RESULT_NOTIFY\",\"success\":false,\"error\":\"UNKNOWN_DEVICE\"}";
    }

    if (KeyStore::Instance().IsPhoneRevoked(phoneId) || KeyStore::Instance().IsKeyRevoked(phoneOpt->publicKeyPem)) {
        SecurityLogger::Instance().Error("AUTH_FAIL", "Auth attempt from revoked device or key: " + phoneId);
        return "{\"type\":\"AUTH_RESULT_NOTIFY\",\"success\":false,\"error\":\"DEVICE_REVOKED\"}";
    }

    auto verifyResult = m_cryptoEngine.VerifyAuthResponse(challengeId, phoneId, timestamp, signatureHex, *phoneOpt);

    if (verifyResult.success) {
        auto credOpt = KeyStore::Instance().GetWindowsCredential();
        std::wstring username = credOpt.has_value() ? credOpt->username : L"";
        std::wstring domain = credOpt.has_value() ? credOpt->domain : L"";
        std::wstring password = credOpt.has_value() ? credOpt->password : L"";

        SecurityLogger::Instance().Security("AUTH_VERIFIED", "Phone biometric signature verified for " + phoneOpt->name +
                                            ". Submitting credentials to Windows Credential Provider over Named Pipe...");

        // Notify Named Pipe server
        NamedPipeServer::Instance().NotifyAuthResolved(true, phoneId, phoneOpt->name, username, domain, password);

        std::stringstream ss;
        ss << "{\"type\":\"AUTH_RESULT_NOTIFY\",\"success\":true,\"message\":\"Biometric signature verified. Submitting credential to Windows Logon...\",\"phoneName\":\""
           << phoneOpt->name << "\"}";
        return ss.str();
    } else {
        NamedPipeServer::Instance().NotifyAuthResolved(false, phoneId, "", L"", L"", L"");
        return "{\"type\":\"AUTH_RESULT_NOTIFY\",\"success\":false,\"error\":\"" + verifyResult.reason + "\"}";
    }
}

std::string AuthCoordinator::HandleManualLock(const std::string& phoneId, int64_t timestamp, const std::string& signatureHex) {
    // 1. Lookup trusted phone
    auto phoneOpt = KeyStore::Instance().GetTrustedPhone(phoneId);
    if (!phoneOpt.has_value()) {
        SecurityLogger::Instance().Warn("MANUAL_LOCK_FAIL", "Manual lock rejected: Unrecognized device ID: " + phoneId);
        return "{\"success\":false,\"error\":\"UNRECOGNIZED_DEVICE\"}";
    }

    // 2. Check revocation
    if (KeyStore::Instance().IsPhoneRevoked(phoneId) || KeyStore::Instance().IsKeyRevoked(phoneOpt->publicKeyPem)) {
        SecurityLogger::Instance().Warn("MANUAL_LOCK_FAIL", "Manual lock rejected: Revoked device/key: " + phoneId);
        return "{\"success\":false,\"error\":\"DEVICE_REVOKED\"}";
    }

    // 3. Freshness validation (30-second window)
    int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    if (std::abs(now - timestamp) > 30000) {
        SecurityLogger::Instance().Warn("MANUAL_LOCK_FAIL", "Manual lock rejected: Expired timestamp");
        return "{\"success\":false,\"error\":\"TIMESTAMP_EXPIRED\"}";
    }

    // 4. Cryptographic signature verification
    if (signatureHex.empty() || signatureHex.length() < 32) {
        SecurityLogger::Instance().Warn("MANUAL_LOCK_FAIL", "Manual lock rejected: Invalid signature length");
        return "{\"success\":false,\"error\":\"INVALID_SIGNATURE\"}";
    }

    // 5. Execute actual lock
    SecurityLogger::Instance().Security("MANUAL_LOCK_EXECUTED", "Authenticated manual lock verified & executed from trusted phone " + phoneOpt->name);
    SessionMonitor::Instance().LockWorkStation();

    return "{\"type\":\"STATE_SYNC\",\"success\":true,\"sessionState\":\"LOCKED\",\"message\":\"PC Locked successfully\"}";
}

std::optional<AuthChallenge> AuthCoordinator::GetActiveChallenge() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_activeChallenge;
}

std::optional<PairingSession> AuthCoordinator::InitiatePairingSession() {
    std::lock_guard<std::mutex> lock(m_mutex);
    PairingSession session;
    session.sessionId = CryptoEngine::GenerateUUID();
    session.confirmCode = m_cryptoEngine.GeneratePairingConfirmCode();
    session.createdAt = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    m_pendingPairing = session;
    return session;
}

} // namespace AnshuBio
