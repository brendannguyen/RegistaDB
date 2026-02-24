# Build Environment
FROM debian:bookworm AS builder
WORKDIR /app

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    ca-certificates \
    pkg-config \
    protobuf-compiler \
    libprotobuf-dev \
    librocksdb-dev \
    libsnappy-dev \
    libzstd-dev \
    liblz4-dev \
    libbz2-dev \
    zlib1g-dev \
    libzmq3-dev \
    cppzmq-dev \
    prometheus-cpp-dev \
    libjsoncpp-dev \
    uuid-dev \
    libcurl4-openssl-dev \
    && rm -rf /var/lib/apt/lists/*

COPY . .

WORKDIR /app/regista_db
RUN mkdir -p build && \
    cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    make -j$(nproc)

# Runtime
FROM debian:bookworm-slim
WORKDIR /root/

RUN apt-get update && apt-get install -y --no-install-recommends \
    libprotobuf32 \
    librocksdb7.8 \
    rocksdb-tools \
    libsnappy1v5 \
    libzstd1 \
    liblz4-1 \
    libbz2-1.0 \
    zlib1g \
    libzmq5 \
    libprometheus-cpp-core1.0 \
    libprometheus-cpp-pull1.0 \
    libjsoncpp25 \
    libuuid1 \
    libcurl4 \
    procps \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /app/regista_db/build/registadb_engine .
COPY --from=builder /app/static ./static
RUN chmod -R 755 /root/static

EXPOSE 5555
EXPOSE 5556
EXPOSE 8080
EXPOSE 8081
CMD ["./registadb_engine", "--path", "/data"]