#pragma once

#include <string>
#include <atomic>
#include <thread>
#include <chrono>
#include <functional>
#include <map>
#include "config.hpp"

// Simple HTTP server for health checks and metrics
class HealthServer {
public:
    explicit HealthServer(const HealthConfig& config);
    ~HealthServer();
    
    // Start/stop server
    bool start();
    void stop();
    
    // Check if running
    bool is_running() const { return running_.load(); }
    
    // Update metrics
    void update_metrics(const std::map<std::string, double>& metrics);
    void set_ai_healthy(bool healthy);
    void set_stream_connected(bool connected);
    void set_fps(double fps);
    void set_queue_size(size_t size);
    
    // Set custom health check
    void set_health_check(std::function<bool()> check) {
        health_check_ = std::move(check);
    }

    // Discovery handler: accepts full path (may include query) and returns JSON string
    void set_discover_handler(std::function<std::string(const std::string&)> handler) {
        discover_handler_ = std::move(handler);
    }

private:
    HealthConfig config_;
    std::atomic<bool> running_{false};
    std::thread server_thread_;
    
    // Metrics
    std::atomic<bool> ai_healthy_{true};
    std::atomic<bool> stream_connected_{false};
    std::atomic<double> current_fps_{0.0};
    std::atomic<size_t> queue_size_{0};
    std::map<std::string, double> custom_metrics_;
    std::chrono::system_clock::time_point last_update_;
    
    std::function<bool()> health_check_;
    std::function<std::string(const std::string&)> discover_handler_;
    
    void server_loop();
    std::string handle_request(const std::string& method, const std::string& path);
    std::string get_health_response();
    std::string get_metrics_response();
    std::string get_status_response();
    
    // Simple HTTP parsing
    struct HttpRequest {
        std::string method;
        std::string path;
        std::string version;
    };
    
    HttpRequest parse_request(const std::string& request);
    std::string create_response(int status, const std::string& content_type, const std::string& body);
};


