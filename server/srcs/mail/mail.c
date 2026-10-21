#ifdef USE_MAIL

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mail_api.h"
#include "../../inc/ft_malloc.h"
#include "../../inc/error_codes.h"
#include <time.h>
#include <openssl/sha.h>
#include "../../third_party/uthash-master/src/uthash.h"

typedef struct 
{
    mail_subject_t subject_type;
    char* subject;
} mail_subject_map_t;

typedef struct
{
    char email[256];
    int code;
    time_t expiration_time;
    UT_hash_handle hh;
} verification_entry_t;

static mail_subject_map_t subject_map[] =
{
    {SUBJECT_LOGIN, "Login Notification"},
    {SUBJECT_2FA, "Two-Factor Authentication"},
    {SUBJECT_PASSWORD_RESET, "Password Reset Request"},
    {SUBJECT_ACCOUNT_CREATED, "Account Created Successfully"},
    {SUBJECT_ACCOUNT_DELETED, "Account Deleted"},
    {SUBJECT_ACCOUNT_UPDATED, "Account Updated"},
    {SUBJECT_ACCOUNT_VERIFIED, "Account Verified"}
};

static verification_entry_t* verification_table = NULL;

static int m_generate_verification_code(const char* email, const char* username)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    char input[512];
    int code;

    snprintf(input, sizeof(input), "%s%s%ld", email, username, time(NULL));
    SHA256((unsigned char*)input, strlen(input), hash);

    code = (hash[0] << 16 | hash[1] << 8 | hash[2]) % 1000000;
    if (code < 0) code = -code;

    return code;
}

static void m_store_verification_code(const char* email, int code, int validity_seconds)
{
    verification_entry_t* entry;

    HASH_FIND_STR(verification_table, email, entry);
    if (entry)
    {
        entry->code = code;
        entry->expiration_time = time(NULL) + validity_seconds;
    }
    else
    {
        entry = malloc(sizeof(verification_entry_t));
        strncpy(entry->email, email, sizeof(entry->email) - 1);
        entry->email[sizeof(entry->email) - 1] = '\0';
        entry->code = code;
        entry->expiration_time = time(NULL) + validity_seconds;
        HASH_ADD_STR(verification_table, email, entry);
    }
}

int verify_code(const char* email, int submitted_code)
{
    verification_entry_t* entry;
    time_t now = time(NULL);

    HASH_FIND_STR(verification_table, email, entry);
    if (!entry)
    {
        printf("No verification code found for email: %s\n", email);
        return 0;
    }

    if (entry->expiration_time < now)
    {
        printf("Verification code for email %s has expired.\n", email);
        HASH_DEL(verification_table, entry);
        free(entry);
        return 0;
    }

    if (entry->code == submitted_code)
    {
        printf("Verification successful for email: %s\n", email);
        HASH_DEL(verification_table, entry);
        free(entry);
        return 1;
    }

    printf("Incorrect verification code for email: %s\n", email);
    return 0;
}

void cleanup_expired_codes()
{
    verification_entry_t* entry;
    verification_entry_t* tmp;
    time_t now = time(NULL);

    HASH_ITER(hh, verification_table, entry, tmp)
    {
        if (entry->expiration_time < now)
        {
            printf("Removing expired code for email: %s\n", entry->email);
            HASH_DEL(verification_table, entry);
            free(entry);
        }
    }
}

int mail_notify_msg(mail_context_t* ctx)
{
    FILE *mail;
    int status;
    char command[512];
 
    if (!ctx || !ctx->to || !ctx->body || (ctx->subject < 0) || (ctx->subject >= SUBJECT_MAX))
    {
        fprintf(stderr, "Invalid mail context.\n");
        return -1;
    }

    snprintf(command, sizeof(command), "msmtp --from=default %s", ctx->to);

    mail = popen(command, "w");
    if (!mail)
    {
        perror("Failed to open msmtp");
        return -1;
    }

    fprintf(mail,
        "To: %s\n"
        "Subject: %s\n"
        "Content-Type: text/plain\n\n"
        "%s\n",
        ctx->to, subject_map[ctx->subject].subject, ctx->body);

    status = pclose(mail);
    if (status == -1)
    {
        perror("Failed to close msmtp");
        return -1;
    }
    else if (WEXITSTATUS(status) != 0)
    {
        fprintf(stderr, "msmtp exited with status %d\n", WEXITSTATUS(status));
        return -1;
    }

    return 0;
}

int mail_send_registration(const char* to, const char* username, char* password)
{
    mail_context_t context;
    char body[512];
    int code;

    if (!to || !username)
        return -1;

    code = m_generate_verification_code(to, username);

    m_store_verification_code(to, code, 300);

    snprintf(body, sizeof(body),
        "Hello %s,\n\n"
        "Welcome to Matcha! Your account has been successfully created.\n"
        "Please verify your account using the following code: %06d\n\n"
        "Best regards,\n"
        "The Matcha Team\n", username, code);

    context.to = (char*)to;
    context.subject = SUBJECT_ACCOUNT_CREATED;
    context.body = body;

    return mail_notify_msg(&context);
}

void mail_init()
{
    printf("Mail system initialized.\n");
}
#else
void KCH_Foo()
{
    (void)0;
}
#endif