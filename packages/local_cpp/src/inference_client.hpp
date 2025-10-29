#pragma once

#include <curl/curl.h>
#include <string>
#include <atomic>
#include <chrono>
#include <functional>
#include "frame.hpp"
#include "config.hpp"

class InferenceClient {
public:
    explicit InferenceClient(const AIServiceConfig& config);
    ~InferenceClient();
    
    // Perform inference on a frame
    bool infer(const Frame& frame, InferenceResult& result);
    
    // Check if AI service is healthy
    bool is_healthy() const { return healthy_.load(); }
    
    // Get connection statistics
    struct Stats {
        uint64_t total_requests = 0;
        uint64_t successful_requests = 0;
        uint64_t failed_requests = 0;
        double average_latency_ms = 0.0;
        std::chrono::system_clock::time_point last_success;
        std::chrono::system_clock::time_point last_failure;
    };
    
    Stats get_stats() const { return stats_; }
    
    // Set callbacks
    void set_error_callback(std::function<void(const std::string&)> callback) {
        error_callback_ = std::move(callback);
    }
    
    void set_health_callback(std::function<void(bool)> callback) {
        health_callback_ = std::move(callback);
    }

private:
    AIServiceConfig config_;
    CURL* curl_;
    std::atomic<bool> healthy_{true};
    Stats stats_;
    
    std::function<void(const std::string&)> error_callback_;
    std::function<void(bool)> health_callback_;
    
    // HTTP response handling
    struct ResponseData {
        std::string data;
        long status_code = 0;
    };
    
    static size_t write_callback(char* ptr, size_t size, size_t nmemb, void* userdata);
    static size_t header_callback(char* buffer, size_t size, size_t nitems, void* userdata);
    
    // Retry logic
    bool perform_request_with_retry(const Frame& frame, InferenceResult& result);
    bool perform_single_request(const Frame& frame, InferenceResult& result);
    
    // Health check
    bool check_health();
    void update_health_status(bool healthy);
    
    // Statistics
    void update_stats(bool success, double latency_ms);
    
    // JSON parsing
    bool parse_response(const std::string& json_response, InferenceResult& result);
    
    // Image encoding
    std::vector<uchar> encode_image_jpeg(const cv::Mat& image, int quality = 95);
};




#include <curl/curl.h>
#include <string>
#include <atomic>
#include <chrono>
#include <functional>
#include "frame.hpp"
#include "config.hpp"

class InferenceClient {
public:
    explicit InferenceClient(const AIServiceConfig& config);
    ~InferenceClient();
    
    // Perform inference on a frame
    bool infer(const Frame& frame, InferenceResult& result);
    
    // Check if AI service is healthy
    bool is_healthy() const { return healthy_.load(); }
    
    // Get connection statistics
    struct Stats {
        uint64_t total_requests = 0;
        uint64_t successful_requests = 0;
        uint64_t failed_requests = 0;
        double average_latency_ms = 0.0;
        std::chrono::system_clock::time_point last_success;
        std::chrono::system_clock::time_point last_failure;
    };
    
    Stats get_stats() const { return stats_; }
    
    // Set callbacks
    void set_error_callback(std::function<void(const std::string&)> callback) {
        error_callback_ = std::move(callback);
    }
    
    void set_health_callback(std::function<void(bool)> callback) {
        health_callback_ = std::move(callback);
    }

private:
    AIServiceConfig config_;
    CURL* curl_;
    std::atomic<bool> healthy_{true};
    Stats stats_;
    
    std::function<void(const std::string&)> error_callback_;
    std::function<void(bool)> health_callback_;
    
    // HTTP response handling
    struct ResponseData {
        std::string data;
        long status_code = 0;
    };
    
    static size_t write_callback(char* ptr, size_t size, size_t nmemb, void* userdata);
    static size_t header_callback(char* buffer, size_t size, size_t nitems, void* userdata);
    
    // Retry logic
    bool perform_request_with_retry(const Frame& frame, InferenceResult& result);
    bool perform_single_request(const Frame& frame, InferenceResult& result);
    
    // Health check
    bool check_health();
    void update_health_status(bool healthy);
    
    // Statistics
    void update_stats(bool success, double latency_ms);
    
    // JSON parsing
    bool parse_response(const std::string& json_response, InferenceResult& result);
    
    // Image encoding
    std::vector<uchar> encode_image_jpeg(const cv::Mat& image, int quality = 95);
};


