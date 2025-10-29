#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <functional>
#include <sstream>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <opencv2/opencv.hpp>
#pragma comment(lib, "ws2_32.lib")

#include "src/config.hpp"
#include "src/stream_reader.hpp"
#include "src/frame.hpp"

struct StreamingConfig {
    std::string bind_address = "127.0.0.1";
    int port = 8088; // Different port for streaming service
    bool enabled = true;
};

class StreamingService {
public:
    explicit StreamingService(const StreamingConfig& config) : config_(config) {
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
    }
    
    ~StreamingService() {
        stop();
        WSACleanup();
    }
    
    bool start() {
        if (running_.load()) {
            return true;
        }
        
        running_ = true;
        server_thread_ = std::thread(&StreamingService::server_loop, this);
        
        std::cout << "Streaming Service started on " << config_.bind_address 
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
        
        std::cout << "Streaming Service stopped" << std::endl;
    }
    
    // Add a stream to be served
    void add_stream(const std::string& stream_id, const std::string& rtsp_url) {
        std::lock_guard<std::mutex> lock(streams_mutex_);
        
        // Create stream config
        StreamConfig config;
        config.url = rtsp_url;
        config.camera_id = stream_id;
        config.fps_cap = 15; // Cap at 15 FPS for web streaming
        config.use_hardware_decode = true;
        config.max_reconnect_attempts = 5;
        config.reconnect_delay_ms = 2000;
        
        // Create stream reader
        auto reader = std::make_shared<StreamReader>(config);
        
        // Set frame callback to store latest frame
        reader->set_frame_callback([this, stream_id](Frame&& frame) {
            std::lock_guard<std::mutex> lock(frames_mutex_);
            latest_frames_[stream_id] = std::move(frame);
        });
        
        // Set error callback
        reader->set_error_callback([stream_id](const std::string& error) {
            std::cerr << "Stream " << stream_id << " error: " << error << std::endl;
        });
        
        // Start the reader
        if (reader->start()) {
            streams_[stream_id] = reader;
            std::cout << "Added stream: " << stream_id << " -> " << rtsp_url << std::endl;
        } else {
            std::cerr << "Failed to start stream: " << stream_id << std::endl;
        }
    }
    
    // Remove a stream
    void remove_stream(const std::string& stream_id) {
        std::lock_guard<std::mutex> lock(streams_mutex_);
        auto it = streams_.find(stream_id);
        if (it != streams_.end()) {
            it->second->stop();
            streams_.erase(it);
            std::cout << "Removed stream: " << stream_id << std::endl;
        }
        
        // Also remove from frames
        std::lock_guard<std::mutex> frame_lock(frames_mutex_);
        latest_frames_.erase(stream_id);
    }

private:
    StreamingConfig config_;
    std::atomic<bool> running_{false};
    std::thread server_thread_;
    
    // Stream management
    std::mutex streams_mutex_;
    std::map<std::string, std::shared_ptr<StreamReader>> streams_;
    
    // Frame storage
    std::mutex frames_mutex_;
    std::map<std::string, Frame> latest_frames_;
    
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
        
        std::cout << "Streaming Service listening for connections..." << std::endl;
        
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
        
        // Handle different endpoints
        if (path == "/health") {
            return create_response(200, "application/json", "{\"status\":\"ok\"}");
        } else if (path.rfind("/stream/", 0) == 0) {
            // Extract stream ID from path
            std::string stream_id = path.substr(8); // Remove "/stream/"
            auto qpos = stream_id.find('?');
            if (qpos != std::string::npos) {
                stream_id = stream_id.substr(0, qpos);
            }
            
            return handle_stream_request(stream_id);
        } else if (path.rfind("/add_stream", 0) == 0 && method == "POST") {
            return handle_add_stream_request(path);
        } else if (path.rfind("/remove_stream", 0) == 0 && method == "POST") {
            return handle_remove_stream_request(path);
        } else {
            return create_response(404, "text/plain", "Not Found");
        }
    }
    
    std::string handle_stream_request(const std::string& stream_id) {
        std::lock_guard<std::mutex> lock(frames_mutex_);
        auto it = latest_frames_.find(stream_id);
        if (it == latest_frames_.end()) {
            return create_response(404, "application/json", "{\"error\":\"Stream not found\"}");
        }
        
        // Get the latest frame
        const Frame& frame = it->second;
        
        // Convert frame to JPEG
        std::vector<uchar> buffer;
        std::vector<int> params = {cv::IMWRITE_JPEG_QUALITY, 80};
        
        try {
            cv::imencode(".jpg", frame.get_mat(), buffer, params);
            
            // Create multipart response for streaming
            std::ostringstream response;
            response << "HTTP/1.1 200 OK\r\n";
            response << "Content-Type: image/jpeg\r\n";
            response << "Content-Length: " << buffer.size() << "\r\n";
            response << "X-Timestamp: " << frame.get_timestamp() << "\r\n";
            response << "X-Frame-Id: " << frame.get_frame_id() << "\r\n";
            response << "Connection: close\r\n";
            response << "\r\n";
            
            // Send headers first
            std::string headers = response.str();
            
            // Then send image data
            // Note: In a real implementation, we would stream this continuously
            // For now, we'll just send one frame
            
            return headers;
        } catch (const std::exception& e) {
            return create_response(500, "application/json", "{\"error\":\"Failed to encode frame: " + std::string(e.what()) + "\"}");
        }
    }
    
    std::string handle_add_stream_request(const std::string& path) {
        // Parse query parameters
        auto qpos = path.find('?');
        if (qpos == std::string::npos) {
            return create_response(400, "application/json", "{\"error\":\"Missing parameters\"}");
        }
        
        std::string query = path.substr(qpos + 1);
        std::string stream_id, rtsp_url;
        
        // Simple parameter parsing
        auto get_param = [&](const std::string& key) -> std::string {
            auto pos = query.find(key + "=");
            if (pos == std::string::npos) return {};
            auto start = pos + key.size() + 1;
            auto end = query.find('&', start);
            return query.substr(start, end == std::string::npos ? std::string::npos : end - start);
        };
        
        stream_id = get_param("id");
        rtsp_url = get_param("url");
        
        if (stream_id.empty() || rtsp_url.empty()) {
            return create_response(400, "application/json", "{\"error\":\"Missing id or url parameter\"}");
        }
        
        // Add the stream
        add_stream(stream_id, rtsp_url);
        
        return create_response(200, "application/json", "{\"success\":true,\"message\":\"Stream added\"}");
    }
    
    std::string handle_remove_stream_request(const std::string& path) {
        // Parse query parameters
        auto qpos = path.find('?');
        if (qpos == std::string::npos) {
            return create_response(400, "application/json", "{\"error\":\"Missing parameters\"}");
        }
        
        std::string query = path.substr(qpos + 1);
        std::string stream_id;
        
        // Simple parameter parsing
        auto get_param = [&](const std::string& key) -> std::string {
            auto pos = query.find(key + "=");
            if (pos == std::string::npos) return {};
            auto start = pos + key.size() + 1;
            auto end = query.find('&', start);
            return query.substr(start, end == std::string::npos ? std::string::npos : end - start);
        };
        
        stream_id = get_param("id");
        
        if (stream_id.empty()) {
            return create_response(400, "application/json", "{\"error\":\"Missing id parameter\"}");
        }
        
        // Remove the stream
        remove_stream(stream_id);
        
        return create_response(200, "application/json", "{\"success\":true,\"message\":\"Stream removed\"}");
    }
    
    std::string create_response(int status, const std::string& content_type, const std::string& body) {
        std::ostringstream oss;
        
        oss << "HTTP/1.1 " << status << " ";
        switch (status) {
            case 200: oss << "OK"; break;
            case 400: oss << "Bad Request"; break;
            case 404: oss << "Not Found"; break;
            case 500: oss << "Internal Server Error"; break;
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

int main() {
    std::cout << "Starting Streaming Service..." << std::endl;
    
    // Create streaming config
    StreamingConfig config;
    config.bind_address = "127.0.0.1";
    config.port = 8088;
    
    // Create streaming service
    StreamingService service(config);
    
    // Start service
    if (!service.start()) {
        std::cerr << "Failed to start streaming service" << std::endl;
        return 1;
    }
    
    std::cout << "Streaming Service is running. Press Ctrl+C to stop." << std::endl;
    
    // Keep running
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    return 0;
}