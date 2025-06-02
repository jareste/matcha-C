#ifndef MAIL_API_H
#define MAIL_API_H

typedef enum
{
    SUBJECT_LOGIN = 0,
    SUBJECT_2FA,
    SUBJECT_PASSWORD_RESET,
    SUBJECT_ACCOUNT_CREATED,
    SUBJECT_ACCOUNT_DELETED,
    SUBJECT_ACCOUNT_UPDATED,
    SUBJECT_ACCOUNT_VERIFIED,
    SUBJECT_MAX
} mail_subject_t;

typedef struct
{
    char* to;
    mail_subject_t subject;
    char* body;
} mail_context_t;

int mail_notify_msg(mail_context_t* context);

#endif /* MAIL_API_H */
