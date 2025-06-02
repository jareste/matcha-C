#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mail_api.h"
#include "../../inc/ft_malloc.h"
#include "../../inc/error_codes.h"

typedef struct 
{
    mail_subject_t subject_type;
    char* subject;
} mail_subject_map_t;

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

void mail_init()
{
    printf("Mail system initialized.\n");
}
