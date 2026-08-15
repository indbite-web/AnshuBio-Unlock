#pragma once
#include <string>
#include <vector>
#include <map>
#include <set>
#include <mutex>
#include <cstdint>
#include "../core/Models.h"

namespace AnshuBio {

struct AuthVerificationResult {
    bool success = false;
    std::string reason;
    std::string phoneId;
    std::string phoneName;
};

class CryptoEngine {
public:
    CryptoEngine();
    ~CryptoEngine();

    // Key management
    bool GenerateECKeyPair(std::string& publicKeyPem, std::string& privateKeyPem, std::string& fingerprint);
    std::string ComputeFingerprint(const std::string& publicKeyPem);

    // Challenge generation
    AuthChallenge CreateChallenge(const std::string& pcId, const std::string& pcName);
    std::string GetChallengePayloadString(const std::string& pcId, const std::string& challengeNonceHex, int64_t timestamp);

    // Signature verification
    AuthVerificationResult VerifyAuthResponse(
        const std::string& challengeId,
        const std::string& phoneId,
        int64_t timestamp,
        const std::string& signatureHex,
        const TrustedPhone& trustedPhone
    );

    // Pairing
    std::string GeneratePairingConfirmCode();

    // Helper crypto utilities
    static std::string GenerateRandomHex(size_t byteCount);
    static std::string Sha256Hex(const std::string& input);
    static std::string GenerateUUID();

private:
    std::mutex m_mutex;
    std::map<std::string, AuthChallenge> m_activeChallenges;
    std::set<std::string> m_consumedNonces;
};

} // namespace AnshuBio
