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
sudo apt install libzmq3-dev
```

2. Create C++ build files

```
mkdir data
mkdir regista_db/build
cd regista_db/build

cmake ..
make
```

3. Run the engine

```
./registadb_engine
```

### Setup java client

1. Install dependenices

```
sudo apt install maven
```

2. Compile

```
cd java_client
mvn compile
```

3. Run Java Producer

```
mvn exec:java -Dexec.mainClass="com.registadb.Producer"
mvn exec:java -Dexec.mainClass="com.registadb.Producer" -Dexec.args="500"
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

### Client changes (java)

1. Compile

```
cd java_client
mvn compile
```

2. Run Producer

```
mvn exec:java -Dexec.mainClass="com.registadb.Producer"
```