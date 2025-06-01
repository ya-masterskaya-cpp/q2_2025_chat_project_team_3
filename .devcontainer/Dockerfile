FROM mcr.microsoft.com/devcontainers/cpp:noble

RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y build-essential cmake python3-jinja2 automake libtool pkg-config bison flex autoconf ninja-build libx11-dev libxft-dev libxext-dev libxi-dev libxtst-dev libxrandr-dev && \
    rm -rf /var/lib/apt/lists/*
