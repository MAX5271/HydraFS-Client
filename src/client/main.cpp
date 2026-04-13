#include <iostream>
#include <fstream>
#include <vector>
#include <boost/asio.hpp>
#include "hydra.pb.h"

using boost::asio::ip::tcp;

// ---------------------------------------------------------
// FUNCTION: Open a connection and fire a single shard
// ---------------------------------------------------------
void dispatch_shard(int shard_id, int total_shards, const std::vector<char>& data, size_t chunk_size) {
    boost::asio::io_context io;
    tcp::resolver resolver(io);
    auto endpoints = resolver.resolve("10.42.0.122", "8080"); 
    tcp::socket socket(io);
    boost::asio::connect(socket, endpoints);

    hydrafs::FileShard shard;
    shard.set_filename("heavy_payload.bin");
    shard.set_shard_id(shard_id);
    shard.set_total_shards(total_shards);
    
    // Pass the raw memory pointer and the exact number of bytes
    shard.set_data(data.data(), chunk_size); 

    std::string serialized_data;
    shard.SerializeToString(&serialized_data);

    boost::asio::write(socket, boost::asio::buffer(serialized_data));
    std::cout << "[NETWORK] -> Dispatched Shard " << shard_id << "/" << total_shards 
              << " (" << chunk_size << " bytes)" << std::endl;
}

// ---------------------------------------------------------
// MAIN ENGINE: The Slicer
// ---------------------------------------------------------
int main() {
    std::string filepath = "heavy_payload.bin";
    const size_t CHUNK_SIZE = 1024 * 1024; // 1 Megabyte

    std::ifstream infile(filepath, std::ios::binary | std::ios::ate);
    if (!infile) {
        std::cerr << "Failed to open file." << std::endl;
        return 1;
    }

    // Calculate total size and total chunks needed
    size_t file_size = infile.tellg();
    infile.seekg(0, std::ios::beg);
    int total_shards = (file_size + CHUNK_SIZE - 1) / CHUNK_SIZE;

    std::cout << "Target: " << filepath << " | Size: " << file_size << " bytes." << std::endl;
    std::cout << "Slicing into " << total_shards << " discrete shards...\n" << std::endl;

    std::vector<char> buffer(CHUNK_SIZE);
    int current_shard = 1;

    // Slide the memory window across the file
    while (infile.read(buffer.data(), CHUNK_SIZE) || infile.gcount() > 0) {
        size_t bytes_read = infile.gcount(); // How many bytes we actually grabbed
        
        dispatch_shard(current_shard, total_shards, buffer, bytes_read);
        current_shard++;
    }

    infile.close();
    std::cout << "\n[SUCCESS] Entire file completely transmitted." << std::endl;
    return 0;
}