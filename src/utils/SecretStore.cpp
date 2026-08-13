#include "SecretStore.hpp"

#include <cstring>
#include <vector>

#ifdef _WIN32
// NOMINMAX before the <windows.h> chain — see CLAUDE.md; wincrypt.h pulls in dpapi.h,
// which declares CryptProtectData/CryptUnprotectData.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <wincrypt.h>
#elif defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#include <Security/Security.h>
#else
#include <libsecret/secret.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <sys/stat.h>
#include <fstream>
#include <iostream>
#include "utils/Platform.hpp"
#endif

namespace timetracker::utils {

namespace {

constexpr char kBase64Alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string base64Encode(const unsigned char* data, size_t size) {
    std::string out;
    out.reserve(((size + 2) / 3) * 4);

    for (size_t i = 0; i < size; i += 3) {
        const unsigned int b0 = data[i];
        const unsigned int b1 = (i + 1 < size) ? data[i + 1] : 0;
        const unsigned int b2 = (i + 2 < size) ? data[i + 2] : 0;
        const unsigned int triple = (b0 << 16) | (b1 << 8) | b2;

        out.push_back(kBase64Alphabet[(triple >> 18) & 0x3F]);
        out.push_back(kBase64Alphabet[(triple >> 12) & 0x3F]);
        out.push_back((i + 1 < size) ? kBase64Alphabet[(triple >> 6) & 0x3F] : '=');
        out.push_back((i + 2 < size) ? kBase64Alphabet[triple & 0x3F] : '=');
    }

    return out;
}

std::vector<unsigned char> base64Decode(const std::string& encoded) {
    signed char table[256];
    std::memset(table, -1, sizeof(table));
    for (int i = 0; i < 64; ++i) {
        table[static_cast<unsigned char>(kBase64Alphabet[i])] = static_cast<signed char>(i);
    }

    std::vector<unsigned char> out;
    out.reserve((encoded.size() / 4) * 3);

    unsigned int buffer = 0;
    int bits = 0;
    for (char c : encoded) {
        const signed char value = table[static_cast<unsigned char>(c)];
        if (value < 0) {
            continue;  // padding and whitespace
        }
        buffer = (buffer << 6) | static_cast<unsigned int>(value);
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out.push_back(static_cast<unsigned char>((buffer >> bits) & 0xFF));
        }
    }

    return out;
}

bool hasPrefix(const std::string& value, const std::string& prefix) {
    return value.size() >= prefix.size() && value.compare(0, prefix.size(), prefix) == 0;
}

#if !defined(_WIN32) && !defined(__APPLE__)

// --- Linux fallback: AES-256-GCM with a key file next to data.db ---------------
//
// Used only when the Secret Service is unavailable (headless sessions, CI). The key
// sits beside the ciphertext, so this protects against casual reads of data.db and
// nothing more; libsecret is always tried first.

constexpr size_t kKeySize = 32;
constexpr size_t kIvSize = 12;
constexpr size_t kTagSize = 16;

std::vector<unsigned char> loadOrCreateKeyFile() {
    const fs::path keyPath = Platform::getDataDirectory() / "secret.key";

    std::ifstream in(keyPath.string(), std::ios::binary);
    if (in) {
        std::vector<unsigned char> key(kKeySize);
        in.read(reinterpret_cast<char*>(key.data()), static_cast<std::streamsize>(kKeySize));
        if (in.gcount() == static_cast<std::streamsize>(kKeySize)) {
            return key;
        }
    }

    std::vector<unsigned char> key(kKeySize);
    if (RAND_bytes(key.data(), static_cast<int>(kKeySize)) != 1) {
        return {};
    }

    Platform::ensureDataDirectoryExists();
    std::ofstream out(keyPath.string(), std::ios::binary | std::ios::trunc);
    if (!out) {
        return {};
    }
    out.write(reinterpret_cast<const char*>(key.data()), static_cast<std::streamsize>(kKeySize));
    out.close();
    ::chmod(keyPath.c_str(), S_IRUSR | S_IWUSR);

    return key;
}

std::optional<std::string> encryptWithKeyFile(const std::string& key, const std::string& secret) {
    const std::vector<unsigned char> keyBytes = loadOrCreateKeyFile();
    if (keyBytes.size() != kKeySize) {
        return std::nullopt;
    }

    std::vector<unsigned char> iv(kIvSize);
    if (RAND_bytes(iv.data(), static_cast<int>(kIvSize)) != 1) {
        return std::nullopt;
    }

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return std::nullopt;
    }

    std::vector<unsigned char> ciphertext(secret.size());
    std::vector<unsigned char> tag(kTagSize);
    int length = 0;
    int ciphertextLength = 0;
    bool ok = EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, keyBytes.data(), iv.data()) == 1;

    // The settings key is authenticated but not encrypted, so a ciphertext cannot be
    // moved from one settings row to another.
    if (ok) {
        ok = EVP_EncryptUpdate(ctx, nullptr, &length,
                               reinterpret_cast<const unsigned char*>(key.data()),
                               static_cast<int>(key.size())) == 1;
    }
    if (ok && !secret.empty()) {
        ok = EVP_EncryptUpdate(ctx, ciphertext.data(), &length,
                               reinterpret_cast<const unsigned char*>(secret.data()),
                               static_cast<int>(secret.size())) == 1;
        ciphertextLength = length;
    }
    if (ok) {
        ok = EVP_EncryptFinal_ex(ctx, ciphertext.data() + ciphertextLength, &length) == 1;
        ciphertextLength += length;
    }
    if (ok) {
        ok = EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, static_cast<int>(kTagSize), tag.data()) == 1;
    }
    EVP_CIPHER_CTX_free(ctx);

    if (!ok) {
        return std::nullopt;
    }

    std::vector<unsigned char> blob;
    blob.reserve(kIvSize + kTagSize + static_cast<size_t>(ciphertextLength));
    blob.insert(blob.end(), iv.begin(), iv.end());
    blob.insert(blob.end(), tag.begin(), tag.end());
    blob.insert(blob.end(), ciphertext.begin(), ciphertext.begin() + ciphertextLength);

    return std::string(SecretStore::kEncryptedPrefix) + base64Encode(blob.data(), blob.size());
}

std::string decryptWithKeyFile(const std::string& key, const std::string& payload) {
    const std::vector<unsigned char> keyBytes = loadOrCreateKeyFile();
    if (keyBytes.size() != kKeySize) {
        return "";
    }

    const std::vector<unsigned char> blob = base64Decode(payload);
    if (blob.size() < kIvSize + kTagSize) {
        return "";
    }

    const unsigned char* iv = blob.data();
    const unsigned char* tag = blob.data() + kIvSize;
    const unsigned char* ciphertext = blob.data() + kIvSize + kTagSize;
    const size_t ciphertextSize = blob.size() - kIvSize - kTagSize;

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return "";
    }

    std::vector<unsigned char> plaintext(ciphertextSize + 1);
    int length = 0;
    int plaintextLength = 0;
    bool ok = EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, keyBytes.data(), iv) == 1;

    if (ok) {
        ok = EVP_DecryptUpdate(ctx, nullptr, &length,
                               reinterpret_cast<const unsigned char*>(key.data()),
                               static_cast<int>(key.size())) == 1;
    }
    if (ok && ciphertextSize > 0) {
        ok = EVP_DecryptUpdate(ctx, plaintext.data(), &length, ciphertext,
                               static_cast<int>(ciphertextSize)) == 1;
        plaintextLength = length;
    }
    if (ok) {
        ok = EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, static_cast<int>(kTagSize),
                                 const_cast<unsigned char*>(tag)) == 1;
    }
    if (ok) {
        // Fails when the tag does not verify, which is the point of using GCM here.
        ok = EVP_DecryptFinal_ex(ctx, plaintext.data() + plaintextLength, &length) == 1;
        plaintextLength += length;
    }
    EVP_CIPHER_CTX_free(ctx);

    if (!ok) {
        return "";
    }

    return std::string(reinterpret_cast<const char*>(plaintext.data()),
                       static_cast<size_t>(plaintextLength));
}

const SecretSchema* tempusSchema() {
    static const SecretSchema schema = {
        "com.tempus.Secret",
        SECRET_SCHEMA_NONE,
        {
            {"key", SECRET_SCHEMA_ATTRIBUTE_STRING},
            {nullptr, SECRET_SCHEMA_ATTRIBUTE_STRING},
        },
    };
    return &schema;
}

#endif  // Linux

#ifdef __APPLE__

CFMutableDictionaryRef makeKeychainQuery(const std::string& key) {
    CFMutableDictionaryRef query = CFDictionaryCreateMutable(
        kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    CFDictionarySetValue(query, kSecClass, kSecClassGenericPassword);
    CFDictionarySetValue(query, kSecAttrService, CFSTR("tempus"));

    CFStringRef account =
        CFStringCreateWithCString(kCFAllocatorDefault, key.c_str(), kCFStringEncodingUTF8);
    CFDictionarySetValue(query, kSecAttrAccount, account);
    CFRelease(account);

    return query;
}

#endif  // __APPLE__

}  // namespace

bool SecretStore::isMarker(const std::string& marker) {
    return marker == kKeyringMarker || hasPrefix(marker, kEncryptedPrefix);
}

// ---------------------------------------------------------------------------
#ifdef _WIN32

std::optional<std::string> SecretStore::store(const std::string& key, const std::string& secret) {
    DATA_BLOB input{static_cast<DWORD>(secret.size()),
                    reinterpret_cast<BYTE*>(const_cast<char*>(secret.data()))};
    DATA_BLOB entropy{static_cast<DWORD>(key.size()),
                      reinterpret_cast<BYTE*>(const_cast<char*>(key.data()))};
    DATA_BLOB output{};

    if (!CryptProtectData(&input, L"tempus", &entropy, nullptr, nullptr,
                          CRYPTPROTECT_UI_FORBIDDEN, &output)) {
        return std::nullopt;
    }

    std::string encoded = base64Encode(output.pbData, output.cbData);
    LocalFree(output.pbData);

    return std::string(kEncryptedPrefix) + encoded;
}

std::string SecretStore::retrieve(const std::string& key, const std::string& marker) {
    if (!hasPrefix(marker, kEncryptedPrefix)) {
        // Legacy plaintext, or a keyring marker written on another platform.
        return marker == kKeyringMarker ? "" : marker;
    }

    std::vector<unsigned char> blob = base64Decode(marker.substr(std::strlen(kEncryptedPrefix)));
    if (blob.empty()) {
        return "";
    }

    DATA_BLOB input{static_cast<DWORD>(blob.size()), blob.data()};
    DATA_BLOB entropy{static_cast<DWORD>(key.size()),
                      reinterpret_cast<BYTE*>(const_cast<char*>(key.data()))};
    DATA_BLOB output{};

    if (!CryptUnprotectData(&input, nullptr, &entropy, nullptr, nullptr,
                            CRYPTPROTECT_UI_FORBIDDEN, &output)) {
        return "";
    }

    std::string secret(reinterpret_cast<char*>(output.pbData), output.cbData);
    SecureZeroMemory(output.pbData, output.cbData);
    LocalFree(output.pbData);

    return secret;
}

void SecretStore::erase(const std::string&) {
    // DPAPI keeps nothing outside the marker itself; clearing the settings row is enough.
}

// ---------------------------------------------------------------------------
#elif defined(__APPLE__)

std::optional<std::string> SecretStore::store(const std::string& key, const std::string& secret) {
    CFMutableDictionaryRef query = makeKeychainQuery(key);
    SecItemDelete(query);  // SecItemAdd returns errSecDuplicateItem otherwise

    CFDataRef data = CFDataCreate(kCFAllocatorDefault,
                                  reinterpret_cast<const UInt8*>(secret.data()),
                                  static_cast<CFIndex>(secret.size()));
    CFDictionarySetValue(query, kSecValueData, data);

    const OSStatus status = SecItemAdd(query, nullptr);

    CFRelease(data);
    CFRelease(query);

    if (status != errSecSuccess) {
        return std::nullopt;
    }
    return std::string(kKeyringMarker);
}

std::string SecretStore::retrieve(const std::string& key, const std::string& marker) {
    if (marker != kKeyringMarker) {
        // Legacy plaintext, or a DPAPI blob written on another platform.
        return hasPrefix(marker, kEncryptedPrefix) ? "" : marker;
    }

    CFMutableDictionaryRef query = makeKeychainQuery(key);
    CFDictionarySetValue(query, kSecReturnData, kCFBooleanTrue);
    CFDictionarySetValue(query, kSecMatchLimit, kSecMatchLimitOne);

    CFTypeRef result = nullptr;
    const OSStatus status = SecItemCopyMatching(query, &result);
    CFRelease(query);

    if (status != errSecSuccess || result == nullptr) {
        return "";
    }

    CFDataRef data = static_cast<CFDataRef>(result);
    std::string secret(reinterpret_cast<const char*>(CFDataGetBytePtr(data)),
                       static_cast<size_t>(CFDataGetLength(data)));
    CFRelease(result);

    return secret;
}

void SecretStore::erase(const std::string& key) {
    CFMutableDictionaryRef query = makeKeychainQuery(key);
    SecItemDelete(query);
    CFRelease(query);
}

// ---------------------------------------------------------------------------
#else  // Linux

std::optional<std::string> SecretStore::store(const std::string& key, const std::string& secret) {
    GError* error = nullptr;
    const std::string label = "Tempus: " + key;

    const gboolean stored = secret_password_store_sync(
        tempusSchema(), SECRET_COLLECTION_DEFAULT, label.c_str(), secret.c_str(), nullptr, &error,
        "key", key.c_str(), nullptr);

    if (error) {
        g_error_free(error);
    } else if (stored) {
        return std::string(kKeyringMarker);
    }

    // No Secret Service (headless session, no keyring daemon) — fall back to the key file.
    return encryptWithKeyFile(key, secret);
}

std::string SecretStore::retrieve(const std::string& key, const std::string& marker) {
    if (hasPrefix(marker, kEncryptedPrefix)) {
        return decryptWithKeyFile(key, marker.substr(std::strlen(kEncryptedPrefix)));
    }
    if (marker != kKeyringMarker) {
        return marker;  // legacy plaintext
    }

    GError* error = nullptr;
    gchar* password =
        secret_password_lookup_sync(tempusSchema(), nullptr, &error, "key", key.c_str(), nullptr);

    if (error) {
        std::cerr << "Warning: failed to read secret '" << key << "' from the keyring: "
                  << error->message << std::endl;
        g_error_free(error);
        return "";
    }
    if (!password) {
        return "";
    }

    std::string secret(password);
    secret_password_free(password);

    return secret;
}

void SecretStore::erase(const std::string& key) {
    GError* error = nullptr;
    secret_password_clear_sync(tempusSchema(), nullptr, &error, "key", key.c_str(), nullptr);
    if (error) {
        g_error_free(error);
    }
}

#endif

}  // namespace timetracker::utils
