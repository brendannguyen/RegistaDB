# Build Environment
FROM debian:bookworm AS builder
WORKDIR /app

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
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
    && rm -rf /var/lib/apt/lists/*

COPY . .

WORKDIR /app/regista_db
RUN mkdir -p build && \
    cd build && \
    cmake .. && \
    make

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
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /app/regista_db/build/registadb_engine .

EXPOSE 5555
EXPOSE 5556
CMD ["./registadb_engine", "--path", "/data"]