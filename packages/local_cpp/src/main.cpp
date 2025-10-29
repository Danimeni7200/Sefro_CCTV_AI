#include <iostream>
#include <string>
#include <csignal>
#include <chrono>
#include <thread>
#include "config.hpp"
#include "pipeline.hpp"

std::atomic<bool> g_running{true};

void signal_handler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down..." << std::endl;
    g_running = false;
}

int main(int argc, char** argv) {
    // Set up signal handlers
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    
    std::cout << "LPR C++ Client v1.0" << std::endl;
    std::cout << "==================" << std::endl;
    
    // Parse command line arguments
    std::string config_file = "config.json";
    if (argc > 1) {
        config_file = argv[1];
    }
    
    std::cout << "Loading configuration from: " << config_file << std::endl;
    
    // Load configuration
    Config config;
    if (!config.load(config_file)) {
        std::cerr << "Failed to load configuration from: " << config_file << std::endl;
        std::cerr << "Creating default configuration..." << std::endl;
        
        // Set some defaults
        config.stream.url = "rtsp://admin:admin@192.168.1.100:554/stream1";
        config.stream.camera_id = "CAM01";
        config.ai_service.host = "http://127.0.0.1:8000";
        
        if (!config.save(config_file)) {
            std::cerr << "Failed to save default configuration" << std::endl;
            return 1;
        }
    }
    
    // Print configuration summary
    std::cout << "Configuration loaded:" << std::endl;
    std::cout << "  Stream URL: " << config.stream.url << std::endl;
    std::cout << "  Camera ID: " << config.stream.camera_id << std::endl;
    std::cout << "  AI Service: " << config.ai_service.host << std::endl;
    std::cout << "  Health Port: " << config.health.port << std::endl;
    std::cout << "  Queue Size: " << config.pipeline.queue_size << std::endl;
    std::cout << std::endl;
    
    // Create and start pipeline
    Pipeline pipeline(config);
    
    if (!pipeline.start()) {
        std::cerr << "Failed to start pipeline" << std::endl;
        return 1;
    }
    
    std::cout << "Pipeline started successfully!" << std::endl;
    std::cout << "Health check available at: http://localhost:" << config.health.port << "/healthz" << std::endl;
    std::cout << "Metrics available at: http://localhost:" << config.health.port << "/metrics" << std::endl;
    std::cout << "Press Ctrl+C to stop..." << std::endl;
    std::cout << std::endl;
    
    // Main loop
    while (g_running.load() && pipeline.is_running()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Check for configuration changes
        if (config.config_changed.load()) {
            std::cout << "Configuration changed, reloading..." << std::endl;
            config.config_changed = false;
            // Note: In a production system, you'd want to restart the pipeline
            // with the new configuration
        }
    }
    
    std::cout << "Shutting down pipeline..." << std::endl;
    pipeline.stop();
    
    // Print final statistics
    auto stats = pipeline.get_stats();
    std::cout << std::endl;
    std::cout << "Final Statistics:" << std::endl;
    std::cout << "  Frames Processed: " << stats.frames_processed << std::endl;
    std::cout << "  Frames Dropped: " << stats.frames_dropped << std::endl;
    std::cout << "  Successful Inferences: " << stats.inferences_successful << std::endl;
    std::cout << "  Failed Inferences: " << stats.inferences_failed << std::endl;
    std::cout << "  Average FPS: " << std::fixed << std::setprecision(2) << stats.current_fps << std::endl;
    std::cout << "  Average Latency: " << std::fixed << std::setprecision(1) << stats.average_latency_ms << "ms" << std::endl;
    
    std::cout << "Goodbye!" << std::endl;
    return 0;
}




            std::cout << "Configuration changed, reloading..." << std::endl;
            config.config_changed = false;
            // Note: In a production system, you'd want to restart the pipeline
            // with the new configuration
        }
    }
    
    std::cout << "Shutting down pipeline..." << std::endl;
    pipeline.stop();
    
    // Print final statistics
    auto stats = pipeline.get_stats();
    std::cout << std::endl;
    std::cout << "Final Statistics:" << std::endl;
    std::cout << "  Frames Processed: " << stats.frames_processed << std::endl;
    std::cout << "  Frames Dropped: " << stats.frames_dropped << std::endl;
    std::cout << "  Successful Inferences: " << stats.inferences_successful << std::endl;
    std::cout << "  Failed Inferences: " << stats.inferences_failed << std::endl;
    std::cout << "  Average FPS: " << std::fixed << std::setprecision(2) << stats.current_fps << std::endl;
    std::cout << "  Average Latency: " << std::fixed << std::setprecision(1) << stats.average_latency_ms << "ms" << std::endl;
    
    std::cout << "Goodbye!" << std::endl;
    return 0;
}



