#!/bin/bash

BASE_DIR=$(dirname $(readlink -f "$0"))
ROOT_PROJECT_DIR=${BASE_DIR}/../../../
USERVER_DIR=${ROOT_PROJECT_DIR}/third_party/userver

echo "============= Initing userver submodule ============="
git submodule update --init ${USERVER_DIR}

echo "============= Installing ============="
sudo apt install -y $(cat ${USERVER_DIR}/scripts/docs/en/deps/ubuntu-22.04.md | tr '\n' ' ')
