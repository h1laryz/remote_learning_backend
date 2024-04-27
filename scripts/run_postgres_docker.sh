#!/bin/bash

SCRIPT_DIR=$(dirname "$(readlink -f "$0")")
ROOT_PROJECT_DIR=${SCRIPT_DIR}/../

docker run --name remote_learning_postgres \
  -e POSTGRES_DB=remote_learning_backend_db_1 \
  -e POSTGRES_USER=user \
  -e POSTGRES_PASSWORD=password \
  -p 5432:5432 \
  -v $(ROOT_PROJECT_DIR)/postgresql/schemas:/docker-entrypoint-initdb.d \
  -v $(ROOT_PROJECT_DIR)/.pgdata:/var/lib/postgresql/data \
  --network postgres \
  postgres:12
