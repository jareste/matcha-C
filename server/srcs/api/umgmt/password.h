#ifndef PASSWORD_H
#define PASSWORD_H

#include <stdbool.h>
#include <stddef.h>

/*
 * Password storage. Hashes are self-describing strings:
 *
 *   pbkdf2_sha256$<iterations>$<salt-hex>$<hash-hex>
 *
 * which is 118 characters wide and so fits users.password_hash VARCHAR(128).
 */
#define PASSWORD_HASH_MAX 128

/* Writes a new salted hash of `password` into out. SUCCESS / ERROR. */
int password_hash(const char *password, char *out, size_t out_size);

/*
 * Constant-time verification against a stored value. A stored value with no
 * algorithm prefix is treated as a legacy plaintext password so existing rows
 * still authenticate; password_is_legacy() says when that happened, so the
 * caller can re-hash and store the upgrade.
 */
bool password_verify(const char *password, const char *stored);
bool password_is_legacy(const char *stored);

#endif /* PASSWORD_H */
