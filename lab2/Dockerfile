FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential ccache cmake git make ninja-build pkg-config \
    libabsl-dev \
    libboost-context1.74-dev \
    libboost-coroutine1.74-dev \
    libboost-filesystem1.74-dev \
    libboost-iostreams1.74-dev \
    libboost-locale1.74-dev \
    libboost-program-options1.74-dev \
    libboost-stacktrace1.74-dev \
    libboost1.74-dev \
    libc-ares-dev \
    libcctz-dev \
    libcrypto++-dev \
    libcurl4-openssl-dev \
    libev-dev \
    libfmt-dev \
    libgmock-dev \
    libgtest-dev \
    libicu-dev \
    libjemalloc-dev \
    libre2-dev \
    liblz4-dev \
    libnghttp2-dev \
    libssl-dev \
    libyaml-cpp-dev \
    libzstd-dev \
    netbase \
    python3-dev python3-jinja2 python3-venv python3-yaml \
    ragel zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

COPY ./cloud-storage /cloud-storage/

WORKDIR /cloud-storage

RUN make build-debug

EXPOSE 8080 8081 8082
