#pragma once

#include <atomic>
#include <thread>
#include <memory>
#include <chrono>
#include "config.hpp"
#include "stream_reader.hpp"
#include "ring_buffer.hpp"
#include "preprocessor.hpp"
#include "inference_client.hpp"
#include "logger.hpp"
#include "health_server.hpp"
#include "frame.hpp"

class Pipeline {
public:
    explicit Pipeline(const Config& config);
    ~Pipeline();
    
    // Start/stop pipeline
    bool start();
    void stop();
    
    // Check if running
    bool is_running() const { return running_.load(); }
    
    // Get statistics
    struct Stats {
        uint64_t frames_processed = 0;
        uint64_t frames_dropped = 0;
        uint64_t inferences_successful = 0;
        uint64_t inferences_failed = 0;
        double current_fps = 0.0;
        double average_latency_ms = 0.0;
        std::chrono::system_clock::time_point start_time;
    };
    
    Stats get_stats() const;

private:
    Config config_;
    std::atomic<bool> running_{false};
    
    // Components
    std::unique_ptr<StreamReader> stream_reader_;
    std::unique_ptr<RingBuffer<Frame>> frame_buffer_;
    std::unique_ptr<RingBuffer<Frame>> inference_buffer_;
    std::unique_ptr<Preprocessor> preprocessor_;
    std::unique_ptr<InferenceClient> inference_client_;
    std::unique_ptr<Logger> logger_;
    std::unique_ptr<HealthServer> health_server_;
    
    // Threads
    std::thread preprocess_thread_;
    std::thread inference_thread_;
    std::thread metrics_thread_;
    
    // Statistics
    mutable std::mutex stats_mutex_;
    Stats stats_;
    std::chrono::system_clock::time_point last_stats_update_;
    
    // Callbacks
    void on_frame_received(Frame&& frame);
    void on_stream_error(const std::string& error);
    void on_ai_health_changed(bool healthy);
    void on_inference_error(const std::string& error);
    
    // Thread functions
    void preprocess_loop();
    void inference_loop();
    void metrics_loop();
    
    // Statistics
    void update_stats();
    void log_periodic_stats();
    
    // Graceful shutdown
    void signal_handler(int signal);
    static std::atomic<bool> shutdown_requested_;
};




#include <atomic>
#include <thread>
#include <memory>
#include <chrono>
#include "config.hpp"
#include "stream_reader.hpp"
#include "ring_buffer.hpp"
#include "preprocessor.hpp"
#include "inference_client.hpp"
#include "logger.hpp"
#include "health_server.hpp"
#include "frame.hpp"

class Pipeline {
public:
    explicit Pipeline(const Config& config);
    ~Pipeline();
    
    // Start/stop pipeline
    bool start();
    void stop();
    
    // Check if running
    bool is_running() const { return running_.load(); }
    
    // Get statistics
    struct Stats {
        uint64_t frames_processed = 0;
        uint64_t frames_dropped = 0;
        uint64_t inferences_successful = 0;
        uint64_t inferences_failed = 0;
        double current_fps = 0.0;
        double average_latency_ms = 0.0;
        std::chrono::system_clock::time_point start_time;
    };
    
    Stats get_stats() const;

private:
    Config config_;
    std::atomic<bool> running_{false};
    
    // Components
    std::unique_ptr<StreamReader> stream_reader_;
    std::unique_ptr<RingBuffer<Frame>> frame_buffer_;
    std::unique_ptr<RingBuffer<Frame>> inference_buffer_;
    std::unique_ptr<Preprocessor> preprocessor_;
    std::unique_ptr<InferenceClient> inference_client_;
    std::unique_ptr<Logger> logger_;
    std::unique_ptr<HealthServer> health_server_;
    
    // Threads
    std::thread preprocess_thread_;
    std::thread inference_thread_;
    std::thread metrics_thread_;
    
    // Statistics
    mutable std::mutex stats_mutex_;
    Stats stats_;
    std::chrono::system_clock::time_point last_stats_update_;
    
    // Callbacks
    void on_frame_received(Frame&& frame);
    void on_stream_error(const std::string& error);
    void on_ai_health_changed(bool healthy);
    void on_inference_error(const std::string& error);
    
    // Thread functions
    void preprocess_loop();
    void inference_loop();
    void metrics_loop();
    
    // Statistics
    void update_stats();
    void log_periodic_stats();
    
    // Graceful shutdown
    void signal_handler(int signal);
    static std::atomic<bool> shutdown_requested_;
};


