#pragma once
#include <string>

namespace AnshuBio {

namespace Product {
    constexpr const char* NAME = "AnshuBio Unlock";
    constexpr const char* PUBLISHER = "AnshuCore";
    constexpr const char* APP_ID = "com.anshucore.bio";
    constexpr const char* VERSION = "1.0.0";
    constexpr const char* EXE_NAME = "AnshuBioUnlock.exe";
    constexpr const char* SERVICE_NAME = "AnshuBioUnlockService";
    constexpr const char* DISPLAY_NAME = "AnshuBio Unlock Background Service";
    constexpr int MAX_TRUSTED_PHONES = 2;
}

namespace Network {
    constexpr int DISCOVERY_PORT = 42424;
    constexpr int SERVER_PORT = 42425;
    constexpr int BROADCAST_INTERVAL_MS = 3000;
    constexpr int HEARTBEAT_INTERVAL_MS = 5000;
    constexpr int CHALLENGE_TIMEOUT_MS = 30000;
    constexpr const wchar_t* PIPE_NAME = L"\\\\.\\pipe\\AnshuBioUnlockAuthPipe";
    constexpr const char* BLE_SERVICE_UUID = "0000ab10-0000-1000-8000-00805f9b34fb";
    constexpr const char* RFCOMM_SERVICE_UUID = "{1b7e8251-2877-41c3-b46e-cf057c562023}";
}

namespace CryptoConfig {
    constexpr const char* ALGORITHM = "ECDSA_P256_SHA256";
    constexpr const char* CURVE = "prime256v1";
    constexpr int CHALLENGE_BYTES = 32;
    constexpr int NONCE_WINDOW_SEC = 30;
}

enum class SessionState {
    RunningUnlocked,
    Locked,
    AuthPending,
    AuthSucceeded,
    AuthFailed,
    Sleep,
    Disabled
};

inline const char* SessionStateToString(SessionState state) {
    switch (state) {
        case SessionState::RunningUnlocked: return "RUNNING_UNLOCKED";
        case SessionState::Locked: return "LOCKED";
        case SessionState::AuthPending: return "AUTH_PENDING";
        case SessionState::AuthSucceeded: return "AUTH_SUCCEEDED";
        case SessionState::AuthFailed: return "AUTH_FAILED";
        case SessionState::Sleep: return "SLEEP";
        case SessionState::Disabled: return "DISABLED";
        default: return "UNKNOWN";
    }
}

namespace MsgTypes {
    constexpr const char* DISCOVERY_BEACON = "DISCOVERY_BEACON";
    constexpr const char* DISCOVERY_QUERY = "DISCOVERY_QUERY";
    constexpr const char* DISCOVERY_RESPONSE = "DISCOVERY_RESPONSE";
    
    constexpr const char* PAIRING_REQUEST = "PAIRING_REQUEST";
    constexpr const char* PAIRING_CONFIRM_PC = "PAIRING_CONFIRM_PC";
    constexpr const char* PAIRING_CONFIRM_PHONE = "PAIRING_CONFIRM_PHONE";
    constexpr const char* PAIRING_COMPLETE = "PAIRING_COMPLETE";
    constexpr const char* PAIRING_CANCEL = "PAIRING_CANCEL";
    
    constexpr const char* AUTH_CHALLENGE_REQ = "AUTH_CHALLENGE_REQ";
    constexpr const char* AUTH_CHALLENGE_RESP = "AUTH_CHALLENGE_RESP";
    constexpr const char* AUTH_RESULT_NOTIFY = "AUTH_RESULT_NOTIFY";
    
    constexpr const char* STATE_SYNC = "STATE_SYNC";
    constexpr const char* MANUAL_LOCK_CMD = "MANUAL_LOCK_CMD";
    constexpr const char* DEVICE_REVOKE_NOTIFY = "DEVICE_REVOKE_NOTIFY";
    constexpr const char* PING = "PING";
    constexpr const char* PONG = "PONG";
}

} // namespace AnshuBio
