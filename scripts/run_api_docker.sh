#!/bin/bash

SCRIPT_DIR=$(dirname "$(readlink -f "$0")")
ROOT_PROJECT_DIR=${SCRIPT_DIR}/../

/home/user/.local/bin/remote_learning_backend \
  --config /home/user/.local/etc/remote_learning_backend/static_config.yaml \
  --config_vars /home/user/.local/etc/remote_learning_backend/config_vars.docker.yaml
