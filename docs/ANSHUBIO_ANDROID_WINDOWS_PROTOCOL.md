# AnshuBio Unlock — Android ↔ Windows Pairing & Authentication Protocol Specification

## Version: 1.0.0
**Target Architecture**: Native Windows (C++17 / Windows SDK) ↔ Native Android (Kotlin / Jetpack BiometricPrompt)  
**Security Model**: Mutual ECDSA (secp256r1) + SHA-256 + 32-Byte CSPRNG Nonce + Machine DPAPI + Replay Defense (<30s window)

---

## 1. QR Code Pairing Architecture

### Protocol Flow
```
┌─────────────────┐                                  ┌──────────────────┐
│   Windows PC    │                                  │   Android App    │
└────────┬────────┘                                  └────────┬─────────┘
         │                                                    │
         │  1. User clicks "Pair New Phone"                   │
         │  2. Generates SessionID, Nonce, PIN (6-digits)     │
         │  3. Renders QR Code                                │
         │                                                    │
         │             ◄── 4. Scans QR Code / Enters PIN ─────┤
         │                                                    │
         │  5. Phone connects (Wi-Fi TCP 42425 or BT RFCOMM)  │
         │     Sends: PAIRING_REQUEST (PhoneID, Public Key)   │
         │◄───────────────────────────────────────────────────┤
         │                                                    │
         │  6. Windows verifies: Trusted count < 2            │
         │     Windows displays: "Pair with Pixel 8 Pro?"     │
         │     Phone displays: "Pair with Anshu-PC?"          │
         │                                                    │
         │  7. User confirms on Windows PC [Confirm]          │
         │  8. User confirms on Android [Confirm]             │
         │◄────────────────── Mutual Handshake ──────────────►│
         │                                                    │
         │  9. Windows saves Phone Public Key to DPAPI Store  │
         │ 10. PC is now Protected. Status: PAIRED            │
         │                                                    │
```

---

## 2. Standard QR Payload Format

The QR Code encodes a UTF-8 JSON string structured as follows:

```json
{
  "protocol": "anshubio",
  "version": "1.0.0",
  "pcId": "PC-B7390F21",
  "pcName": "Anshu-PC",
  "sessionId": "4f18c8b1-9872-4d2b-bbcf-59e5124ec682",
  "nonce": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
  "ip": "192.168.1.50",
  "port": 42425,
  "btUuid": "00001101-0000-1000-8000-00805F9B34FB",
  "fingerprint": "A1B2C3D4E5F60718",
  "confirmCode": "583729",
  "expiresAt": 1723712405
}
```

### Field Definitions

| Field Name | Type | Description | Security Constraints |
| :--- | :--- | :--- | :--- |
| `protocol` | String | Protocol identifier (`"anshubio"`). | Fixed value. |
| `version` | String | Protocol semantic version (`"1.0.0"`). | Backward compatibility checking. |
| `pcId` | String | Unique hardware workstation UUID. | Public workstation ID. |
| `pcName` | String | Human-readable PC hostname / display name. | Displayed in Android pairing UI. |
| `sessionId` | String (UUID) | Ephemeral pairing session identifier. | Valid for single pairing attempt. |
| `nonce` | String (64-char Hex) | 32-byte CSPRNG random cryptographic challenge. | Single-use anti-replay nonce. |
| `ip` | String | Local IPv4 address for Wi-Fi discovery. | Valid on local LAN subnet. |
| `port` | Integer | TCP listener port (Default: `42425`). | Bound with non-overlapping IPC. |
| `btUuid` | String | Bluetooth SPP / RFCOMM Service GUID. | RFCOMM channel 1. |
| `fingerprint` | String (16-char Hex) | SHA-256 fingerprint of the PC's public key. | Mutual authenticity verification. |
| `confirmCode` | String (6-digit) | Mutual PIN confirmation code. | Matched on PC and Phone screen. |
| `expiresAt` | Integer (Unix Epoch) | Expiration timestamp in seconds. | Max 60 seconds TTL. |

> [!CAUTION]
> **Zero-Knowledge Security Rule**: The QR payload MUST NEVER contain Windows account passwords, PINs, phone passwords, private keys, or biometric data.

---

## 3. Mutual Confirmation Message Specifications

### 3.1 Pairing Request (Android → Windows)
```json
{
  "type": "PAIRING_REQUEST",
  "sessionId": "4f18c8b1-9872-4d2b-bbcf-59e5124ec682",
  "phoneId": "ANDROID-PH-98231",
  "phoneName": "Google Pixel 8 Pro",
  "phonePublicKeyPem": "-----BEGIN PUBLIC KEY-----\nMFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAE...\n-----END PUBLIC KEY-----",
  "confirmCode": "583729"
}
```

### 3.2 Pairing Response (Windows → Android)
```json
{
  "type": "PAIRING_RESPONSE",
  "sessionId": "4f18c8b1-9872-4d2b-bbcf-59e5124ec682",
  "pcId": "PC-B7390F21",
  "pcName": "Anshu-PC",
  "pcPublicKeyPem": "-----BEGIN PUBLIC KEY-----\nMFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAE...\n-----END PUBLIC KEY-----",
  "confirmCode": "583729"
}
```

### 3.3 Phone Confirmation (Android → Windows)
```json
{
  "type": "PAIRING_CONFIRM_PHONE",
  "sessionId": "4f18c8b1-9872-4d2b-bbcf-59e5124ec682",
  "phoneId": "ANDROID-PH-98231"
}
```

### 3.4 Pairing Completion (Windows → Android)
```json
{
  "type": "PAIRING_COMPLETE",
  "success": true,
  "message": "Pairing complete. PC is now protected."
}
```

---

## 4. Trusted Phone Registration Rules

1. **Maximum Device Limit**: Exactly **2 trusted phones** permitted per Windows workstation.
2. **Revocation Check**: If a device ID or public key is in the revoked blacklist, pairing is immediately rejected with `DEVICE_PREVIOUSLY_REVOKED`.
3. **Atomic Commit**: A device is saved to DPAPI vault ONLY after mutual confirmation (`pcConfirmed == true && phoneConfirmed == true`).
