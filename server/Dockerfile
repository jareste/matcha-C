# ---------- build stage ----------
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    make \
    libssl-dev \
    libpq-dev \
    openssl \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . .

RUN mkdir -p third_party/openssl_build/include third_party/openssl_build/lib

RUN make POSTGRESS_LIB="-lpq"

# ---------- runtime stage ----------
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    libssl3 \
    libpq5 \
    libasan6 \
    openssl \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=builder /app/matCha /app/matCha
COPY --from=builder /app/config /app/config

COPY --from=builder /app/certs /app/certs
COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

EXPOSE 8080

ENTRYPOINT ["/entrypoint.sh"]
CMD ["/app/matCha"]
