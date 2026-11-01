#ifndef TOKEN_H
#define TOKEN_H

#include <stddef.h>

/* Must be called once at startup with a non-empty secret; token_generate and
 * token_validate both fail until it is. */
int token_init(const char* secret);
void token_cleanup();

int token_generate(const char* username, const char* email, int uid, char* out_token, size_t out_token_len);
int token_validate(const char* token, char** out_username, char** out_email, int* out_uid);

#endif /* TOKEN_H */