FROM ghcr.io/userver-framework/ubuntu-22.04-userver-pg:latest

WORKDIR /install

RUN apt-get -y update && apt-get -y upgrade
RUN apt-get install -y sudo

ENV DEBIAN_FRONTEND=noninteractive
RUN ln -fs /usr/share/zoneinfo/UTC /etc/localtime && \
    apt install --quiet --yes --no-install-recommends tzdata && \
    dpkg-reconfigure --frontend noninteractive tzdata

COPY scripts scripts

RUN ./scripts/install_env.sh

EXPOSE "8080"