FROM ghcr.io/userver-framework/ubuntu-22.04-userver-pg:latest

WORKDIR /install

RUN apt-get -y update && apt-get -y upgrade
RUN apt-get install -y sudo

COPY scripts scripts

RUN ./scripts/install_env.sh

EXPOSE "8080"