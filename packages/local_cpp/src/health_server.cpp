#include "health_server.hpp"
#include <iostream>
#include <sstream>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

HealthServer::HealthServer(const HealthConfig& config) : config_(config) {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
}

HealthServer::~HealthServer() {
    stop();
#ifdef _WIN32
    WSACleanup();
#endif
}

bool HealthServer::start() {
    if (running_.load()) {
        return true;
    }
    
    running_ = true;
    server_thread_ = std::thread(&HealthServer::server_loop, this);
    
    std::cout << "Health server started on " << config_.bind_address 
              << ":" << config_.port << std::endl;
    return true;
}

void HealthServer::stop() {
    if (!running_.load()) {
        return;
    }
    
    running_ = false;
    if (server_thread_.joinable()) {
        server_thread_.join();
    }
    
    std::cout << "Health server stopped" << std::endl;
}

void HealthServer::update_metrics(const std::map<std::string, double>& metrics) {
    custom_metrics_ = metrics;
    last_update_ = std::chrono::system_clock::now();
}

void HealthServer::set_ai_healthy(bool healthy) {
    ai_healthy_.store(healthy);
}

void HealthServer::set_stream_connected(bool connected) {
    stream_connected_.store(connected);
}

void HealthServer::set_fps(double fps) {
    current_fps_.store(fps);
}

void HealthServer::set_queue_size(size_t size) {
    queue_size_.store(size);
}

void HealthServer::server_loop() {
    int server_fd = (int)socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return;
    }
    
    // Set socket options
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
    
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(config_.bind_address.c_str());
    address.sin_port = htons(config_.port);
    
    if (bind(server_fd, (sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Failed to bind socket" << std::endl;
        #ifdef _WIN32
        closesocket(server_fd);
        #else
        close(server_fd);
        #endif
        return;
    }
    
    if (listen(server_fd, 3) < 0) {
        std::cerr << "Failed to listen on socket" << std::endl;
        #ifdef _WIN32
        closesocket(server_fd);
        #else
        close(server_fd);
        #endif
        return;
    }
    
    while (running_.load()) {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_fd = (int)accept(server_fd, (sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            if (running_.load()) {
                std::cerr << "Failed to accept connection" << std::endl;
            }
            continue;
        }
        
        // Read request
        char buffer[4096];
        int bytes_read = (int)recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            std::string request(buffer);
            
            HttpRequest req = parse_request(request);
            std::string response = handle_request(req.method, req.path);
            
            send(client_fd, response.c_str(), (int)response.length(), 0);
        }
        
        #ifdef _WIN32
        closesocket(client_fd);
        #else
        close(client_fd);
        #endif
    }
    
    #ifdef _WIN32
    closesocket(server_fd);
    #else
    close(server_fd);
    #endif
}

std::string HealthServer::handle_request(const std::string& method, const std::string& path) {
    if (path == "/healthz") {
        return get_health_response();
    } else if (path == "/metrics") {
        return get_metrics_response();
    } else if (path == "/status") {
        return get_status_response();
    } else if (path.rfind("/discover", 0) == 0 && method == "POST") {
        if (!discover_handler_) {
            return create_response(501, "application/json", "{\"error\":\"discover not implemented\"}");
        }
        // very simple parse: expect /discover?ip=...&user=...&pass=...
        auto qpos = path.find('?');
        std::string query = qpos != std::string::npos ? path.substr(qpos + 1) : std::string();
        std::string result = discover_handler_(query);
        return create_response(200, "application/json", result);
    } else {
        return create_response(404, "text/plain", "Not Found");
    }
}

std::string HealthServer::get_health_response() {
    bool healthy = true;
    
    if (health_check_) {
        healthy = health_check_();
    } else {
        healthy = ai_healthy_.load() && stream_connected_.load();
    }
    
    int status = healthy ? 200 : 503;
    std::string body = healthy ? "OK" : "Service Unavailable";
    
    return create_response(status, "text/plain", body);
}
 
std::string HealthServer::get_metrics_response() {
    std::ostringstream oss;
    
    oss << "# HELP cpp_client_fps Current frames per second\n";
    oss << "# TYPE cpp_client_fps gauge\n";
    oss << "cpp_client_fps " << current_fps_.load() << "\n";
    
    oss << "# HELP cpp_client_queue_size Current queue size\n";
    oss << "# TYPE cpp_client_queue_size gauge\n";
    oss << "cpp_client_queue_size " << queue_size_.load() << "\n";
    
    oss << "# HELP cpp_client_ai_healthy AI service health status\n";
    oss << "# TYPE cpp_client_ai_healthy gauge\n";
    oss << "cpp_client_ai_healthy " << (ai_healthy_.load() ? 1 : 0) << "\n";
    
    oss << "# HELP cpp_client_stream_connected Stream connection status\n";
    oss << "# TYPE cpp_client_stream_connected gauge\n";
    oss << "cpp_client_stream_connected " << (stream_connected_.load() ? 1 : 0) << "\n";
    
    // Add custom metrics
    for (const auto& [name, value] : custom_metrics_) {
        oss << "# HELP cpp_client_" << name << " Custom metric\n";
        oss << "# TYPE cpp_client_" << name << " gauge\n";
        oss << "cpp_client_" << name << " " << value << "\n";
    }
    
    return create_response(200, "text/plain", oss.str());
}

std::string HealthServer::get_status_response() {
    std::ostringstream oss;
    
    oss << "{\n";
    oss << "  \"status\": \"" << (ai_healthy_.load() && stream_connected_.load() ? "healthy" : "unhealthy") << "\",\n";
    oss << "  \"ai_service\": " << (ai_healthy_.load() ? "true" : "false") << ",\n";
    oss << "  \"stream_connected\": " << (stream_connected_.load() ? "true" : "false") << ",\n";
    oss << "  \"fps\": " << current_fps_.load() << ",\n";
    oss << "  \"queue_size\": " << queue_size_.load() << ",\n";
    oss << "  \"timestamp\": " << std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count() << "\n";
    oss << "}\n";
    
    return create_response(200, "application/json", oss.str());
}

HealthServer::HttpRequest HealthServer::parse_request(const std::string& request) {
    HttpRequest req;
    
    std::istringstream iss(request);
    std::string line;
    if (std::getline(iss, line)) {
        std::istringstream line_stream(line);
        line_stream >> req.method >> req.path >> req.version;
    }
    
    return req;
}

std::string HealthServer::create_response(int status, const std::string& content_type, const std::string& body) {
    std::ostringstream oss;
    
    oss << "HTTP/1.1 " << status << " ";
    switch (status) {
        case 200: oss << "OK"; break;
        case 404: oss << "Not Found"; break;
        case 503: oss << "Service Unavailable"; break;
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


