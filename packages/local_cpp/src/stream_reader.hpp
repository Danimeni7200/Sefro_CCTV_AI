#pragma once

#include <opencv2/opencv.hpp>
#include <string>
#include <atomic>
#include <thread>
#include <chrono>
#include <functional>
#include "frame.hpp"
#include "config.hpp"

class StreamReader {
public:
    StreamReader(const StreamConfig& config);
    ~StreamReader();
    
    // Start/stop streaming
    bool start();
    void stop();
    
    // Check if currently connected
    bool is_connected() const { return connected_.load(); }
    
    // Get current FPS
    double get_current_fps() const { return current_fps_.load(); }
    
    // Get frame count
    uint64_t get_frame_count() const { return frame_count_.load(); }
    
    // Get reconnect attempts
    int get_reconnect_attempts() const { return reconnect_attempts_.load(); }
    
    // Set frame callback
    void set_frame_callback(std::function<void(Frame&&)> callback) {
        frame_callback_ = std::move(callback);
    }
    
    // Set error callback
    void set_error_callback(std::function<void(const std::string&)> callback) {
        error_callback_ = std::move(callback);
    }

private:
    StreamConfig config_;
    cv::VideoCapture cap_;
    std::atomic<bool> running_{false};
    std::atomic<bool> connected_{false};
    std::atomic<double> current_fps_{0.0};
    std::atomic<uint64_t> frame_count_{0};
    std::atomic<int> reconnect_attempts_{0};
    
    std::thread capture_thread_;
    std::chrono::steady_clock::time_point last_frame_time_;
    std::chrono::steady_clock::time_point last_fps_calc_time_;
    int fps_counter_{0};
    
    std::function<void(Frame&&)> frame_callback_;
    std::function<void(const std::string&)> error_callback_;
    
    void capture_loop();
    bool connect();
    void disconnect();
    void calculate_fps();
    void handle_error(const std::string& error);
    void reconnect_with_backoff();
    
    // Frame rate limiting
    std::chrono::steady_clock::time_point last_capture_time_;
    std::chrono::milliseconds frame_interval_;
};




#include <opencv2/opencv.hpp>
#include <string>
#include <atomic>
#include <thread>
#include <chrono>
#include <functional>
#include "frame.hpp"
#include "config.hpp"

class StreamReader {
public:
    StreamReader(const StreamConfig& config);
    ~StreamReader();
    
    // Start/stop streaming
    bool start();
    void stop();
    
    // Check if currently connected
    bool is_connected() const { return connected_.load(); }
    
    // Get current FPS
    double get_current_fps() const { return current_fps_.load(); }
    
    // Get frame count
    uint64_t get_frame_count() const { return frame_count_.load(); }
    
    // Get reconnect attempts
    int get_reconnect_attempts() const { return reconnect_attempts_.load(); }
    
    // Set frame callback
    void set_frame_callback(std::function<void(Frame&&)> callback) {
        frame_callback_ = std::move(callback);
    }
    
    // Set error callback
    void set_error_callback(std::function<void(const std::string&)> callback) {
        error_callback_ = std::move(callback);
    }

private:
    StreamConfig config_;
    cv::VideoCapture cap_;
    std::atomic<bool> running_{false};
    std::atomic<bool> connected_{false};
    std::atomic<double> current_fps_{0.0};
    std::atomic<uint64_t> frame_count_{0};
    std::atomic<int> reconnect_attempts_{0};
    
    std::thread capture_thread_;
    std::chrono::steady_clock::time_point last_frame_time_;
    std::chrono::steady_clock::time_point last_fps_calc_time_;
    int fps_counter_{0};
    
    std::function<void(Frame&&)> frame_callback_;
    std::function<void(const std::string&)> error_callback_;
    
    void capture_loop();
    bool connect();
    void disconnect();
    void calculate_fps();
    void handle_error(const std::string& error);
    void reconnect_with_backoff();
    
    // Frame rate limiting
    std::chrono::steady_clock::time_point last_capture_time_;
    std::chrono::milliseconds frame_interval_;
};


