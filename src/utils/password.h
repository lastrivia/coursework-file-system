#ifndef PASSWORD_H
#define PASSWORD_H

#include <cstring>
#include <sodium.h>

#include "except.h"

namespace cs2313 {

    inline void init_crypt() {
        if (sodium_init() < 0) {
            throw except(ERROR_PWD_INIT_FAIL, "Sodium init failed");
        }
    }

    inline void generate_salt(unsigned char salt[crypto_pwhash_SALTBYTES]) {
        randombytes_buf(salt, crypto_pwhash_SALTBYTES);
    }

    static constexpr size_t PWD_NONCE_LENGTH = 16;

    inline void generate_nonce(unsigned char nonce[PWD_NONCE_LENGTH]) {
        randombytes_buf(nonce, PWD_NONCE_LENGTH);
    }

    inline void compute_key(unsigned char key[crypto_auth_hmacsha256_KEYBYTES],
                            const char *password,
                            const unsigned char *salt) {
        if (crypto_pwhash(
                key,
                crypto_auth_hmacsha256_KEYBYTES,
                password,
                strlen(password),
                salt,
                crypto_pwhash_OPSLIMIT_INTERACTIVE,
                crypto_pwhash_MEMLIMIT_INTERACTIVE,
                crypto_pwhash_ALG_DEFAULT
            ) != 0) {
            throw except(ERROR_PWD_COMPUTE_FAIL, "Key calculation failed");
        }
    }

    inline void compute_hmac(unsigned char mac[crypto_auth_hmacsha256_BYTES],
                             const unsigned char *key,
                             const unsigned char *nonce) {
        crypto_auth_hmacsha256(mac, nonce, PWD_NONCE_LENGTH, key);
    }

    inline bool verify_hmac(const unsigned char *expected_mac,
                            const unsigned char *key,
                            const unsigned char *nonce) {
        unsigned char computed_mac[crypto_auth_hmacsha256_BYTES];
        compute_hmac(computed_mac, key, nonce);
        return sodium_memcmp(expected_mac, computed_mac, crypto_auth_hmacsha256_BYTES) == 0;
    }
}

#endif
