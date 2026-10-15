#!/bin/sh
set -e

CONFIG=/app/config

# Rewrite ./config from the environment, so the same image works against any
# database/port without rebuilding. matCha reads "config" relative to its cwd.
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

update_config DB_HOST     "${DB_HOST:-localhost}"
update_config DB_PORT     "${DB_PORT:-5432}"
update_config DB_USER     "${DB_USER:-admin}"
update_config DB_PASSWORD "${DB_PASSWORD:-1234qwer}"
update_config DB_NAME     "${DB_NAME:-matcha_db}"
update_config PORT        "${PORT:-8080}"
update_config LOG_LEVEL   "${LOG_LEVEL:-10}"

exec "$@"
