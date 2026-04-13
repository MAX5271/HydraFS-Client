# HydraFS - Client Engine (The Slicer) 🐉

HydraFS is a high-performance, C++ based distributed storage system designed to shard, transmit, and manage data across a network of storage nodes. The **Client Engine** serves as the "Slicer," responsible for breaking large files into manageable binary shards and ensuring they are securely transmitted to the Raspberry Pi Vault.

## 🚀 Features

* **Dynamic File Sharding**: Slices heavy binary payloads into 1MB discrete shards for memory-efficient transmission.
* **Protobuf Serialization**: Utilizes Google Protocol Buffers for structured, language-agnostic binary serialization.
* **Pre-flight Health Checks**: Implements a "Heartbeat" protocol to verify storage node availability before initiating heavy I/O tasks.
* **Network Resilience**: 
    * **Half-Close Logic**: Uses TCP `shutdown_send` to signal EOF without closing the receive pipe.
    * **Throttling**: Built-in 100ms delays between shards to prevent buffer overflows on lower-power nodes (Raspberry Pi).
    * **Fault Tolerance**: Robust exception handling for `Broken Pipe` and `Connection Reset` errors.

## 🛠️ Tech Stack

* **Language**: C++17
* **Networking**: Boost.Asio
* **Serialization**: Google Protocol Buffers (Protobuf)
* **Build System**: CMake

## 📦 Prerequisites

Ensure you have the following dependencies installed on your Linux Mint environment:

```bash
sudo apt update
sudo apt install cmake g++ libprotobuf-dev protobuf-compiler libboost-all-dev
```

## 🏗️ Building the Client

1. **Clone the repository**:
   ```bash
   git clone https://github.com/MAX5271/HydraFS-Client.git
   cd HydraFS-Client
   ```

2. **Generate Protobuf classes**:
   ```bash
   protoc -I=src/proto --cpp_out=src/proto src/proto/hydra.proto
   ```

3. **Compile**:
   ```bash
   mkdir build && cd build
   cmake ..
   make
   ```

## 🚦 Usage

Before running the client, ensure the **HydraFS-Worker** is active on the target node (e.g., Raspberry Pi).

1. Place your target file (e.g., `heavy_payload.bin`) in the `build` directory.
2. Update the `PI_IP` in `src/client/main.cpp` to match your node's address.
3. Run the Slicer:
   ```bash
   ./hydra_client
   ```

## 📊 Data Integrity Proof

HydraFS prioritizes 100% data fidelity. You can verify the success of a transfer by comparing the MD5 checksums of the original file and the reassembled file on the vault:

```bash
# On Laptop
md5sum heavy_payload.bin

# On Raspberry Pi (Vault)
md5sum /mnt/vault/FINAL_FILE.bin
```
