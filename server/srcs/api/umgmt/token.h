#ifndef TOKEN_H
#define TOKEN_H

#include <stddef.h>

int token_generate(const char* username, const char* email, int uid, char* out_token, size_t out_token_len);
int token_validate(const char* token, char** out_username, char** out_email, int* out_uid);

#endif /* TOKEN_H */