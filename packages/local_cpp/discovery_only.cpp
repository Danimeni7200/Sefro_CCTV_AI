#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <functional>
#include <sstream>
#include <cstring>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <opencv2/opencv.hpp>
#include <map>
#include <memory>
#include <mutex>
#pragma comment(lib, "ws2_32.lib")

// Forward declarations for stream reader functionality
struct StreamConfig {
    std::string url;
    std::string camera_id;
    int fps_cap = 15;
    bool use_hardware_decode = true;
    int max_reconnect_attempts = 5;
    int reconnect_delay_ms = 2000;
};

class Frame {
public:
    Frame(cv::Mat mat, const std::string& camera_id, uint64_t frame_id) 
        : mat_(std::move(mat)), camera_id_(camera_id), frame_id_(frame_id) {
        timestamp_ = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
    
    const cv::Mat& get_mat() const { return mat_; }
    const std::string& get_camera_id() const { return camera_id_; }
    uint64_t get_frame_id() const { return frame_id_; }
    uint64_t get_timestamp() const { return timestamp_; }
    
private:
    cv::Mat mat_;
    std::string camera_id_;
    uint64_t frame_id_;
    uint64_t timestamp_;
};

class StreamReader {
public:
    StreamReader(const StreamConfig& config) : config_(config) {}
    ~StreamReader() { stop(); }
    
    bool start() {
        if (running_.load()) return true;
        running_ = true;
        capture_thread_ = std::thread(&StreamReader::capture_loop, this);
        return true;
    }
    
    void stop() {
        if (!running_.load()) return;
        running_ = false;
        if (capture_thread_.joinable()) capture_thread_.join();
    }
    
    bool is_connected() const { return connected_.load(); }
    
    void set_frame_callback(std::function<void(Frame&&)> callback) {
        frame_callback_ = std::move(callback);
    }
    
private:
    StreamConfig config_;
    cv::VideoCapture cap_;
    std::atomic<bool> running_{false};
    std::atomic<bool> connected_{false};
    std::thread capture_thread_;
    std::function<void(Frame&&)> frame_callback_;
    std::mutex frames_mutex_;
    std::map<std::string, Frame> latest_frames_;
    
    void capture_loop() {
        std::cout << "Capture loop started for: " << config_.camera_id << std::endl;
        if (!connect()) {
            std::cerr << "Failed to connect to stream: " << config_.url << std::endl;
            return;
        }
        
        std::cout << "Connected, starting frame capture..." << std::endl;
        int frame_count = 0;
        
        while (running_.load() && connected_.load()) {
            cv::Mat frame;
            if (!cap_.read(frame) || frame.empty()) {
                std::cerr << "Failed to read frame from: " << config_.url << std::endl;
                connected_ = false;
                continue;
            }
            
            frame_count++;
            if (frame_count % 30 == 0) {
                std::cout << "Captured " << frame_count << " frames from " << config_.camera_id << std::endl;
            }
            
            Frame frame_obj(std::move(frame), config_.camera_id, 0);
            if (frame_callback_) {
                frame_callback_(std::move(frame_obj));
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(1000 / config_.fps_cap));
        }
        
        std::cout << "Capture loop ended for: " << config_.camera_id << std::endl;
    }
    
    bool connect() {
        try {
            // Try to open with FFMPEG backend first
            cv::VideoCapture cap_ff;
            cap_ff.open(config_.url, cv::CAP_FFMPEG);
            
            if (cap_ff.isOpened()) {
                std::cout << "✓ Opened RTSP stream with FFMPEG backend" << std::endl;
                cap_ = cap_ff;
                cap_.set(cv::CAP_PROP_BUFFERSIZE, 1);
                connected_ = true;
                return true;
            }
            
            // Fallback to default backend
            cap_.open(config_.url);
            if (cap_.isOpened()) {
                std::cout << "✓ Opened RTSP stream with default backend" << std::endl;
                cap_.set(cv::CAP_PROP_BUFFERSIZE, 1);
                connected_ = true;
                return true;
            }
            
            std::cerr << "✗ Failed to open stream: " << config_.url << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Exception opening stream: " << e.what() << std::endl;
        }
        return false;
    }
};

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
    
    // Add a stream to be served
    void add_stream(const std::string& stream_id, const std::string& rtsp_url) {
        std::lock_guard<std::mutex> lock(streams_mutex_);
        
        // Create stream config
        StreamConfig config;
        config.url = rtsp_url;
        config.camera_id = stream_id;
        config.fps_cap = 15;
        
        // Create stream reader
        auto reader = std::make_shared<StreamReader>(config);
        
        // Set frame callback to store latest frame
        reader->set_frame_callback([this, stream_id](Frame&& frame) {
            std::lock_guard<std::mutex> lock(frames_mutex_);
            // Avoid requiring a default constructor for Frame
            latest_frames_.insert_or_assign(stream_id, std::move(frame));
        });
        
        // Start the reader
        std::cout << "Attempting to start stream: " << stream_id << std::endl;
        if (reader->start()) {
            streams_[stream_id] = reader;
            std::cout << "✓ Added stream: " << stream_id << " -> " << rtsp_url << std::endl;
            
            // Initialize an empty frame entry so the endpoint doesn't return 404
            std::lock_guard<std::mutex> lock(frames_mutex_);
            // We'll add this when a frame is captured
        } else {
            std::cerr << "✗ Failed to start stream: " << stream_id << std::endl;
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
    HealthConfig config_;
    std::atomic<bool> running_{false};
    std::thread server_thread_;
    std::function<std::string(const std::string&)> discover_handler_;
    
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
        // Parse the HTTP request properly
        std::istringstream iss(request);
        std::string line;
        std::getline(iss, line);
        
        // Parse the request line (e.g., "POST /add_stream?id=... HTTP/1.1")
        std::istringstream line_stream(line);
        std::string method, path, version;
        line_stream >> method >> path >> version;
        
        // Parse query parameters from the path
        std::string query;
        auto qpos = path.find('?');
        if (qpos != std::string::npos) {
            query = path.substr(qpos + 1);
            path = path.substr(0, qpos);
        }
        
        std::cout << "Request: " << method << " " << path << std::endl;
        
        if (path == "/health") {
            return create_response(200, "application/json", "{\"status\":\"ok\"}");
        } else if (path.rfind("/stream/") == 0) {
            // Extract stream ID from path
            std::string stream_id = path.substr(8); // Remove "/stream/"
            return handle_stream_request(stream_id);
        } else if (path == "/add_stream" && method == "POST") {
            return handle_add_stream_request(query);
        } else if (path == "/remove_stream" && method == "POST") {
            return handle_remove_stream_request(query);
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
    
    std::string handle_stream_request(const std::string& stream_id) {
        std::lock_guard<std::mutex> lock(frames_mutex_);
        std::cout << "Attempting to fetch stream: " << stream_id << std::endl;
        std::cout << "Total streams in latest_frames_: " << latest_frames_.size() << std::endl;
        
        auto it = latest_frames_.find(stream_id);
        if (it == latest_frames_.end()) {
            std::cout << "Stream " << stream_id << " not found in latest_frames_" << std::endl;
            // List all available streams
            std::string available = "";
            for (const auto& [key, val] : latest_frames_) {
                if (!available.empty()) available += ", ";
                available += key;
            }
            return create_response(404, "application/json", "{\"error\":\"Stream not found\",\"available_streams\":[\"" + available + "\"]}");
        }
        
        std::cout << "Found stream " << stream_id << ", returning status" << std::endl;
        return create_response(200, "application/json", "{\"status\":\"stream exists\",\"stream_id\":\"" + stream_id + "\"}");
    }
    
    std::string handle_add_stream_request(const std::string& query_string) {
        // Parse query parameters (already extracted from path)
        if (query_string.empty()) {
            return create_response(400, "application/json", "{\"error\":\"Missing parameters\"}");
        }
        
        std::string stream_id, rtsp_url;
        
        // Simple parameter parsing
        auto get_param = [&](const std::string& key) -> std::string {
            auto pos = query_string.find(key + "=");
            if (pos == std::string::npos) return {};
            auto start = pos + key.size() + 1;
            auto end = query_string.find('&', start);
            return query_string.substr(start, end == std::string::npos ? std::string::npos : end - start);
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
    
    std::string handle_remove_stream_request(const std::string& query_string) {
        // Parse query parameters (already extracted from path)
        if (query_string.empty()) {
            return create_response(400, "application/json", "{\"error\":\"Missing parameters\"}");
        }
        
        std::string stream_id;
        
        // Simple parameter parsing
        auto get_param = [&](const std::string& key) -> std::string {
            auto pos = query_string.find(key + "=");
            if (pos == std::string::npos) return {};
            auto start = pos + key.size() + 1;
            auto end = query_string.find('&', start);
            return query_string.substr(start, end == std::string::npos ? std::string::npos : end - start);
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
            case 501: oss << "Not Implemented"; break;
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

// Discovery handler function
std::string discovery_handler(const std::string& query) {
    // Parse query parameters
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
    
    // Build candidate RTSP URLs (brand-first)
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
    
    // Common fallbacks
    add("stream1");
    add("live/ch00_0");
    add("avstream/channel=1");
    
    // Build JSON response
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
    
    // Create health config
    HealthConfig config;
    config.bind_address = "127.0.0.1";
    config.port = 8086;
    
    // Create health server
    HealthServer server(config);
    
    // Set discovery handler
    server.set_discover_handler(discovery_handler);
    
    // Start server
    if (!server.start()) {
        std::cerr << "Failed to start discovery service" << std::endl;
        return 1;
    }
    
    std::cout << "C++ Discovery & Streaming Service is running. Press Ctrl+C to stop." << std::endl;
    
    // Keep running
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    return 0;
}