#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <functional>
#include <sstream>
#include <cstring>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <map>
#include <memory>
#include <mutex>
#pragma comment(lib, "ws2_32.lib")

struct HealthConfig {
    std::string bind_address = "127.0.0.1";
    int port = 8086;
    bool enabled = true;
};

class HealthServer {
public:
    explicit HealthServer(const HealthConfig& config) : config_(config) {
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
    }
    
    ~HealthServer() {
        stop();
        WSACleanup();
    }
    
    bool start() {
        if (running_.load()) {
            return true;
        }
        
        running_ = true;
        server_thread_ = std::thread(&HealthServer::server_loop, this);
        
        std::cout << "C++ Discovery & Streaming Service started on " << config_.bind_address 
                  << ":" << config_.port << std::endl;
        return true;
    }
    
    void stop() {
        if (!running_.load()) {
            return;
        }
        
        running_ = false;
        if (server_thread_.joinable()) {
            server_thread_.join();
        }
        
        std::cout << "C++ Discovery & Streaming Service stopped" << std::endl;
    }
    
    void set_discover_handler(std::function<std::string(const std::string&)> handler) {
        discover_handler_ = std::move(handler);
    }

private:
    HealthConfig config_;
    std::atomic<bool> running_{false};
    std::thread server_thread_;
    std::function<std::string(const std::string&)> discover_handler_;
    
    void server_loop() {
        int server_fd = (int)socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) {
            std::cerr << "Failed to create socket" << std::endl;
            return;
        }
        
        int opt = 1;
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
        
        sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = inet_addr(config_.bind_address.c_str());
        address.sin_port = htons(config_.port);
        
        if (bind(server_fd, (sockaddr*)&address, sizeof(address)) < 0) {
            std::cerr << "Failed to bind socket" << std::endl;
            closesocket(server_fd);
            return;
        }
        
        if (listen(server_fd, 3) < 0) {
            std::cerr << "Failed to listen on socket" << std::endl;
            closesocket(server_fd);
            return;
        }
        
        std::cout << "Listening for connections..." << std::endl;
        
        while (running_.load()) {
            sockaddr_in client_addr;
            int client_len = sizeof(client_addr);
            
            int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
            if (client_fd < 0) {
                if (running_.load()) {
                    std::cerr << "Failed to accept connection" << std::endl;
                }
                continue;
            }
            
            char buffer[4096];
            int bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
            if (bytes_read > 0) {
                buffer[bytes_read] = '\0';
                std::string request(buffer);
                
                std::string response = handle_request(request);
                send(client_fd, response.c_str(), (int)response.length(), 0);
            }
            
            closesocket(client_fd);
        }
        
        closesocket(server_fd);
    }
    
    std::string handle_request(const std::string& request) {
        std::istringstream iss(request);
        std::string line;
        std::getline(iss, line);
        
        std::istringstream line_stream(line);
        std::string method, path, version;
        line_stream >> method >> path >> version;
        
        std::string query;
        auto qpos = path.find('?');
        if (qpos != std::string::npos) {
            query = path.substr(qpos + 1);
            path = path.substr(0, qpos);
        }
        
        std::cout << "Request: " << method << " " << path << std::endl;
        
        if (path == "/health") {
            return create_response(200, "application/json", "{\"status\":\"ok\"}");
        } else if (path == "/discover" && method == "POST") {
            if (!discover_handler_) {
                return create_response(501, "application/json", "{\"error\":\"discover not implemented\"}");
            }
            
            std::string result = discover_handler_(query);
            return create_response(200, "application/json", result);
        } else {
            return create_response(404, "text/plain", "Not Found");
        }
    }
    
    std::string create_response(int status, const std::string& content_type, const std::string& body) {
        std::ostringstream oss;
        
        oss << "HTTP/1.1 " << status << " ";
        switch (status) {
            case 200: oss << "OK"; break;
            case 501: oss << "Not Implemented"; break;
            case 404: oss << "Not Found"; break;
            default: oss << "Unknown"; break;
        }
        oss << "\r\n";
        
        oss << "Content-Type: " << content_type << "\r\n";
        oss << "Content-Length: " << body.length() << "\r\n";
        oss << "Connection: close\r\n";
        oss << "\r\n";
        oss << body;
        
        return oss.str();
    }
};

// Discovery handler function
std::string discovery_handler(const std::string& query) {
    auto get_param = [&](const std::string& key) -> std::string {
        auto pos = query.find(key + "=");
        if (pos == std::string::npos) return {};
        auto start = pos + key.size() + 1;
        auto end = query.find('&', start);
        return query.substr(start, end == std::string::npos ? std::string::npos : end - start);
    };
    
    std::string ip = get_param("ip");
    std::string user = get_param("user");
    std::string pass = get_param("pass");
    std::string brand = get_param("brand");
    
    if (ip.empty() || user.empty()) {
        return "{\"success\":false,\"error\":\"ip and user required\"}";
    }
    
    std::vector<std::string> candidates;
    auto add = [&](const std::string& path) {
        candidates.emplace_back("rtsp://" + user + ":" + pass + "@" + ip + ":554/" + path);
    };
    
    if (brand == "reolink") {
        add("h264Preview_01_sub");
        add("h264Preview_01_main");
    } else if (brand == "hikvision") {
        add("Streaming/Channels/101");
        add("Streaming/Channels/102");
    } else if (brand == "dahua") {
        add("cam/realmonitor?channel=1&subtype=1");
        add("cam/realmonitor?channel=1&subtype=0");
    }
    
    add("stream1");
    add("live/ch00_0");
    add("avstream/channel=1");
    
    std::string json = "{\"success\":true,\"candidates\":[";
    for (size_t i = 0; i < candidates.size(); ++i) {
        json += "\"" + candidates[i] + "\"";
        if (i + 1 < candidates.size()) json += ",";
    }
    json += "]}";
    
    return json;
}

int main() {
    std::cout << "Starting C++ Discovery & Streaming Service..." << std::endl;
    
    HealthConfig config;
    config.bind_address = "127.0.0.1";
    config.port = 8086;
    
    HealthServer server(config);
    server.set_discover_handler(discovery_handler);
    
    if (!server.start()) {
        std::cerr << "Failed to start discovery service" << std::endl;
        return 1;
    }
    
    std::cout << "C++ Discovery & Streaming Service is running. Press Ctrl+C to stop." << std::endl;
    
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    return 0;
}


