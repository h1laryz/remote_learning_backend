#!/bin/bash

SCRIPT_DIR=$(dirname "$(readlink -f "$0")")
ROOT_PROJECT_DIR=${SCRIPT_DIR}/../

awslocal s3api create-bucket --bucket homework

awslocal s3api put-bucket-cors --bucket homework --cors-configuration file://${SCRIPT_DIR}/cors-config.json