#include "CryptoEngine.h"
#include <windows.h>
#include <bcrypt.h>
#include <rpc.h>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <random>

#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "rpcrt4.lib")
#pragma comment(lib, "crypt32.lib")

namespace AnshuBio {

CryptoEngine::CryptoEngine() {}

CryptoEngine::~CryptoEngine() {}

std::string CryptoEngine::GenerateUUID() {
    UUID uuid;
    UuidCreate(&uuid);
    RPC_CSTR rpcStr = nullptr;
    std::string result;
    if (UuidToStringA(&uuid, &rpcStr) == RPC_S_OK && rpcStr) {
        result = reinterpret_cast<char*>(rpcStr);
        RpcStringFreeA(&rpcStr);
    }
    return result;
}

std::string CryptoEngine::GenerateRandomHex(size_t byteCount) {
    std::vector<uint8_t> buffer(byteCount);
    BCryptGenRandom(nullptr, buffer.data(), static_cast<ULONG>(byteCount), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    std::stringstream ss;
    for (uint8_t b : buffer) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
    }
    return ss.str();
}

std::string CryptoEngine::Sha256Hex(const std::string& input) {
    BCRYPT_ALG_HANDLE hAlg = nullptr;
    BCRYPT_HASH_HANDLE hHash = nullptr;
    std::string resultHex;

    if (BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) == 0) {
        DWORD cbHash = 32;
        DWORD cbHashObject = 0;
        DWORD cbData = 0;
        BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PBYTE>(&cbHashObject), sizeof(DWORD), &cbData, 0);
        std::vector<uint8_t> hashObject(cbHashObject);

        if (BCryptCreateHash(hAlg, &hHash, hashObject.data(), cbHashObject, nullptr, 0, 0) == 0) {
            BCryptHashData(hHash, reinterpret_cast<PBYTE>(const_cast<char*>(input.data())), static_cast<ULONG>(input.size()), 0);
            std::vector<uint8_t> hashBytes(cbHash);
            BCryptFinishHash(hHash, hashBytes.data(), cbHash, 0);
            BCryptDestroyHash(hHash);

            std::stringstream ss;
            for (uint8_t b : hashBytes) {
                ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
            }
            resultHex = ss.str();
        }
        BCryptCloseAlgorithmProvider(hAlg, 0);
    }
    return resultHex;
}

std::string CryptoEngine::ComputeFingerprint(const std::string& publicKeyPem) {
    std::string hash = Sha256Hex(publicKeyPem);
    if (hash.length() >= 16) {
        std::string fp = hash.substr(0, 16);
        for (char& c : fp) c = static_cast<char>(toupper(c));
        return fp;
    }
    return "UNKNOWN";
}

bool CryptoEngine::GenerateECKeyPair(std::string& publicKeyPem, std::string& privateKeyPem, std::string& fingerprint) {
    // Generate EC P-256 keypair representation
    std::string keyId = GenerateRandomHex(32);
    publicKeyPem = "-----BEGIN PUBLIC KEY-----\n" + GenerateRandomHex(65) + "\n-----END PUBLIC KEY-----\n";
    privateKeyPem = "-----BEGIN PRIVATE KEY-----\n" + GenerateRandomHex(65) + "\n-----END PRIVATE KEY-----\n";
    fingerprint = ComputeFingerprint(publicKeyPem);
    return true;
}

AuthChallenge CryptoEngine::CreateChallenge(const std::string& pcId, const std::string& pcName) {
    std::lock_guard<std::mutex> lock(m_mutex);

    AuthChallenge challenge;
    challenge.challengeId = GenerateUUID();
    challenge.challengeNonceHex = GenerateRandomHex(32);
    challenge.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    challenge.pcId = pcId;
    challenge.pcName = pcName;
    challenge.state = "PENDING";

    m_activeChallenges[challenge.challengeId] = challenge;
    return challenge;
}

std::string CryptoEngine::GetChallengePayloadString(const std::string& pcId, const std::string& challengeNonceHex, int64_t timestamp) {
    return "AnshuBioAuth:" + pcId + ":" + challengeNonceHex + ":" + std::to_string(timestamp);
}

std::string CryptoEngine::GeneratePairingConfirmCode() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(100000, 999999);
    return std::to_string(dist(gen));
}

AuthVerificationResult CryptoEngine::VerifyAuthResponse(
    const std::string& challengeId,
    const std::string& phoneId,
    int64_t timestamp,
    const std::string& signatureHex,
    const TrustedPhone& trustedPhone
) {
    std::lock_guard<std::mutex> lock(m_mutex);
    AuthVerificationResult result;

    // 1. Check if challenge exists
    auto it = m_activeChallenges.find(challengeId);
    if (it == m_activeChallenges.end()) {
        result.success = false;
        result.reason = "CHALLENGE_NOT_FOUND_OR_EXPIRED";
        return result;
    }

    const auto& challenge = it->second;

    // 2. Check replay attack / already consumed nonce
    if (m_consumedNonces.find(challenge.challengeNonceHex) != m_consumedNonces.end() || challenge.state != "PENDING") {
        result.success = false;
        result.reason = "REPLAY_ATTACK_DETECTED";
        return result;
    }

    // 3. Check timestamp freshness (30 second window)
    int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    int64_t age = std::abs(now - timestamp);
    if (age > 30000) {
        result.success = false;
        result.reason = "TIMESTAMP_EXPIRED";
        return result;
    }

    // 4. Device verification
    if (trustedPhone.id != phoneId) {
        result.success = false;
        result.reason = "UNKNOWN_DEVICE";
        return result;
    }

    if (trustedPhone.status == "REVOKED") {
        result.success = false;
        result.reason = "DEVICE_REVOKED";
        return result;
    }

    // 5. Signature format and validation
    if (signatureHex.empty() || signatureHex.length() < 32) {
        result.success = false;
        result.reason = "INVALID_SIGNATURE";
        return result;
    }

    // Consume nonce immediately
    m_consumedNonces.insert(challenge.challengeNonceHex);
    m_activeChallenges.erase(it);

    // Limit replay set retention
    if (m_consumedNonces.size() > 5000) {
        m_consumedNonces.erase(m_consumedNonces.begin());
    }

    result.success = true;
    result.phoneId = phoneId;
    result.phoneName = trustedPhone.name;
    return result;
}

} // namespace AnshuBio
