#pragma once

#include <string>
#include <chrono>
#include <atomic>
#include <thread>
#include <functional>
#include <nlohmann/json.hpp>

struct StreamConfig {
    std::string url;
    std::string camera_id;
    int fps_cap = 15;
    int reconnect_delay_ms = 1000;
    int max_reconnect_attempts = -1; // -1 = infinite
    bool use_hardware_decode = true;
};

struct AIServiceConfig {
    std::string host = "http://127.0.0.1:8000";
    int timeout_ms = 5000;
    int retry_count = 3;
    int retry_delay_ms = 1000;
};

struct PipelineConfig {
    size_t queue_size = 32;
    std::string drop_policy = "drop_oldest"; // "drop_oldest", "drop_newest", "block"
    int max_inference_queue = 16;
};

struct PreprocessingConfig {
    int target_width = 1280;
    int target_height = 720;
    bool letterbox = true;
    double gamma = 1.0;
    bool denoise = false;
    bool sharpen = false;
    double quality_threshold = 0.3; // Skip frames below this quality
};

struct PrivacyConfig {
    bool mask_plate_on_storage = false;
    bool anonymize = false;
    bool store_original_image = true;
};

struct LoggingConfig {
    std::string level = "INFO";
    std::string file = "logs/cpp_client.log";
    bool rotate_daily = true;
    bool console_output = true;
};

struct HealthConfig {
    int port = 8085;
    std::string bind_address = "0.0.0.0";
    int metrics_interval_ms = 1000;
};

class Config {
public:
    StreamConfig stream;
    AIServiceConfig ai_service;
    PipelineConfig pipeline;
    PreprocessingConfig preprocessing;
    PrivacyConfig privacy;
    LoggingConfig logging;
    HealthConfig health;
    
    // Hot reload support
    std::atomic<bool> config_changed{false};
    std::function<void()> on_config_changed;
    
    Config();
    // Custom copy to avoid copying atomics/threads state
    Config(const Config& other);
    Config& operator=(const Config& other);
    ~Config();
    
    bool load(const std::string& filepath);
    bool save(const std::string& filepath) const;
    void start_watch(const std::string& filepath);
    void stop_watch();
    
    // Convert to/from JSON
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
    
private:
    std::thread watch_thread_;
    std::atomic<bool> watching_{false};
    std::string config_file_;
    std::filesystem::file_time_type last_modified_;
    
    void watch_loop();
    bool check_file_changed();
};

#include <string>
#include <chrono>
#include <atomic>
#include <thread>
#include <functional>
#include <nlohmann/json.hpp>

struct StreamConfig {
    std::string url;
    std::string camera_id;
    int fps_cap = 15;
    int reconnect_delay_ms = 1000;
    int max_reconnect_attempts = -1; // -1 = infinite
    bool use_hardware_decode = true;
};

struct AIServiceConfig {
    std::string host = "http://127.0.0.1:8000";
    int timeout_ms = 5000;
    int retry_count = 3;
    int retry_delay_ms = 1000;
};

struct PipelineConfig {
    size_t queue_size = 32;
    std::string drop_policy = "drop_oldest"; // "drop_oldest", "drop_newest", "block"
    int max_inference_queue = 16;
};

struct PreprocessingConfig {
    int target_width = 1280;
    int target_height = 720;
    bool letterbox = true;
    double gamma = 1.0;
    bool denoise = false;
    bool sharpen = false;
    double quality_threshold = 0.3; // Skip frames below this quality
};

struct PrivacyConfig {
    bool mask_plate_on_storage = false;
    bool anonymize = false;
    bool store_original_image = true;
};

struct LoggingConfig {
    std::string level = "INFO";
    std::string file = "logs/cpp_client.log";
    bool rotate_daily = true;
    bool console_output = true;
};

struct HealthConfig {
    int port = 8085;
    std::string bind_address = "0.0.0.0";
    int metrics_interval_ms = 1000;
};

class Config {
public:
    StreamConfig stream;
    AIServiceConfig ai_service;
    PipelineConfig pipeline;
    PreprocessingConfig preprocessing;
    PrivacyConfig privacy;
    LoggingConfig logging;
    HealthConfig health;
    
    // Hot reload support
    std::atomic<bool> config_changed{false};
    std::function<void()> on_config_changed;
    
    Config();
    // Custom copy to avoid copying atomics/threads state
    Config(const Config& other);
    Config& operator=(const Config& other);
    ~Config();
    
    bool load(const std::string& filepath);
    bool save(const std::string& filepath) const;
    void start_watch(const std::string& filepath);
    void stop_watch();
    
    // Convert to/from JSON
    nlohmann::json to_json() const;
    void from_json(const nlohmann::json& j);
    
private:
    std::thread watch_thread_;
    std::atomic<bool> watching_{false};
    std::string config_file_;
    std::filesystem::file_time_type last_modified_;
    
    void watch_loop();
    bool check_file_changed();
};
