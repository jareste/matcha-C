#!/bin/sh
set -e

CONFIG=/app/config

: "${DB_USER:?not set - compose reads it from .env, run 'make secrets' first}"
: "${DB_PASSWORD:?not set - compose reads it from .env, run 'make secrets' first}"
: "${DB_NAME:?not set - compose reads it from .env, run 'make secrets' first}"
: "${TOKEN_SECRET:?not set - compose reads it from .env, run 'make secrets' first}"

update_config() {
    key="$1"
    val="$2"
    [ -n "$val" ] || return 0
    if grep -q "^${key}=" "$CONFIG"; then
        sed -i "s|^${key}=.*|${key}=${val}|" "$CONFIG"
    else
        echo "${key}=${val}" >> "$CONFIG"
    fi
}

update_config DB_HOST      "${DB_HOST:-localhost}"
update_config DB_PORT      "${DB_PORT:-5432}"
update_config DB_USER      "$DB_USER"
update_config DB_PASSWORD  "$DB_PASSWORD"
update_config DB_NAME      "$DB_NAME"
update_config TOKEN_SECRET "$TOKEN_SECRET"
update_config PORT         "${PORT:-8080}"
update_config LOG_LEVEL    "${LOG_LEVEL:-10}"
update_config SSL_ENABLE   "${SSL_ENABLE:-n}"
update_config CERT_PATH    "${CERT_PATH:-certs/cert.pem}"
update_config KEY_PATH     "${KEY_PATH:-certs/key.pem}"

chmod 600 "$CONFIG"

exec "$@"
