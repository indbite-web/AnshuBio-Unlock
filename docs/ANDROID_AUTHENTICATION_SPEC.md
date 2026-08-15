# AnshuBio Unlock — Android Native Authentication Architecture Specification

**Product**: AnshuBio Unlock  
**Publisher**: AnshuCore  
**Target Platform**: Android 10+ (API Level 29+)  
**Security Standard**: Android Native Biometric & Hardware KeyStore Architecture  

---

## 1. Core Security Principles & Constraints

1. **Native OS Biometrics Only**: The Android application MUST strictly use Android's official `androidx.biometric.BiometricPrompt` and `android.security.keystore.KeyGenParameterSpec` APIs.
2. **Zero Biometric Access / Ingestion**: AnshuBio Unlock does NOT create custom camera face scanners, does NOT access raw fingerprint hardware, does NOT capture sensor images, and does NOT store or transmit biometric templates.
3. **Zero Device Credential Access**: The app does NOT prompt for or record the device PIN/pattern/password in custom input fields. Device PIN fallback is handled entirely inside Android's secure system dialog (`setAllowedAuthenticators(BIOMETRIC_STRONG | DEVICE_CREDENTIAL)`).
4. **Cryptographic Proof Bridge**: Successful native biometric authentication unlocks the Android `KeyStore` private key to sign the PC's 32-byte cryptographic challenge. The PC receives **only** the cryptographic ECDSA signature proof.

---

## 2. Supported Authentication Modalities

Android `BiometricPrompt` prioritizes and supports all three tiers:

1. **Native Fingerprint** (`BIOMETRIC_STRONG` / Class 3)
2. **Native Face Recognition** (`BIOMETRIC_STRONG` / Class 3 where supported by hardware)
3. **Native Device PIN / Pattern / Password** (`DEVICE_CREDENTIAL` fallback)

```
+-------------------------------------------------------------+
|                 Android BiometricPrompt UI                  |
|                                                             |
|           "Unlock [Anshu-PC]"                               |
|   Touch fingerprint sensor, glance at camera,               |
|            or use device PIN/pattern                        |
+-------------------------------------------------------------+
                              |
                     [Native OS Auth OK]
                              |
                              v
+-------------------------------------------------------------+
|               Hardware Keystore Private Key                 |
|         (ECDSA P-256 Signature over CSPRNG Nonce)           |
+-------------------------------------------------------------+
                              |
                 signatureHex (Cryptographic Proof)
                              |
                              v
+-------------------------------------------------------------+
|               Windows PC (AnshuBio Unlock)                  |
|     Verifies signature -> Submits credential to LogonUI     |
+-------------------------------------------------------------+
```

---

## 3. Android Implementation Pattern (Kotlin / AndroidX)

### 3.1 Hardware-Backed KeyPair Generation
```kotlin
import android.security.keystore.KeyGenParameterSpec
import android.security.keystore.KeyProperties
import java.security.KeyPairGenerator
import java.security.KeyStore

object AnshuBioKeyStore {
    private const val KEY_ALIAS = "AnshuBioPhoneKey"
    private const val ANDROID_KEYSTORE = "AndroidKeyStore"

    fun generateKeyPair(): String {
        val keyPairGenerator = KeyPairGenerator.getInstance(
            KeyProperties.KEY_ALGORITHM_EC,
            ANDROID_KEYSTORE
        )

        val spec = KeyGenParameterSpec.Builder(
            KEY_ALIAS,
            KeyProperties.PURPOSE_SIGN or KeyProperties.PURPOSE_VERIFY
        )
            .setDigests(KeyProperties.DIGEST_SHA256)
            .setUserAuthenticationRequired(true) // Requires Biometric/PIN to sign
            .setUserAuthenticationParameters(
                0, // Auth valid only for single signature operation
                KeyProperties.AUTH_BIOMETRIC_STRONG or KeyProperties.AUTH_DEVICE_CREDENTIAL
            )
            .build()

        keyPairGenerator.initialize(spec)
        val keyPair = keyPairGenerator.generateKeyPair()
        return encodePublicKeyPem(keyPair.public)
    }
}
```

### 3.2 Native BiometricPrompt Execution
```kotlin
import androidx.biometric.BiometricManager.Authenticators.BIOMETRIC_STRONG
import androidx.biometric.BiometricManager.Authenticators.DEVICE_CREDENTIAL
import androidx.biometric.BiometricPrompt
import androidx.core.content.ContextCompat
import androidx.fragment.app.FragmentActivity
import java.security.Signature

class AnshuBioAuthManager(private val activity: FragmentActivity) {

    fun authenticateAndSignChallenge(
        pcId: String,
        challengeNonceHex: String,
        timestamp: Long,
        onSuccess: (signatureHex: String) -> Unit,
        onError: (errorCode: Int, errString: String) -> Unit
    ) {
        val executor = ContextCompat.getMainExecutor(activity)

        // Initialize Java Security Signature object backed by AndroidKeyStore
        val signature = Signature.getInstance("SHA256withECDSA")
        val privateKey = AnshuBioKeyStore.getPrivateKey()
        signature.initSign(privateKey)

        val cryptoObject = BiometricPrompt.CryptoObject(signature)

        val promptInfo = BiometricPrompt.PromptInfo.Builder()
            .setTitle("Unlock PC")
            .setSubtitle("Confirm biometric to authorize Windows unlock")
            .setAllowedAuthenticators(BIOMETRIC_STRONG or DEVICE_CREDENTIAL)
            .build()

        val biometricPrompt = BiometricPrompt(activity, executor,
            object : BiometricPrompt.AuthenticationCallback() {
                override fun onAuthenticationSucceeded(result: BiometricPrompt.AuthenticationResult) {
                    super.onAuthenticationSucceeded(result)
                    
                    // Hardware key is now unlocked for signing
                    val authedSignature = result.cryptoObject?.signature ?: signature
                    val payload = "AnshuBioAuth:$pcId:$challengeNonceHex:$timestamp".toByteArray(Charsets.UTF_8)
                    authedSignature.update(payload)
                    val signatureBytes = authedSignature.sign()
                    val signatureHex = signatureBytes.joinToString("") { "%02x".format(it) }

                    onSuccess(signatureHex)
                }

                override fun onAuthenticationError(errorCode: Int, errString: CharSequence) {
                    super.onAuthenticationError(errorCode, errString)
                    onError(errorCode, errString.toString())
                }
            })

        biometricPrompt.authenticate(promptInfo, cryptoObject)
    }
}
```

---

## 4. Security Guarantees

| Data Item | Handled By Android OS | Transmitted to PC | Stored by AnshuBio |
|-----------|------------------------|-------------------|---------------------|
| Fingerprint Template | Yes (Hardware TEE / Secure Enclave) | **NO** | **NO** |
| Face Biometric Data | Yes (Hardware TEE / Secure Enclave) | **NO** | **NO** |
| Device PIN / Pattern | Yes (KeyguardManager / LockPatternUtils) | **NO** | **NO** |
| ECDSA Public Key | Exported once at pairing | **Yes** (During Pairing) | **Yes** (DPAPI Keystore) |
| ECDSA Signature | Generated in TEE per challenge | **Yes** (Auth Response) | Single-use consumed |
