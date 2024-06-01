#!/bin/bash

SCRIPT_DIR=$(dirname "$(readlink -f "$0")")
ROOT_PROJECT_DIR=${SCRIPT_DIR}/../

export PREFIX="/home/user/.local/bin/"

export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH

make install-${BUILD_CONFIGURATION}

/home/user/.local/bin/remote_learning_backend_api \
  --config /home/user/.local/bin/static_config.yaml \
  --config_vars /home/user/.local/bin/config_vars.docker.yaml
