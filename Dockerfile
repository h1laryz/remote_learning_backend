FROM ghcr.io/userver-framework/ubuntu-22.04-userver-pg:latest

WORKDIR /app
COPY ~/remote_learning_backend/scripts .
RUN ./scripts/install_env.sh