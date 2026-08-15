#pragma once

#include <optional>
#include <string>

namespace timetracker::utils {

/**
 * Storage for sensitive settings values, backed by the platform's native facility.
 *
 * The `settings` table never holds a raw secret — it holds a *marker*:
 *   "enc:v1:<base64>"  the secret is encrypted inside the marker itself
 *                      (Windows DPAPI, or the OpenSSL keyfile fallback on Linux)
 *   "keyring:v1"       the secret lives in the OS vault (macOS Keychain, libsecret)
 *                      under `key`
 *   anything else      legacy plaintext written before encryption existed; returned
 *                      verbatim so that existing data.db files keep working
 *
 * Backends differ in shape — DPAPI encrypts a value in place while Keychain and
 * libsecret are key/value vaults — which is why store() returns a marker instead of
 * a ciphertext: the caller persists the marker and stays agnostic to which backend
 * actually holds the bytes.
 */
class SecretStore {
public:
    static constexpr const char* kEncryptedPrefix = "enc:v1:";
    static constexpr const char* kKeyringMarker = "keyring:v1";

    /**
     * Persist a secret and return the marker to write into the settings table.
     * @param key Stable identifier for the secret, e.g. "youtrack_token".
     * @param secret The value to protect.
     * @return Marker string, or std::nullopt if no backend accepted the secret.
     */
    static std::optional<std::string> store(const std::string& key, const std::string& secret);

    /**
     * Resolve a marker back to the secret it stands for.
     * @param key Same identifier that was passed to store().
     * @param marker Value read from the settings table.
     * @return The secret, or the marker itself when it is legacy plaintext, or an
     *         empty string when a known marker cannot be resolved.
     */
    static std::string retrieve(const std::string& key, const std::string& marker);

    /**
     * Drop any vault entry held for `key`. Safe to call when nothing is stored.
     */
    static void erase(const std::string& key);

    /**
     * Whether `marker` was produced by this class (as opposed to legacy plaintext).
     */
    static bool isMarker(const std::string& marker);
};

}  // namespace timetracker::utils
