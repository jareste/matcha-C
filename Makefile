UNAME_S := $(shell uname -s)

all: docker-up
	@docker compose up -d --build
	@echo "Server on http://localhost:8080 — 'make logs' to follow it."

# The idea is that before any other operation, to ensure that the Docker daemon is running.
# May need some adjustments
docker-up:
	@if docker info >/dev/null 2>&1; then \
		echo "Docker daemon already running."; \
	else \
		echo "Starting Docker daemon..."; \
		if [ "$(UNAME_S)" = "Darwin" ]; then \
			open -a Docker || { echo "Docker Desktop not found in /Applications."; exit 1; }; \
		else \
			sudo systemctl start docker 2>/dev/null || sudo service docker start || true; \
		fi; \
		printf "Waiting for Docker"; \
		i=0; \
		while ! docker info >/dev/null 2>&1; do \
			i=$$((i+1)); \
			if [ $$i -gt 90 ]; then echo " timed out after 90s."; exit 1; fi; \
			printf "."; \
			sleep 1; \
		done; \
		echo " ready."; \
	fi

logs: docker-up
	@docker compose logs -f server

ps: docker-up
	@docker compose ps

restart: docker-up
	@docker compose restart server

# Rebuild the image from scratch (ignores the layer cache) and restart.
re: docker-up
	@docker compose build --no-cache server
	@docker compose up -d

sh: docker-up
	@docker exec -it matcha-server sh

db_connect: docker-up
	@docker exec -it matcha-db psql -U admin -d matcha_db

down:
	@docker compose down

# Also drops the postgres volume: every user, message and picture goes away.
fclean:
	@echo "Removing containers AND the database volume..."
	@docker compose down -v

.PHONY: all docker-up logs ps restart re sh db_connect down fclean
