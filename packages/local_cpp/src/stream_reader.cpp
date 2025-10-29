#include "stream_reader.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>

StreamReader::StreamReader(const StreamConfig& config) 
    : config_(config), frame_interval_(1000 / config.fps_cap) {
    last_capture_time_ = std::chrono::steady_clock::now();
    last_fps_calc_time_ = std::chrono::steady_clock::now();
    
    // Log OpenCV build info for diagnostics
    std::cout << "OpenCV version: " << CV_VERSION << std::endl;
    std::cout << "OpenCV build info: " << cv::getBuildInformation() << std::endl;
}

StreamReader::~StreamReader() {
    stop();
}

bool StreamReader::start() {
    if (running_.load()) {
        return true;
    }
    
    running_ = true;
    capture_thread_ = std::thread(&StreamReader::capture_loop, this);
    
    std::cout << "Stream reader started for: " << config_.url << std::endl;
    return true;
}

void StreamReader::stop() {
    if (!running_.load()) {
        return;
    }
    
    running_ = false;
    if (capture_thread_.joinable()) {
        capture_thread_.join();
    }
    
    disconnect();
    std::cout << "Stream reader stopped" << std::endl;
}

void StreamReader::capture_loop() {
    while (running_.load()) {
        if (!connected_.load()) {
            if (!connect()) {
                reconnect_with_backoff();
                continue;
            }
        }
        
        cv::Mat frame;
        if (!cap_.read(frame) || frame.empty()) {
            handle_error("Failed to read frame from stream");
            disconnect();
            continue;
        }
        
        // Frame rate limiting
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_capture_time_);
        
        if (elapsed < frame_interval_) {
            std::this_thread::sleep_for(frame_interval_ - elapsed);
        }
        last_capture_time_ = std::chrono::steady_clock::now();
        
        // Create frame object
        Frame frame_obj(std::move(frame), config_.camera_id, frame_count_.fetch_add(1));
        
        // Call callback if set
        if (frame_callback_) {
            frame_callback_(std::move(frame_obj));
        }
        
        // Update FPS calculation
        calculate_fps();
    }
}

bool StreamReader::connect() {
    try {
        // Validate URL format
        if (config_.url.empty()) {
            handle_error("Stream URL is empty");
            return false;
        }
        
        if (config_.url.find("rtsp://") != 0 && config_.url.find("http://") != 0 && 
            config_.url.find("https://") != 0 && config_.url.find("file://") != 0) {
            handle_error("Invalid URL scheme. Expected rtsp://, http://, https://, or file://");
            return false;
        }
        
        std::cout << "[StreamReader] Validating URL: " << config_.url << std::endl;
        
        // Try different backends in order of preference
        std::vector<std::pair<int, std::string>> backends = {
            {cv::CAP_FFMPEG, "FFMPEG"},
            {cv::CAP_GSTREAMER, "GStreamer"},
            {cv::CAP_ANY, "ANY (auto-detect)"}
        };
        
        for (const auto& [backend, backend_name] : backends) {
            std::cout << "[StreamReader] Attempting backend: " << backend_name << " (ID: " << backend << ")" << std::endl;
            
            // Attempt to open with timeout (use a separate thread to avoid hanging)
            std::atomic<bool> open_success{false};
            std::atomic<bool> open_done{false};
            
            std::thread open_thread([this, backend, &open_success, &open_done]() {
                open_success = cap_.open(config_.url, backend);
                open_done = true;
            });
            
            // Wait up to 5 seconds for the open to complete
            auto start = std::chrono::steady_clock::now();
            while (!open_done.load() && std::chrono::steady_clock::now() - start < std::chrono::seconds(5)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            
            if (open_thread.joinable()) {
                open_thread.detach(); // Detach if still running (will be cleaned up by OS)
            }
            
            if (!open_done.load()) {
                std::cout << "[StreamReader] Backend " << backend_name << " timed out (>5s)" << std::endl;
                continue;
            }
            
            if (open_success.load() && cap_.isOpened()) {
                std::cout << "[StreamReader] Successfully opened stream with backend: " << backend_name << std::endl;
                
                // Get stream properties
                double fps = cap_.get(cv::CAP_PROP_FPS);
                int width = static_cast<int>(cap_.get(cv::CAP_PROP_FRAME_WIDTH));
                int height = static_cast<int>(cap_.get(cv::CAP_PROP_FRAME_HEIGHT));
                
                std::cout << "[StreamReader] Stream properties - FPS: " << fps 
                         << ", Resolution: " << width << "x" << height << std::endl;
                
                // Set buffer size to reduce latency
                cap_.set(cv::CAP_PROP_BUFFERSIZE, 1);
                std::cout << "[StreamReader] Buffer size set to 1 (low latency mode)" << std::endl;
                
                // Try to enable hardware acceleration if available
                if (config_.use_hardware_decode) {
                    bool set_success = cap_.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('H', '2', '6', '4'));
                    std::cout << "[StreamReader] Hardware decode (H.264) set: " << (set_success ? "success" : "failed") << std::endl;
                }
                
                connected_ = true;
                reconnect_attempts_ = 0;
                
                std::cout << "[StreamReader] Connected to stream: " << config_.url << std::endl;
                return true;
            } else {
                std::cout << "[StreamReader] Failed to open with backend: " << backend_name << std::endl;
            }
        }
        
        // If all backends fail, provide detailed diagnostics
        std::ostringstream error_msg;
        error_msg << "Failed to open stream with any backend. Diagnostics:\n"
                  << "  URL: " << config_.url << "\n"
                  << "  Possible causes:\n"
                  << "    1. RTSP URL is incorrect (check camera brand-specific paths)\n"
                  << "    2. OpenCV not built with FFMPEG support\n"
                  << "    3. Network unreachable (firewall, IP, port 554)\n"
                  << "    4. Camera credentials invalid (admin:test1234)\n"
                  << "    5. Camera offline or stream not available\n"
                  << "  Suggestions:\n"
                  << "    - Use /discover endpoint to find correct RTSP URL\n"
                  << "    - Test with VLC: vlc '" << config_.url << "'\n"
                  << "    - Check network connectivity: ping 192.168.4.252\n"
                  << "    - Verify OpenCV build includes FFMPEG support";
        
        handle_error(error_msg.str());
        return false;
    } catch (const std::exception& e) {
        handle_error("Exception during connection: " + std::string(e.what()));
        return false;
    }
}

void StreamReader::disconnect() {
    if (cap_.isOpened()) {
        cap_.release();
    }
    connected_ = false;
}

void StreamReader::calculate_fps() {
    auto now = std::chrono::steady_clock::now();
    fps_counter_++;
    
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_fps_calc_time_);
    if (elapsed.count() >= 1000) { // Update every second
        double fps = static_cast<double>(fps_counter_) * 1000.0 / elapsed.count();
        current_fps_.store(fps);
        fps_counter_ = 0;
        last_fps_calc_time_ = now;
    }
}

void StreamReader::handle_error(const std::string& error) {
    std::cerr << "Stream error: " << error << std::endl;
    if (error_callback_) {
        error_callback_(error);
    }
}

void StreamReader::reconnect_with_backoff() {
    int attempts = reconnect_attempts_.fetch_add(1);
    
    if (config_.max_reconnect_attempts > 0 && attempts >= config_.max_reconnect_attempts) {
        handle_error("Max reconnect attempts reached");
        running_ = false;
        return;
    }
    
    // Exponential backoff with jitter
    int delay_ms = config_.reconnect_delay_ms * (1 << std::min(attempts, 10));
    delay_ms += (rand() % 1000); // Add jitter
    
    std::cout << "Reconnecting in " << delay_ms << "ms (attempt " << (attempts + 1) << ")" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
}