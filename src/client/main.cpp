#include <iostream>
#include <fstream>
#include <vector>
#include <ctime>
#include <thread>
#include <chrono>  
#include <boost/asio.hpp>
#include "hydra.pb.h"

using boost::asio::ip::tcp;

// Configuration
const std::string PI_IP = "10.42.0.122";
const std::string PI_PORT = "8080";

// ---------------------------------------------------------
// FUNCTION: Check if the Pi is alive and responsive to Heartbeat messages
// ---------------------------------------------------------
bool is_node_healthy(const std::string& ip, const std::string& port) {
    try {
        boost::asio::io_context io;
        tcp::socket socket(io);
        tcp::resolver resolver(io);
        
        // Blocking connect is fine for a pre-flight health check
        boost::asio::connect(socket, resolver.resolve(ip, port));

        // 1. Prepare Heartbeat PING
        hydrafs::Heartbeat hb;
        hb.set_status(hydrafs::Heartbeat_Status_PING);
        hb.set_timestamp(std::time(nullptr));

        std::string out;
        hb.SerializeToString(&out);

        // 2. Send Ping
        boost::asio::write(socket, boost::asio::buffer(out));

        // Signals the Pi that the message is done so its async_read finishes.
        socket.shutdown(tcp::socket::shutdown_send); 

        // 3. Wait for PONG
        char reply[1024];
        boost::system::error_code ec;
        size_t len = socket.read_some(boost::asio::buffer(reply), ec);
        
        // If the connection closed normally or we got data, it's a success
        if (ec && ec != boost::asio::error::eof) {
            throw boost::system::system_error(ec);
        }

        hydrafs::Heartbeat response;
        if (response.ParseFromArray(reply, len) && 
            response.status() == hydrafs::Heartbeat_Status_PONG) {
            std::cout << "[CHECK] Heartbeat PONG received. Node is healthy! ❤️" << std::endl;
            return true;
        }
    } catch (std::exception& e) {
        std::cerr << "[CHECK] Health check failed: " << e.what() << std::endl;
    }
    return false;
}

// ---------------------------------------------------------
// FUNCTION: Transmit a single shard
// ---------------------------------------------------------
void dispatch_shard(int shard_id, int total_shards, const std::vector<char>& data, size_t chunk_size) {
    try {
        boost::asio::io_context io;
        tcp::resolver resolver(io);
        auto endpoints = resolver.resolve(PI_IP, PI_PORT); 
        tcp::socket socket(io);
        boost::asio::connect(socket, endpoints);

        hydrafs::FileShard shard;
        shard.set_filename("heavy_payload.bin");
        shard.set_shard_id(shard_id);
        shard.set_total_shards(total_shards);
        shard.set_data(data.data(), chunk_size); 

        std::string serialized_data;
        shard.SerializeToString(&serialized_data);

        boost::asio::write(socket, boost::asio::buffer(serialized_data));
        
        // HALF-CLOSE to signal end of shard transmission
        socket.shutdown(tcp::socket::shutdown_send);

        std::cout << "[NETWORK] -> Dispatched Shard " << shard_id << "/" << total_shards 
                  << " (" << chunk_size << " bytes)" << std::endl;

    } catch (std::exception& e) {
        // Prevents "Connection reset by peer" from crashing the whole program.
        std::cerr << "[NETWORK ERROR] Shard " << shard_id << " failed: " << e.what() << std::endl;
    }
}

// ---------------------------------------------------------
// MAIN FUNCTION: Orchestrates the entire file transmission process
// ---------------------------------------------------------
int main() {
    const std::string filepath = "heavy_payload.bin";
    const size_t CHUNK_SIZE = 1024 * 1024; // 1 Megabyte

    // 1. Pre-flight check
    if (!is_node_healthy(PI_IP, PI_PORT)) {
        std::cerr << "🛑 FATAL: Raspberry Pi is unreachable. Aborting." << std::endl;
        return 1;
    }

    // 2. Open file
    std::ifstream infile(filepath, std::ios::binary | std::ios::ate);
    if (!infile) {
        std::cerr << "Failed to open file: " << filepath << std::endl;
        return 1;
    }

    size_t file_size = infile.tellg();
    infile.seekg(0, std::ios::beg);
    int total_shards = (file_size + CHUNK_SIZE - 1) / CHUNK_SIZE;

    std::cout << "Target: " << filepath << " | Size: " << file_size << " bytes." << std::endl;
    std::cout << "Slicing into " << total_shards << " shards...\n" << std::endl;

    std::vector<char> buffer(CHUNK_SIZE);
    int current_shard = 1;

    // 3. The Transmission Loop
    while (infile.read(buffer.data(), CHUNK_SIZE) || infile.gcount() > 0) {
        size_t bytes_read = infile.gcount(); 
        dispatch_shard(current_shard, total_shards, buffer, bytes_read);
        current_shard++;

        // THROTTLING
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    infile.close();
    std::cout << "\n[SUCCESS] Entire file completely transmitted. ✅" << std::endl;
    return 0;
}