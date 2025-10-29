#include "pipeline.hpp"
#include <iostream>
#include <csignal>
#include <iomanip>
#include <chrono>

std::atomic<bool> Pipeline::shutdown_requested_{false};

Pipeline::Pipeline(const Config& config) : config_(config) {
    // Initialize components
    stream_reader_ = std::make_unique<StreamReader>(config.stream);
    frame_buffer_ = std::make_unique<RingBuffer<Frame>>(config.pipeline.queue_size);
    inference_buffer_ = std::make_unique<RingBuffer<Frame>>(config.pipeline.max_inference_queue);
    preprocessor_ = std::make_unique<Preprocessor>(config.preprocessing);
    inference_client_ = std::make_unique<InferenceClient>(config.ai_service);
    logger_ = std::make_unique<Logger>(config.logging);
    health_server_ = std::make_unique<HealthServer>(config.health);
    
    // Set up callbacks
    stream_reader_->set_frame_callback([this](Frame&& frame) {
        on_frame_received(std::move(frame));
    });
    
    stream_reader_->set_error_callback([this](const std::string& error) {
        on_stream_error(error);
    });
    
    inference_client_->set_health_callback([this](bool healthy) {
        on_ai_health_changed(healthy);
    });
    
    inference_client_->set_error_callback([this](const std::string& error) {
        on_inference_error(error);
    });
    
    // Set up health check: liveness OK if process is running.
    // Detailed readiness is available at /status.
    health_server_->set_health_check([this]() {
        return true; // liveness
    });

    // Set up discovery handler
    health_server_->set_discover_handler([this](const std::string& query) -> std::string {
        // parse query params: ip=...&user=...&pass=...&brand=...
        auto get = [&](const std::string& key) -> std::string {
            auto pos = query.find(key + "=");
            if (pos == std::string::npos) return {};
            auto start = pos + key.size() + 1;
            auto end = query.find('&', start);
            return query.substr(start, end == std::string::npos ? std::string::npos : end - start);
        };
        std::string ip = get("ip");
        std::string user = get("user");
        std::string pass = get("pass");
        std::string brand = get("brand");
        if (ip.empty() || user.empty()) {
            return "{\"success\":false,\"error\":\"ip and user required\"}";
        }
        // Build candidate RTSP URLs (brand-first)
        std::vector<std::string> candidates;
        auto add = [&](const std::string& path){ candidates.emplace_back("rtsp://" + user + ":" + pass + "@" + ip + ":554/" + path); };
        if (brand == "reolink") {
            add("h264Preview_01_sub");
            add("h264Preview_01_main");
        }
        // Common fallbacks
        add("Streaming/Channels/101");
        add("Streaming/Channels/102");
        add("h264");
        add("live/ch00_0");
        add("avstream/channel=1");

        // Try to swap into config and test quickly by attempting to open via StreamReader connect()
        // We can't block the running reader; so return the first candidate. The operator can save/apply.
        // For now, return the list for the UI to try sequentially.
        std::string json = "{\"success\":true,\"candidates\":[";
        for (size_t i=0;i<candidates.size();++i){
            json += "\"" + candidates[i] + "\"";
            if (i+1<candidates.size()) json += ",";
        }
        json += "]}";
        return json;
    });
    
    // Set up signal handlers
    std::signal(SIGINT, [](int signal) {
        std::cout << "\nReceived SIGINT, shutting down gracefully..." << std::endl;
        shutdown_requested_ = true;
    });
    
    std::signal(SIGTERM, [](int signal) {
        std::cout << "\nReceived SIGTERM, shutting down gracefully..." << std::endl;
        shutdown_requested_ = true;
    });
    
    stats_.start_time = std::chrono::system_clock::now();
    last_stats_update_ = stats_.start_time;
}

Pipeline::~Pipeline() {
    stop();
}

bool Pipeline::start() {
    if (running_.load()) {
        return true;
    }
    
    std::cout << "Starting LPR pipeline..." << std::endl;
    
    // Start health server first
    if (!health_server_->start()) {
        std::cerr << "Failed to start health server" << std::endl;
        return false;
    }
    
    // Start stream reader
    if (!stream_reader_->start()) {
        std::cerr << "Failed to start stream reader" << std::endl;
        health_server_->stop();
        return false;
    }
    
    // Start processing threads
    running_ = true;
    preprocess_thread_ = std::thread(&Pipeline::preprocess_loop, this);
    inference_thread_ = std::thread(&Pipeline::inference_loop, this);
    metrics_thread_ = std::thread(&Pipeline::metrics_loop, this);
    
    logger_->log_info("Pipeline started successfully");
    std::cout << "Pipeline started successfully" << std::endl;
    
    return true;
}

void Pipeline::stop() {
    if (!running_.load()) {
        return;
    }
    
    std::cout << "Stopping pipeline..." << std::endl;
    running_ = false;
    
    // Stop stream reader
    stream_reader_->stop();
    
    // Wait for threads to finish
    if (preprocess_thread_.joinable()) {
        preprocess_thread_.join();
    }
    if (inference_thread_.joinable()) {
        inference_thread_.join();
    }
    if (metrics_thread_.joinable()) {
        metrics_thread_.join();
    }
    
    // Stop health server
    health_server_->stop();
    
    logger_->log_info("Pipeline stopped");
    std::cout << "Pipeline stopped" << std::endl;
}

void Pipeline::on_frame_received(Frame&& frame) {
    if (!running_.load()) return;
    
    // Try to push frame to buffer
    if (!frame_buffer_->try_push(std::move(frame))) {
        // Handle backpressure
        if (config_.pipeline.drop_policy == "drop_oldest") {
            frame_buffer_->drop_oldest();
            if (frame_buffer_->try_push(std::move(frame))) {
                stats_.frames_dropped++;
            }
        } else if (config_.pipeline.drop_policy == "drop_newest") {
            // Just drop the new frame
            stats_.frames_dropped++;
        }
        // For "block" policy, we just lose the frame (not ideal for real-time)
    }
    
    logger_->log_frame(frame, "received");
}

void Pipeline::on_stream_error(const std::string& error) {
    logger_->log_error("Stream error: " + error);
    health_server_->set_stream_connected(false);
}

void Pipeline::on_ai_health_changed(bool healthy) {
    health_server_->set_ai_healthy(healthy);
    if (healthy) {
        logger_->log_info("AI service is healthy");
    } else {
        logger_->log_warning("AI service is unhealthy");
    }
}

void Pipeline::on_inference_error(const std::string& error) {
    logger_->log_error("Inference error: " + error);
}

void Pipeline::preprocess_loop() {
    logger_->log_info("Preprocessing thread started");
    
    while (running_.load() || !frame_buffer_->empty()) {
        Frame frame;
        if (frame_buffer_->pop(frame, std::chrono::milliseconds(100))) {
            // Process frame
            Frame processed = preprocessor_->process(std::move(frame));
            
            if (!processed.image.empty()) {
                // Push to inference queue
                if (!inference_buffer_->try_push(std::move(processed))) {
                    // Drop if inference queue is full
                    stats_.frames_dropped++;
                }
                
                stats_.frames_processed++;
                logger_->log_frame(processed, "preprocessed");
            } else {
                // Frame was rejected by preprocessor
                stats_.frames_dropped++;
                logger_->log_frame(processed, "rejected");
            }
        }
    }
    
    logger_->log_info("Preprocessing thread stopped");
}

void Pipeline::inference_loop() {
    logger_->log_info("Inference thread started");
    
    while (running_.load() || !inference_buffer_->empty()) {
        Frame frame;
        if (inference_buffer_->pop(frame, std::chrono::milliseconds(100))) {
            InferenceResult result;
            
            auto start_time = std::chrono::high_resolution_clock::now();
            bool success = inference_client_->infer(frame, result);
            auto end_time = std::chrono::high_resolution_clock::now();
            
            double latency_ms = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(
                end_time - start_time).count();
            
            if (success) {
                stats_.inferences_successful++;
                stats_.average_latency_ms = (stats_.average_latency_ms * (stats_.inferences_successful - 1) + 
                                           latency_ms) / stats_.inferences_successful;
                
                logger_->log_inference(result);
            } else {
                stats_.inferences_failed++;
                logger_->log_error("Inference failed for frame " + std::to_string(frame.frame_id));
            }
        }
    }
    
    logger_->log_info("Inference thread stopped");
}

void Pipeline::metrics_loop() {
    logger_->log_info("Metrics thread started");
    
    while (running_.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(config_.health.metrics_interval_ms));
        
        update_stats();
        log_periodic_stats();
        
        // Update health server metrics
        health_server_->set_fps(stats_.current_fps);
        health_server_->set_queue_size(frame_buffer_->size() + inference_buffer_->size());
        health_server_->set_stream_connected(stream_reader_->is_connected());
        health_server_->set_ai_healthy(inference_client_->is_healthy());
        
        // Check for shutdown request
        if (shutdown_requested_.load()) {
            running_ = false;
            break;
        }
    }
    
    logger_->log_info("Metrics thread stopped");
}

void Pipeline::update_stats() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_stats_update_).count();
    
    if (elapsed > 0) {
        stats_.current_fps = static_cast<double>(stats_.frames_processed) / elapsed;
        last_stats_update_ = now;
    }
}

void Pipeline::log_periodic_stats() {
    auto stats = get_stats();
    
    std::ostringstream oss;
    oss << "Stats - FPS: " << std::fixed << std::setprecision(2) << stats.current_fps
        << ", Processed: " << stats.frames_processed
        << ", Dropped: " << stats.frames_dropped
        << ", Inferences: " << stats.inferences_successful << "/" << (stats.inferences_successful + stats.inferences_failed)
        << ", Avg Latency: " << std::fixed << std::setprecision(1) << stats.average_latency_ms << "ms";
    
    logger_->log_info(oss.str());
}

Pipeline::Stats Pipeline::get_stats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

#include <csignal>
#include <iomanip>
#include <chrono>

std::atomic<bool> Pipeline::shutdown_requested_{false};

Pipeline::Pipeline(const Config& config) : config_(config) {
    // Initialize components
    stream_reader_ = std::make_unique<StreamReader>(config.stream);
    frame_buffer_ = std::make_unique<RingBuffer<Frame>>(config.pipeline.queue_size);
    inference_buffer_ = std::make_unique<RingBuffer<Frame>>(config.pipeline.max_inference_queue);
    preprocessor_ = std::make_unique<Preprocessor>(config.preprocessing);
    inference_client_ = std::make_unique<InferenceClient>(config.ai_service);
    logger_ = std::make_unique<Logger>(config.logging);
    health_server_ = std::make_unique<HealthServer>(config.health);
    
    // Set up callbacks
    stream_reader_->set_frame_callback([this](Frame&& frame) {
        on_frame_received(std::move(frame));
    });
    
    stream_reader_->set_error_callback([this](const std::string& error) {
        on_stream_error(error);
    });
    
    inference_client_->set_health_callback([this](bool healthy) {
        on_ai_health_changed(healthy);
    });
    
    inference_client_->set_error_callback([this](const std::string& error) {
        on_inference_error(error);
    });
    
    // Set up health check: liveness OK if process is running.
    // Detailed readiness is available at /status.
    health_server_->set_health_check([this]() {
        return true; // liveness
    });

    // Set up discovery handler
    health_server_->set_discover_handler([this](const std::string& query) -> std::string {
        // parse query params: ip=...&user=...&pass=...&brand=...
        auto get = [&](const std::string& key) -> std::string {
            auto pos = query.find(key + "=");
            if (pos == std::string::npos) return {};
            auto start = pos + key.size() + 1;
            auto end = query.find('&', start);
            return query.substr(start, end == std::string::npos ? std::string::npos : end - start);
        };
        std::string ip = get("ip");
        std::string user = get("user");
        std::string pass = get("pass");
        std::string brand = get("brand");
        if (ip.empty() || user.empty()) {
            return "{\"success\":false,\"error\":\"ip and user required\"}";
        }
        // Build candidate RTSP URLs (brand-first)
        std::vector<std::string> candidates;
        auto add = [&](const std::string& path){ candidates.emplace_back("rtsp://" + user + ":" + pass + "@" + ip + ":554/" + path); };
        if (brand == "reolink") {
            add("h264Preview_01_sub");
            add("h264Preview_01_main");
        }
        // Common fallbacks
        add("Streaming/Channels/101");
        add("Streaming/Channels/102");
        add("h264");
        add("live/ch00_0");
        add("avstream/channel=1");

        // Try to swap into config and test quickly by attempting to open via StreamReader connect()
        // We can't block the running reader; so return the first candidate. The operator can save/apply.
        // For now, return the list for the UI to try sequentially.
        std::string json = "{\"success\":true,\"candidates\":[";
        for (size_t i=0;i<candidates.size();++i){
            json += "\"" + candidates[i] + "\"";
            if (i+1<candidates.size()) json += ",";
        }
        json += "]}";
        return json;
    });
    
    // Set up signal handlers
    std::signal(SIGINT, [](int signal) {
        std::cout << "\nReceived SIGINT, shutting down gracefully..." << std::endl;
        shutdown_requested_ = true;
    });
    
    std::signal(SIGTERM, [](int signal) {
        std::cout << "\nReceived SIGTERM, shutting down gracefully..." << std::endl;
        shutdown_requested_ = true;
    });
    
    stats_.start_time = std::chrono::system_clock::now();
    last_stats_update_ = stats_.start_time;
}

Pipeline::~Pipeline() {
    stop();
}

bool Pipeline::start() {
    if (running_.load()) {
        return true;
    }
    
    std::cout << "Starting LPR pipeline..." << std::endl;
    
    // Start health server first
    if (!health_server_->start()) {
        std::cerr << "Failed to start health server" << std::endl;
        return false;
    }
    
    // Start stream reader
    if (!stream_reader_->start()) {
        std::cerr << "Failed to start stream reader" << std::endl;
        health_server_->stop();
        return false;
    }
    
    // Start processing threads
    running_ = true;
    preprocess_thread_ = std::thread(&Pipeline::preprocess_loop, this);
    inference_thread_ = std::thread(&Pipeline::inference_loop, this);
    metrics_thread_ = std::thread(&Pipeline::metrics_loop, this);
    
    logger_->log_info("Pipeline started successfully");
    std::cout << "Pipeline started successfully" << std::endl;
    
    return true;
}

void Pipeline::stop() {
    if (!running_.load()) {
        return;
    }
    
    std::cout << "Stopping pipeline..." << std::endl;
    running_ = false;
    
    // Stop stream reader
    stream_reader_->stop();
    
    // Wait for threads to finish
    if (preprocess_thread_.joinable()) {
        preprocess_thread_.join();
    }
    if (inference_thread_.joinable()) {
        inference_thread_.join();
    }
    if (metrics_thread_.joinable()) {
        metrics_thread_.join();
    }
    
    // Stop health server
    health_server_->stop();
    
    logger_->log_info("Pipeline stopped");
    std::cout << "Pipeline stopped" << std::endl;
}

void Pipeline::on_frame_received(Frame&& frame) {
    if (!running_.load()) return;
    
    // Try to push frame to buffer
    if (!frame_buffer_->try_push(std::move(frame))) {
        // Handle backpressure
        if (config_.pipeline.drop_policy == "drop_oldest") {
            frame_buffer_->drop_oldest();
            if (frame_buffer_->try_push(std::move(frame))) {
                stats_.frames_dropped++;
            }
        } else if (config_.pipeline.drop_policy == "drop_newest") {
            // Just drop the new frame
            stats_.frames_dropped++;
        }
        // For "block" policy, we just lose the frame (not ideal for real-time)
    }
    
    logger_->log_frame(frame, "received");
}

void Pipeline::on_stream_error(const std::string& error) {
    logger_->log_error("Stream error: " + error);
    health_server_->set_stream_connected(false);
}

void Pipeline::on_ai_health_changed(bool healthy) {
    health_server_->set_ai_healthy(healthy);
    if (healthy) {
        logger_->log_info("AI service is healthy");
    } else {
        logger_->log_warning("AI service is unhealthy");
    }
}

void Pipeline::on_inference_error(const std::string& error) {
    logger_->log_error("Inference error: " + error);
}

void Pipeline::preprocess_loop() {
    logger_->log_info("Preprocessing thread started");
    
    while (running_.load() || !frame_buffer_->empty()) {
        Frame frame;
        if (frame_buffer_->pop(frame, std::chrono::milliseconds(100))) {
            // Process frame
            Frame processed = preprocessor_->process(std::move(frame));
            
            if (!processed.image.empty()) {
                // Push to inference queue
                if (!inference_buffer_->try_push(std::move(processed))) {
                    // Drop if inference queue is full
                    stats_.frames_dropped++;
                }
                
                stats_.frames_processed++;
                logger_->log_frame(processed, "preprocessed");
            } else {
                // Frame was rejected by preprocessor
                stats_.frames_dropped++;
                logger_->log_frame(processed, "rejected");
            }
        }
    }
    
    logger_->log_info("Preprocessing thread stopped");
}

void Pipeline::inference_loop() {
    logger_->log_info("Inference thread started");
    
    while (running_.load() || !inference_buffer_->empty()) {
        Frame frame;
        if (inference_buffer_->pop(frame, std::chrono::milliseconds(100))) {
            InferenceResult result;
            
            auto start_time = std::chrono::high_resolution_clock::now();
            bool success = inference_client_->infer(frame, result);
            auto end_time = std::chrono::high_resolution_clock::now();
            
            double latency_ms = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(
                end_time - start_time).count();
            
            if (success) {
                stats_.inferences_successful++;
                stats_.average_latency_ms = (stats_.average_latency_ms * (stats_.inferences_successful - 1) + 
                                           latency_ms) / stats_.inferences_successful;
                
                logger_->log_inference(result);
            } else {
                stats_.inferences_failed++;
                logger_->log_error("Inference failed for frame " + std::to_string(frame.frame_id));
            }
        }
    }
    
    logger_->log_info("Inference thread stopped");
}

void Pipeline::metrics_loop() {
    logger_->log_info("Metrics thread started");
    
    while (running_.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(config_.health.metrics_interval_ms));
        
        update_stats();
        log_periodic_stats();
        
        // Update health server metrics
        health_server_->set_fps(stats_.current_fps);
        health_server_->set_queue_size(frame_buffer_->size() + inference_buffer_->size());
        health_server_->set_stream_connected(stream_reader_->is_connected());
        health_server_->set_ai_healthy(inference_client_->is_healthy());
        
        // Check for shutdown request
        if (shutdown_requested_.load()) {
            running_ = false;
            break;
        }
    }
    
    logger_->log_info("Metrics thread stopped");
}

void Pipeline::update_stats() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_stats_update_).count();
    
    if (elapsed > 0) {
        stats_.current_fps = static_cast<double>(stats_.frames_processed) / elapsed;
        last_stats_update_ = now;
    }
}

void Pipeline::log_periodic_stats() {
    auto stats = get_stats();
    
    std::ostringstream oss;
    oss << "Stats - FPS: " << std::fixed << std::setprecision(2) << stats.current_fps
        << ", Processed: " << stats.frames_processed
        << ", Dropped: " << stats.frames_dropped
        << ", Inferences: " << stats.inferences_successful << "/" << (stats.inferences_successful + stats.inferences_failed)
        << ", Avg Latency: " << std::fixed << std::setprecision(1) << stats.average_latency_ms << "ms";
    
    logger_->log_info(oss.str());
}

Pipeline::Stats Pipeline::get_stats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}
