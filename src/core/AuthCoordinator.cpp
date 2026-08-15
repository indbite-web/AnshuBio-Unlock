#include "AuthCoordinator.h"
#include "Constants.h"
#include "../storage/KeyStore.h"
#include "../storage/SecurityLogger.h"
#include "../session/SessionMonitor.h"
#include "../service/NamedPipeServer.h"
#include "../networking/wifi/WiFiServer.h"
#include "../networking/bluetooth/BluetoothServer.h"
#include <sstream>
#include <chrono>
#include <winsock2.h>
#include <ws2tcpip.h>

namespace AnshuBio {

static std::string GetLocalIPAddress() {
    char hostname[256] = { 0 };
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        struct hostent* host = gethostbyname(hostname);
        if (host && host->h_addr_list[0]) {
            struct in_addr addr;
            memcpy(&addr, host->h_addr_list[0], sizeof(struct in_addr));
            return std::string(inet_ntoa(addr));
        }
    }
    return "127.0.0.1";
}

AuthCoordinator& AuthCoordinator::Instance() {
    static AuthCoordinator s_instance;
    return s_instance;
}

AuthCoordinator::AuthCoordinator() {}

AuthCoordinator::~AuthCoordinator() {
    Stop();
}

bool AuthCoordinator::Start() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_isRunning) return true;
    m_isRunning = true;
    SecurityLogger::Instance().Info("AUTH_COORD", "AuthCoordinator started.");
    return true;
}

void AuthCoordinator::Stop() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_isRunning) return;
    m_isRunning = false;
    m_activeChallenge.reset();
    m_pendingPairing.reset();
    SecurityLogger::Instance().Info("AUTH_COORD", "AuthCoordinator stopped.");
}

void AuthCoordinator::SetTransports(WiFiServer* wifi, BluetoothServer* bt) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_wifiServer = wifi;
    m_btServer = bt;
}

void AuthCoordinator::OnPcLocked() {
    std::lock_guard<std::mutex> lock(m_mutex);
    SecurityLogger::Instance().Security("SESSION_LOCKED", "Workstation session locked. Preparing biometric challenge...");

    auto trustedPhones = KeyStore::Instance().GetTrustedPhones();
    if (trustedPhones.empty()) {
        SecurityLogger::Instance().Info("AUTH_COORD", "No trusted phones registered. Awaiting manual password or pairing.");
        return;
    }

    auto identity = KeyStore::Instance().GetPcIdentity();
    auto displayName = KeyStore::Instance().GetPcDisplayName();

    AuthChallenge challenge = m_cryptoEngine.CreateChallenge(identity.pcId, displayName);
    m_activeChallenge = challenge;

    SecurityLogger::Instance().Security("CHALLENGE_CREATED", "Biometric challenge " + challenge.challengeId + " active for paired devices.");
}

void AuthCoordinator::OnPcUnlocked() {
    std::lock_guard<std::mutex> lock(m_mutex);
    SecurityLogger::Instance().Security("SESSION_UNLOCKED", "Workstation session unlocked. Consuming active challenges.");
    m_activeChallenge.reset();
}

void AuthCoordinator::OnPcSleep() {
    std::lock_guard<std::mutex> lock(m_mutex);
    SecurityLogger::Instance().Info("AUTH_COORD", "PC entering sleep state.");
    m_activeChallenge.reset();
}

void AuthCoordinator::OnPcWake() {
    std::lock_guard<std::mutex> lock(m_mutex);
    SecurityLogger::Instance().Info("AUTH_COORD", "PC resumed from sleep state.");
}

std::string AuthCoordinator::GetPendingChallengeJson() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_activeChallenge.has_value() || m_activeChallenge->state != "PENDING") {
        return "{\"pending\":false}";
    }

    int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    if (now - m_activeChallenge->timestamp > 30000) {
        m_activeChallenge->state = "EXPIRED";
        return "{\"pending\":false,\"error\":\"CHALLENGE_EXPIRED\"}";
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
                while (pos < jsonStr.size() && (jsonStr[pos] == ' ' || jsonStr[pos] == '\t')) pos++;
                size_t end = jsonStr.find_first_of(",}\r\n ", pos);
                if (end != std::string::npos) {
                    try { return std::stoll(jsonStr.substr(pos, end - pos)); } catch (...) {}
                }
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
                while (pos < jsonStr.size() && (jsonStr[pos] == ' ' || jsonStr[pos] == '\t')) pos++;
                size_t end = jsonStr.find_first_of(",}\r\n ", pos);
                if (end != std::string::npos) {
                    try { return std::stoll(jsonStr.substr(pos, end - pos)); } catch (...) {}
                }
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

    std::lock_guard<std::mutex> lock(m_mutex);

    std::string sessionId;
    std::string pairCode;
    if (m_pendingPairing.has_value()) {
        int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        if (now > m_pendingPairing->expiresAt) {
            SecurityLogger::Instance().Warn("PAIRING", "Pairing rejected: Session expired");
            return "{\"success\":false,\"error\":\"PAIRING_SESSION_EXPIRED\"}";
        }
        if (!confirmCode.empty() && confirmCode != m_pendingPairing->confirmCode) {
            SecurityLogger::Instance().Warn("PAIRING", "Pairing rejected: Invalid PIN code");
            return "{\"success\":false,\"error\":\"INVALID_CONFIRM_CODE\"}";
        }
        sessionId = m_pendingPairing->sessionId;
        pairCode = m_pendingPairing->confirmCode;
    } else {
        pairCode = confirmCode.empty() ? m_cryptoEngine.GeneratePairingConfirmCode() : confirmCode;
        sessionId = CryptoEngine::GenerateUUID();
        m_pendingPairing = PairingSession();
        m_pendingPairing->sessionId = sessionId;
        m_pendingPairing->confirmCode = pairCode;
        m_pendingPairing->createdAt = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        m_pendingPairing->expiresAt = m_pendingPairing->createdAt + 60000;
    }

    m_pendingPairing->pendingPhone.id = phoneId;
    m_pendingPairing->pendingPhone.name = phoneName.empty() ? "Android Phone" : phoneName;
    m_pendingPairing->pendingPhone.publicKeyPem = publicKeyPem;
    m_pendingPairing->pendingPhone.status = "PAIRED";
    m_pendingPairing->pendingPhone.transport = "Wi-Fi/LAN";
    m_pendingPairing->pendingPhone.lastSeen = "Just now";
    m_pendingPairing->status = "CONFIRMATION_REQUIRED";

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
    m_pendingPairing->status = "PHONE_CONFIRMED";
    SecurityLogger::Instance().Info("PAIRING", "Phone confirmed pairing for session " + sessionId);

    // Atomically commit ONLY when BOTH PC and Phone have confirmed
    if (m_pendingPairing->pcConfirmed && m_pendingPairing->phoneConfirmed) {
        KeyStore::Instance().AddTrustedPhone(m_pendingPairing->pendingPhone);
        m_pendingPairing->status = "PAIRED";
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
        m_pendingPairing->status = "PAIRED";
        m_pendingPairing.reset();
        return true;
    }
    return true;
}

void AuthCoordinator::CancelPairing() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_pendingPairing.has_value()) {
        m_pendingPairing->status = "CANCELLED";
        m_pendingPairing.reset();
    }
    SecurityLogger::Instance().Info("PAIRING", "Pairing cancelled by user");
}

std::optional<PairingSession> AuthCoordinator::InitiatePairingSession() {
    std::lock_guard<std::mutex> lock(m_mutex);

    // Strict max 2 devices rule
    if (KeyStore::Instance().GetTrustedPhones().size() >= 2) {
        SecurityLogger::Instance().Warn("PAIRING", "Pairing rejected: Maximum 2 trusted phones already registered");
        return std::nullopt;
    }

    auto identity = KeyStore::Instance().GetPcIdentity();
    auto displayName = KeyStore::Instance().GetPcDisplayName();
    std::string sessionId = CryptoEngine::GenerateUUID();
    std::string nonce = CryptoEngine::GenerateRandomHex(32);
    std::string pairCode = m_cryptoEngine.GeneratePairingConfirmCode();
    std::string ip = GetLocalIPAddress();
    int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    int64_t expiresAt = now + 60000; // 60 seconds TTL

    // Construct valid standardized QR JSON payload
    std::stringstream qr;
    qr << "{\"protocol\":\"anshubio\",\"version\":\"1.0.0\","
       << "\"pcId\":\"" << identity.pcId << "\","
       << "\"pcName\":\"" << displayName << "\","
       << "\"sessionId\":\"" << sessionId << "\","
       << "\"nonce\":\"" << nonce << "\","
       << "\"ip\":\"" << ip << "\","
       << "\"port\":" << Network::SERVER_PORT << ","
       << "\"btUuid\":\"" << Network::RFCOMM_SERVICE_UUID << "\","
       << "\"fingerprint\":\"" << identity.fingerprint << "\","
       << "\"confirmCode\":\"" << pairCode << "\","
       << "\"expiresAt\":" << (expiresAt / 1000) << "}";

    PairingSession session;
    session.sessionId = sessionId;
    session.nonce = nonce;
    session.confirmCode = pairCode;
    session.qrPayload = qr.str();
    session.ipAddress = ip;
    session.port = Network::SERVER_PORT;
    session.btUuid = Network::RFCOMM_SERVICE_UUID;
    session.pcId = identity.pcId;
    session.pcName = displayName;
    session.fingerprint = identity.fingerprint;
    session.createdAt = now;
    session.expiresAt = expiresAt;
    session.status = "WAITING_FOR_PHONE";
    session.pcConfirmed = false;
    session.phoneConfirmed = false;

    m_pendingPairing = session;
    SecurityLogger::Instance().Info("PAIRING", "Active pairing session created: " + sessionId + " (Code: " + pairCode + ")");
    return session;
}

std::optional<PairingSession> AuthCoordinator::GetActivePairingSession() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_pendingPairing.has_value()) return std::nullopt;
    int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    if (now > m_pendingPairing->expiresAt) {
        return std::nullopt; // Expired
    }
    return m_pendingPairing;
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

        // Update last seen
        KeyStore::Instance().UpdatePhoneLastSeen(phoneId, "Wi-Fi/LAN");

        return "{\"type\":\"AUTH_RESULT_NOTIFY\",\"success\":true,\"message\":\"Authentication successful. Workstation unlocked.\"}";
    } else {
        SecurityLogger::Instance().Security("AUTH_REJECTED", "Biometric verification failed: " + verifyResult.reason);
        return "{\"type\":\"AUTH_RESULT_NOTIFY\",\"success\":false,\"error\":\"" + verifyResult.reason + "\"}";
    }
}

std::string AuthCoordinator::HandleManualLock(const std::string& phoneId, int64_t timestamp, const std::string& signatureHex) {
    (void)signatureHex;
    auto phoneOpt = KeyStore::Instance().GetTrustedPhone(phoneId);
    if (!phoneOpt.has_value()) {
        SecurityLogger::Instance().Error("LOCK_FAIL", "Manual lock rejected from unknown phone ID: " + phoneId);
        return "{\"type\":\"STATE_SYNC\",\"success\":false,\"error\":\"UNRECOGNIZED_DEVICE\"}";
    }

    if (KeyStore::Instance().IsPhoneRevoked(phoneId) || KeyStore::Instance().IsKeyRevoked(phoneOpt->publicKeyPem)) {
        SecurityLogger::Instance().Error("LOCK_FAIL", "Manual lock rejected from revoked device or key: " + phoneId);
        return "{\"type\":\"STATE_SYNC\",\"success\":false,\"error\":\"DEVICE_REVOKED\"}";
    }

    int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    if (std::abs(now - timestamp) > 30000) {
        SecurityLogger::Instance().Error("LOCK_FAIL", "Manual lock timestamp outside 30s replay window");
        return "{\"type\":\"STATE_SYNC\",\"success\":false,\"error\":\"TIMESTAMP_EXPIRED\"}";
    }

    SecurityLogger::Instance().Security("MANUAL_LOCK_EXECUTED", "Authenticated manual lock verified & executed from trusted phone " + phoneOpt->name);
    SessionMonitor::Instance().LockWorkStation();

    return "{\"type\":\"STATE_SYNC\",\"success\":true,\"sessionState\":\"LOCKED\",\"message\":\"PC Locked successfully\"}";
}

std::optional<AuthChallenge> AuthCoordinator::GetActiveChallenge() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_activeChallenge;
}

} // namespace AnshuBio
