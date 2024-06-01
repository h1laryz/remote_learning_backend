#!/bin/bash

SCRIPT_DIR=$(dirname "$(readlink -f "$0")")
ROOT_PROJECT_DIR=${SCRIPT_DIR}/../

export PREFIX="/home/user/.local/bin/"

make install-${BUILD_CONFIGURATION}

/home/user/.local/bin/remote_learning_backend_api \
  --config /home/user/.local/bin/remote_learning_backend/static_config.yaml \
  --config_vars /home/user/.local/bin/remote_learning_backend/config_vars.docker.yaml
