#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <vector>
#include "json.hpp"
#include <fstream>

#define PORT 3000

using json = nlohmann::json;

// Struct to hold packet data
struct Packet {
    std::string symbol;
    char buysellindicator;
    int quantity;
    int price;
    int packetSequence;
};

// Function to receive data from server
Packet receivePacket(int sock) {
    Packet packet;
    char buffer[1024] = {0};

    // Read data from the socket
    read(sock, buffer, sizeof(buffer));

    // Extract packet details from buffer (manual parsing based on expected format)
    // Assuming data format is known
    int symbol_length = 4;  // e.g., 'AAPL'
    packet.symbol = std::string(buffer, symbol_length);
    packet.buysellindicator = buffer[symbol_length];
    packet.quantity = ntohl(*(int *)(buffer + symbol_length + 1));
    packet.price = ntohl(*(int *)(buffer + symbol_length + 5));
    packet.packetSequence = ntohl(*(int *)(buffer + symbol_length + 9));

    return packet;
}

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char request[2] = {0x1, 0x00};  // Request code to fetch packets

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cout << "Socket creation error" << std::endl;
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 address from text to binary form
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        std::cout << "Invalid address/ Address not supported" << std::endl;
        return -1;
    }

    // Connect to server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cout << "Connection Failed" << std::endl;
        return -1;
    }

    // Send the request to the server to get data
    send(sock, request, sizeof(request), 0);

    std::vector<Packet> packets;
    int expected_sequence = 0;

    // Keep receiving packets until the server closes the connection
    while (true) {
        Packet packet = receivePacket(sock);
        if (packet.packetSequence == expected_sequence) {
            packets.push_back(packet);
            expected_sequence++;
        } else {
            std::cout << "Missing packet sequence: " << expected_sequence << std::endl;
            // Optionally handle missing packets by requesting them again
            // Code to request missing packet can go here
        }

        // Print packet details (for debugging purposes)
        std::cout << "Received packet: "
                  << "Symbol: " << packet.symbol
                  << ", Buy/Sell: " << packet.buysellindicator
                  << ", Quantity: " << packet.quantity
                  << ", Price: " << packet.price
                  << ", Sequence: " << packet.packetSequence << std::endl;
    }

    // Convert the packets to JSON and save to a file
    json jsonData = json::array();
    for (const auto &packet : packets) {
        jsonData.push_back({
            {"symbol", packet.symbol},
            {"buysellindicator", std::string(1, packet.buysellindicator)},
            {"quantity", packet.quantity},
            {"price", packet.price},
            {"packetSequence", packet.packetSequence}
        });
    }

    std::ofstream file("output.json");
    file << jsonData.dump(4);  // Pretty print the JSON with indentation
    file.close();

    std::cout << "Data saved to output.json" << std::endl;

    // Close the socket
    close(sock);
    return 0;
}
