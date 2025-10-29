#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include <atomic>
#include "frame.hpp"
#include "config.hpp"

class Logger {
public:
    explicit Logger(const LoggingConfig& config);
    ~Logger();
    
    // Log inference result
    void log_inference(const InferenceResult& result);
    
    // Log frame processing info
    void log_frame(const Frame& frame, const std::string& status);
    
    // Log error
    void log_error(const std::string& error);
    
    // Log info
    void log_info(const std::string& message);
    
    // Log warning
    void log_warning(const std::string& message);
    
    // Log debug
    void log_debug(const std::string& message);
    
    // Set log level
    void set_level(const std::string& level);
    
    // Check if should log at given level
    bool should_log(const std::string& level) const;

private:
    LoggingConfig config_;
    std::ofstream log_file_;
    std::mutex log_mutex_;
    std::atomic<int> log_level_{2}; // 0=DEBUG, 1=INFO, 2=WARNING, 3=ERROR
    
    // Log rotation
    std::string current_log_file_;
    std::chrono::system_clock::time_point last_rotation_;
    
    void write_log(const std::string& level, const std::string& message);
    void rotate_log_if_needed();
    std::string get_timestamp() const;
    std::string format_inference_result(const InferenceResult& result) const;
    std::string format_frame_info(const Frame& frame, const std::string& status) const;
    int level_to_int(const std::string& level) const;
};




#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include <atomic>
#include "frame.hpp"
#include "config.hpp"

class Logger {
public:
    explicit Logger(const LoggingConfig& config);
    ~Logger();
    
    // Log inference result
    void log_inference(const InferenceResult& result);
    
    // Log frame processing info
    void log_frame(const Frame& frame, const std::string& status);
    
    // Log error
    void log_error(const std::string& error);
    
    // Log info
    void log_info(const std::string& message);
    
    // Log warning
    void log_warning(const std::string& message);
    
    // Log debug
    void log_debug(const std::string& message);
    
    // Set log level
    void set_level(const std::string& level);
    
    // Check if should log at given level
    bool should_log(const std::string& level) const;

private:
    LoggingConfig config_;
    std::ofstream log_file_;
    std::mutex log_mutex_;
    std::atomic<int> log_level_{2}; // 0=DEBUG, 1=INFO, 2=WARNING, 3=ERROR
    
    // Log rotation
    std::string current_log_file_;
    std::chrono::system_clock::time_point last_rotation_;
    
    void write_log(const std::string& level, const std::string& message);
    void rotate_log_if_needed();
    std::string get_timestamp() const;
    std::string format_inference_result(const InferenceResult& result) const;
    std::string format_frame_info(const Frame& frame, const std::string& status) const;
    int level_to_int(const std::string& level) const;
};


