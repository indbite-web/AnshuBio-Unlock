#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <windows.h>
#include <wincrypt.h>

namespace AnshuBio {

class DPAPIVault {
public:
    static bool Encrypt(const std::string& plainText, std::string& cipherBase64);
    static bool Decrypt(const std::string& cipherBase64, std::string& plainText);

    static bool EncryptBytes(const std::vector<uint8_t>& plainBytes, std::vector<uint8_t>& cipherBytes);
    static bool DecryptBytes(const std::vector<uint8_t>& cipherBytes, std::vector<uint8_t>& plainBytes);

private:
    static std::string Base64Encode(const uint8_t* data, size_t length);
    static std::vector<uint8_t> Base64Decode(const std::string& base64Str);
};

} // namespace AnshuBio
