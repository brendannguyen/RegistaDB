# RegistaDB
A high-performance C++ middleware engine using RocksDB to orchestrate data ingestion streams from Java applications.


## Guide

### Setup RocksDB & RegistaDB Engine

1. Install dependencies

```
sudo apt update
sudo apt install librocksdb-dev libsnappy-dev libzstd-dev liblz4-dev libbz2-dev zlib1g-dev rocksdb-tools
sudo apt install build-essential gdb
sudo apt install protobuf-compiler
```

2. Create C++ build files

```
mkdir regista_db/build
mkdir regista_db/data
cd regista_db/build

cmake ..
make
```

3. Run the engine

```
./registadb_engine
```

### Development Guide

#### Playbook changes (proto)

1. Rebuild registaDB engine

```
cd regista_db/build
cmake ..
make
```

#### Engine Changes (src, c++)

1. Compile & run

```
cd regista_db/build
make && ./registadb_engine
```