#!/bin/bash

SCRIPT_DIR=$(dirname "$(readlink -f "$0")")
ROOT_PROJECT_DIR=${SCRIPT_DIR}/../

awslocal s3api create-bucket --bucket homework-bucket

awslocal s3api put-bucket-cors --bucket homework-bucket --cors-configuration file://cors-config.json