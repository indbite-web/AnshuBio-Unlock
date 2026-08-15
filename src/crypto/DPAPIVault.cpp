#include "DPAPIVault.h"
#include <stdexcept>
#include <cstdint>

namespace AnshuBio {

bool DPAPIVault::EncryptBytes(const std::vector<uint8_t>& plainBytes, std::vector<uint8_t>& cipherBytes) {
    if (plainBytes.empty()) {
        cipherBytes.clear();
        return true;
    }

    DATA_BLOB dataIn;
    dataIn.pbData = const_cast<BYTE*>(reinterpret_cast<const BYTE*>(plainBytes.data()));
    dataIn.cbData = static_cast<DWORD>(plainBytes.size());

    DATA_BLOB dataOut;
    ZeroMemory(&dataOut, sizeof(dataOut));

    // Machine-level DPAPI protection
    if (CryptProtectData(&dataIn, L"AnshuBioDPAPIVault", nullptr, nullptr, nullptr, CRYPTPROTECT_LOCAL_MACHINE, &dataOut)) {
        cipherBytes.assign(dataOut.pbData, dataOut.pbData + dataOut.cbData);
        LocalFree(dataOut.pbData);
        return true;
    }

    // Fallback to current user DPAPI protection
    if (CryptProtectData(&dataIn, L"AnshuBioDPAPIVault", nullptr, nullptr, nullptr, 0, &dataOut)) {
        cipherBytes.assign(dataOut.pbData, dataOut.pbData + dataOut.cbData);
        LocalFree(dataOut.pbData);
        return true;
    }

    return false;
}

bool DPAPIVault::DecryptBytes(const std::vector<uint8_t>& cipherBytes, std::vector<uint8_t>& plainBytes) {
    if (cipherBytes.empty()) {
        plainBytes.clear();
        return true;
    }

    DATA_BLOB dataIn;
    dataIn.pbData = const_cast<BYTE*>(reinterpret_cast<const BYTE*>(cipherBytes.data()));
    dataIn.cbData = static_cast<DWORD>(cipherBytes.size());

    DATA_BLOB dataOut;
    ZeroMemory(&dataOut, sizeof(dataOut));

    if (CryptUnprotectData(&dataIn, nullptr, nullptr, nullptr, nullptr, 0, &dataOut)) {
        plainBytes.assign(dataOut.pbData, dataOut.pbData + dataOut.cbData);
        SecureZeroMemory(dataOut.pbData, dataOut.cbData);
        LocalFree(dataOut.pbData);
        return true;
    }

    return false;
}

bool DPAPIVault::Encrypt(const std::string& plainText, std::string& cipherBase64) {
    std::vector<uint8_t> plainBytes(plainText.begin(), plainText.end());
    std::vector<uint8_t> cipherBytes;
    if (!EncryptBytes(plainBytes, cipherBytes)) {
        return false;
    }
    cipherBase64 = Base64Encode(cipherBytes.data(), cipherBytes.size());
    return true;
}

bool DPAPIVault::Decrypt(const std::string& cipherBase64, std::string& plainText) {
    std::vector<uint8_t> cipherBytes = Base64Decode(cipherBase64);
    std::vector<uint8_t> plainBytes;
    if (!DecryptBytes(cipherBytes, plainBytes)) {
        return false;
    }
    plainText.assign(plainBytes.begin(), plainBytes.end());
    SecureZeroMemory(plainBytes.data(), plainBytes.size());
    return true;
}

std::string DPAPIVault::Base64Encode(const uint8_t* data, size_t length) {
    if (!data || length == 0) return "";
    DWORD len = 0;
    const BYTE* pbBinary = reinterpret_cast<const BYTE*>(data);
    CryptBinaryToStringA(pbBinary, static_cast<DWORD>(length), CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, nullptr, &len);
    std::string result(len, '\0');
    if (CryptBinaryToStringA(pbBinary, static_cast<DWORD>(length), CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, &result[0], &len)) {
        if (!result.empty() && result.back() == '\0') result.pop_back();
        return result;
    }
    return "";
}

std::vector<uint8_t> DPAPIVault::Base64Decode(const std::string& base64Str) {
    if (base64Str.empty()) return {};
    DWORD len = 0;
    CryptStringToBinaryA(base64Str.c_str(), static_cast<DWORD>(base64Str.length()), CRYPT_STRING_BASE64, nullptr, &len, nullptr, nullptr);
    std::vector<uint8_t> result(len);
    if (CryptStringToBinaryA(base64Str.c_str(), static_cast<DWORD>(base64Str.length()), CRYPT_STRING_BASE64, reinterpret_cast<BYTE*>(result.data()), &len, nullptr, nullptr)) {
        result.resize(len);
        return result;
    }
    return {};
}

} // namespace AnshuBio
