

up:
	docker compose up

prune:
	docker system prune -a -f
	docker volume prune -a -f

clean:
	@if [ ! -z "$$(docker ps -aq)" ]; then \
		docker stop $$(docker ps -aq); \
		docker rm $$(docker ps -aq); \
	fi
	@if [ ! -z "$$(docker images -aq)" ]; then \
		docker rmi -f $$(docker images -aq); \
	fi
	@if [ ! -z "$$(docker volume ls -q)" ]; then \
		docker volume rm $$(docker volume ls -q); \
	fi
	@if [ ! -z "$$(docker network ls -q --filter type=custom)" ]; then \
		docker network rm $$(docker network ls -q --filter type=custom); \
	fi
	@echo "$(GREEN)Deleted all docker containers, volumes, networks, and images succesfully$(END)"

fclean: clean
	rm -rf $(VOLUMES_DIR)

re: clean up

#https://docs.docker.com/reference/cli/docker/compose/

.PHONY: up prune clean fclean re