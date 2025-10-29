#include <iostream>
#include <string>
#include <csignal>
#include <chrono>
#include <thread>
#include <atomic>
#include <curl/curl.h>
#include <fstream>
#include <sstream>
#include <stdexcept>

std::atomic<bool> g_running{true};

void signal_handler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down..." << std::endl;
    g_running = false;
}

struct Config {
    std::string stream_url = "rtsp://admin:admin@192.168.1.100:554/stream1";
    std::string camera_id = "CAM01";
    std::string ai_host = "http://127.0.0.1:8000";
    int health_port = 8085;
    int fps_cap = 15;
    int queue_size = 32;
};

class SimpleStreamProcessor {
public:
    SimpleStreamProcessor(const Config& config) : config_(config) {
        curl_ = curl_easy_init();
        if (!curl_) {
            throw std::runtime_error("Failed to initialize CURL");
        }
        
        // Set common CURL options
        curl_easy_setopt(curl_, CURLOPT_TIMEOUT_MS, 5000);
        curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, 1L);
    }
    
    ~SimpleStreamProcessor() {
        if (curl_) {
            curl_easy_cleanup(curl_);
        }
    }
    
    bool process_image(const std::string& image_path) {
        if (!curl_) return false;
        
        // Read image file
        std::ifstream file(image_path, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Failed to open image: " << image_path << std::endl;
            return false;
        }
        
        std::ostringstream buffer;
        buffer << file.rdbuf();
        std::string image_data = buffer.str();
        
        if (image_data.empty()) {
            std::cerr << "Image file is empty: " << image_path << std::endl;
            return false;
        }
        
        // Create multipart form
        curl_mime* form = curl_mime_init(curl_);
        if (!form) {
            std::cerr << "Failed to create multipart form" << std::endl;
            return false;
        }
        
        // Add image part
        curl_mimepart* image_part = curl_mime_addpart(form);
        curl_mime_name(image_part, "image");
        curl_mime_data(image_part, image_data.data(), image_data.size());
        curl_mime_filename(image_part, "frame.jpg");
        curl_mime_type(image_part, "image/jpeg");
        
        // Add camera_id part
        curl_mimepart* camera_part = curl_mime_addpart(form);
        curl_mime_name(camera_part, "camera_id");
        curl_mime_data(camera_part, config_.camera_id.c_str(), CURL_ZERO_TERMINATED);
        
        // Set URL and form
        std::string url = config_.ai_host + "/infer";
        curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl_, CURLOPT_MIMEPOST, form);
        
        // Prepare response
        std::string response;
        curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, +[](char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
            std::string* resp = static_cast<std::string*>(userdata);
            resp->append(ptr, size * nmemb);
            return size * nmemb;
        });
        curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response);
        
        // Perform request
        CURLcode res = curl_easy_perform(curl_);
        
        // Clean up form
        curl_mime_free(form);
        
        if (res != CURLE_OK) {
            std::cerr << "CURL error: " << curl_easy_strerror(res) << std::endl;
            return false;
        }
        
        // Check response
        long status_code;
        curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &status_code);
        
        if (status_code == 200) {
            std::cout << "Inference result: " << response << std::endl;
            return true;
        } else {
            std::cerr << "HTTP error: " << status_code << " - " << response << std::endl;
            return false;
        }
    }
    
    bool check_ai_health() {
        if (!curl_) return false;
        
        std::string url = config_.ai_host + "/healthz";
        curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl_, CURLOPT_HTTPGET, 1L);
        curl_easy_setopt(curl_, CURLOPT_MIMEPOST, nullptr);
        
        std::string response;
        curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, +[](char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
            std::string* resp = static_cast<std::string*>(userdata);
            resp->append(ptr, size * nmemb);
            return size * nmemb;
        });
        curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response);
        
        CURLcode res = curl_easy_perform(curl_);
        if (res != CURLE_OK) {
            std::cerr << "CURL error for health check: " << curl_easy_strerror(res) << std::endl;
            return false;
        }
        
        long status_code;
        curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &status_code);
        
        std::cout << "AI health response: " << response << " (status: " << status_code << ")" << std::endl;
        return status_code == 200;
    }

private:
    Config config_;
    CURL* curl_;
};

int main(int argc, char** argv) {
    // Set up signal handlers
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    
    std::cout << "LPR C++ Client v1.0 (Simple Mode)" << std::endl;
    std::cout << "=================================" << std::endl;
    
    // Parse command line arguments
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <image_path> [camera_id] [ai_host]" << std::endl;
        std::cerr << "Example: " << argv[0] << " test.jpg CAM01 http://127.0.0.1:8000" << std::endl;
        return 1;
    }
    
    std::string image_path = argv[1];
    std::string camera_id = (argc > 2) ? argv[2] : "CAM01";
    std::string ai_host = (argc > 3) ? argv[3] : "http://127.0.0.1:8000";
    
    Config config;
    config.camera_id = camera_id;
    config.ai_host = ai_host;
    
    std::cout << "Configuration:" << std::endl;
    std::cout << "  Image: " << image_path << std::endl;
    std::cout << "  Camera ID: " << config.camera_id << std::endl;
    std::cout << "  AI Host: " << config.ai_host << std::endl;
    std::cout << std::endl;
    
    try {
        SimpleStreamProcessor processor(config);
        
        // Check AI service health
        std::cout << "Checking AI service health..." << std::endl;
        if (!processor.check_ai_health()) {
            std::cerr << "AI service is not healthy at " << config.ai_host << std::endl;
            std::cerr << "Make sure the Python AI service is running on port 8000" << std::endl;
            return 1;
        }
        std::cout << "AI service is healthy!" << std::endl;
        
        // Process image
        std::cout << "Processing image: " << image_path << std::endl;
        if (processor.process_image(image_path)) {
            std::cout << "Image processed successfully!" << std::endl;
        } else {
            std::cerr << "Failed to process image" << std::endl;
            return 1;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "Done!" << std::endl;
    return 0;
}