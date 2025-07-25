



// test_client.cpp
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <thread>
#include <vector>
#include <mutex>  


std::mutex output_mutex;

void client_thread(int client_id) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    if (connect(sock, (sockaddr*)&server_addr, sizeof(server_addr)) == 0) {
        {
            std::lock_guard<std::mutex> lock(output_mutex); 
            std::cout << "Client " << client_id << " connected\n";
        }
        std::string message = "Hello from client " + std::to_string(client_id);
        send(sock, message.c_str(), message.length(), 0);
        
        char buffer[1024] = {0};
        recv(sock, buffer, sizeof(buffer), 0);
        {
            std::lock_guard<std::mutex> lock(output_mutex);
            std::cout << "Client " << client_id << " received: " << buffer << "\n";
        } 
        sleep(2);  // Keep connection alive briefly
    }
    close(sock);
    {
        std::lock_guard<std::mutex> lock(output_mutex);
        std::cout << "Client " << client_id << " disconnected\n";
    }
}

int main() {
    std::vector<std::thread> clients;
    
    // Create 5 simultaneous clients
    for (int i = 1; i <= 5; i++) {
        clients.emplace_back(client_thread, i);
    }
    
    // Wait for all clients to finish
    for (auto& client : clients) {
        client.join();
    }
    
    return 0;
}