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

// #define USE_MAIL

#ifdef USE_MAIL
int mail_notify_msg(mail_context_t* context);
int mail_send_registration(const char* to, const char* username, char* password);
int verify_code(const char* email, int submitted_code);
void mail_init(void);
void cleanup_expired_codes(void);
#else
#define mail_notify_msg(context) (0)
#define mail_send_registration(to, username, password) (0)
#define verify_code(email, submitted_code) (1)
#define mail_init() ((void)0)
#define cleanup_expired_codes() ((void)0)
#endif

#endif /* MAIL_API_H */
