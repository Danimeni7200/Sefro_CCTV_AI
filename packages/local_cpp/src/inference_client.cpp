#include "inference_client.hpp"
#include <opencv2/opencv.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <sstream>

InferenceClient::InferenceClient(const AIServiceConfig& config) 
    : config_(config), curl_(curl_easy_init()) {
    if (!curl_) {
        throw std::runtime_error("Failed to initialize CURL");
    }
    
    // Set common CURL options
    curl_easy_setopt(curl_, CURLOPT_TIMEOUT_MS, config_.timeout_ms);
    curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, 0L);
    
    // Set callbacks
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl_, CURLOPT_HEADERFUNCTION, header_callback);
    
    // Perform initial health check
    check_health();
}

InferenceClient::~InferenceClient() {
    if (curl_) {
        curl_easy_cleanup(curl_);
    }
}

bool InferenceClient::infer(const Frame& frame, InferenceResult& result) {
    if (frame.image.empty()) {
        return false;
    }
    
    return perform_request_with_retry(frame, result);
}

bool InferenceClient::perform_request_with_retry(const Frame& frame, InferenceResult& result) {
    for (int attempt = 0; attempt <= config_.retry_count; ++attempt) {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        bool success = perform_single_request(frame, result);
        
        auto end_time = std::chrono::high_resolution_clock::now();
        double latency_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
        
        update_stats(success, latency_ms);
        
        if (success) {
            update_health_status(true);
            return true;
        }
        
        // If this wasn't the last attempt, wait before retrying
        if (attempt < config_.retry_count) {
            std::this_thread::sleep_for(std::chrono::milliseconds(config_.retry_delay_ms));
        }
    }
    
    update_health_status(false);
    return false;
}

bool InferenceClient::perform_single_request(const Frame& frame, InferenceResult& result) {
    try {
        // Encode image as JPEG
        std::vector<uchar> image_data = encode_image_jpeg(frame.image);
        if (image_data.empty()) {
            return false;
        }
        
        // Create multipart form
        curl_mime* form = curl_mime_init(curl_);
        if (!form) {
            return false;
        }
        
        // Add image part
        curl_mimepart* image_part = curl_mime_addpart(form);
        curl_mime_name(image_part, "image");
        curl_mime_data(image_part, reinterpret_cast<const char*>(image_data.data()), image_data.size());
        curl_mime_filename(image_part, "frame.jpg");
        curl_mime_type(image_part, "image/jpeg");
        
        // Add camera_id part
        curl_mimepart* camera_part = curl_mime_addpart(form);
        curl_mime_name(camera_part, "camera_id");
        curl_mime_data(camera_part, frame.camera_id.c_str(), CURL_ZERO_TERMINATED);
        
        // Set URL and form
        std::string url = config_.host + "/infer";
        curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl_, CURLOPT_MIMEPOST, form);
        
        // Prepare response
        ResponseData response;
        curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl_, CURLOPT_HEADERDATA, &response);
        
        // Perform request
        CURLcode res = curl_easy_perform(curl_);
        
        // Clean up form
        curl_mime_free(form);
        
        if (res != CURLE_OK) {
            std::string error = "CURL error: " + std::string(curl_easy_strerror(res));
            if (error_callback_) {
                error_callback_(error);
            }
            return false;
        }
        
        // Check HTTP status
        if (response.status_code != 200) {
            std::string error = "HTTP error: " + std::to_string(response.status_code);
            if (error_callback_) {
                error_callback_(error);
            }
            return false;
        }
        
        // Parse response
        if (!parse_response(response.data, result)) {
            std::string error = "Failed to parse response JSON";
            if (error_callback_) {
                error_callback_(error);
            }
            return false;
        }
        
        // Update result with frame info
        result.timestamp = frame.timestamp;
        result.camera_id = frame.camera_id;
        result.frame_id = frame.frame_id;
        
        return true;
        
    } catch (const std::exception& e) {
        std::string error = "Exception in inference request: " + std::string(e.what());
        if (error_callback_) {
            error_callback_(error);
        }
        return false;
    }
}

bool InferenceClient::check_health() {
    try {
        std::string url = config_.host + "/healthz";
        curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl_, CURLOPT_HTTPGET, 1L);
        curl_easy_setopt(curl_, CURLOPT_MIMEPOST, nullptr);
        
        ResponseData response;
        curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl_, CURLOPT_HEADERDATA, &response);
        
        CURLcode res = curl_easy_perform(curl_);
        bool healthy = (res == CURLE_OK && response.status_code == 200);
        
        update_health_status(healthy);
        return healthy;
        
    } catch (const std::exception& e) {
        update_health_status(false);
        return false;
    }
}

void InferenceClient::update_health_status(bool healthy) {
    bool was_healthy = healthy_.exchange(healthy);
    
    if (was_healthy != healthy) {
        if (health_callback_) {
            health_callback_(healthy);
        }
        
        std::cout << "AI service health changed: " << (healthy ? "HEALTHY" : "UNHEALTHY") << std::endl;
    }
}

void InferenceClient::update_stats(bool success, double latency_ms) {
    stats_.total_requests++;
    
    if (success) {
        stats_.successful_requests++;
        stats_.last_success = std::chrono::system_clock::now();
        
        // Update average latency (exponential moving average)
        if (stats_.average_latency_ms == 0.0) {
            stats_.average_latency_ms = latency_ms;
        } else {
            stats_.average_latency_ms = 0.9 * stats_.average_latency_ms + 0.1 * latency_ms;
        }
    } else {
        stats_.failed_requests++;
        stats_.last_failure = std::chrono::system_clock::now();
    }
}

bool InferenceClient::parse_response(const std::string& json_response, InferenceResult& result) {
    try {
        nlohmann::json j = nlohmann::json::parse(json_response);
        
        result.plate_text = j.value("plate_text", "");
        result.confidence = j.value("confidence", 0.0);
        result.vehicle_color = j.value("vehicle_color", "");
        result.vehicle_type = j.value("vehicle_type", "");
        
        // Parse bounding box
        if (j.contains("bbox")) {
            const auto& bbox = j["bbox"];
            result.bbox.x = bbox.value("x1", 0);
            result.bbox.y = bbox.value("y1", 0);
            result.bbox.width = bbox.value("x2", 0) - result.bbox.x;
            result.bbox.height = bbox.value("y2", 0) - result.bbox.y;
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "JSON parsing error: " << e.what() << std::endl;
        return false;
    }
}

std::vector<uchar> InferenceClient::encode_image_jpeg(const cv::Mat& image, int quality) {
    std::vector<uchar> buffer;
    std::vector<int> params = {cv::IMWRITE_JPEG_QUALITY, quality};
    
    if (!cv::imencode(".jpg", image, buffer, params)) {
        return {};
    }
    
    return buffer;
}

size_t InferenceClient::write_callback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    ResponseData* response = static_cast<ResponseData*>(userdata);
    size_t total_size = size * nmemb;
    response->data.append(ptr, total_size);
    return total_size;
}

size_t InferenceClient::header_callback(char* buffer, size_t size, size_t nitems, void* userdata) {
    ResponseData* response = static_cast<ResponseData*>(userdata);
    size_t total_size = size * nitems;
    
    std::string header(buffer, total_size);
    
    // Extract status code
    if (header.substr(0, 4) == "HTTP") {
        std::istringstream iss(header);
        std::string http_version;
        iss >> http_version >> response->status_code;
    }
    
    return total_size;
}



#include <opencv2/opencv.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <sstream>

InferenceClient::InferenceClient(const AIServiceConfig& config) 
    : config_(config), curl_(curl_easy_init()) {
    if (!curl_) {
        throw std::runtime_error("Failed to initialize CURL");
    }
    
    // Set common CURL options
    curl_easy_setopt(curl_, CURLOPT_TIMEOUT_MS, config_.timeout_ms);
    curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, 0L);
    
    // Set callbacks
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl_, CURLOPT_HEADERFUNCTION, header_callback);
    
    // Perform initial health check
    check_health();
}

InferenceClient::~InferenceClient() {
    if (curl_) {
        curl_easy_cleanup(curl_);
    }
}

bool InferenceClient::infer(const Frame& frame, InferenceResult& result) {
    if (frame.image.empty()) {
        return false;
    }
    
    return perform_request_with_retry(frame, result);
}

bool InferenceClient::perform_request_with_retry(const Frame& frame, InferenceResult& result) {
    for (int attempt = 0; attempt <= config_.retry_count; ++attempt) {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        bool success = perform_single_request(frame, result);
        
        auto end_time = std::chrono::high_resolution_clock::now();
        double latency_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
        
        update_stats(success, latency_ms);
        
        if (success) {
            update_health_status(true);
            return true;
        }
        
        // If this wasn't the last attempt, wait before retrying
        if (attempt < config_.retry_count) {
            std::this_thread::sleep_for(std::chrono::milliseconds(config_.retry_delay_ms));
        }
    }
    
    update_health_status(false);
    return false;
}

bool InferenceClient::perform_single_request(const Frame& frame, InferenceResult& result) {
    try {
        // Encode image as JPEG
        std::vector<uchar> image_data = encode_image_jpeg(frame.image);
        if (image_data.empty()) {
            return false;
        }
        
        // Create multipart form
        curl_mime* form = curl_mime_init(curl_);
        if (!form) {
            return false;
        }
        
        // Add image part
        curl_mimepart* image_part = curl_mime_addpart(form);
        curl_mime_name(image_part, "image");
        curl_mime_data(image_part, reinterpret_cast<const char*>(image_data.data()), image_data.size());
        curl_mime_filename(image_part, "frame.jpg");
        curl_mime_type(image_part, "image/jpeg");
        
        // Add camera_id part
        curl_mimepart* camera_part = curl_mime_addpart(form);
        curl_mime_name(camera_part, "camera_id");
        curl_mime_data(camera_part, frame.camera_id.c_str(), CURL_ZERO_TERMINATED);
        
        // Set URL and form
        std::string url = config_.host + "/infer";
        curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl_, CURLOPT_MIMEPOST, form);
        
        // Prepare response
        ResponseData response;
        curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl_, CURLOPT_HEADERDATA, &response);
        
        // Perform request
        CURLcode res = curl_easy_perform(curl_);
        
        // Clean up form
        curl_mime_free(form);
        
        if (res != CURLE_OK) {
            std::string error = "CURL error: " + std::string(curl_easy_strerror(res));
            if (error_callback_) {
                error_callback_(error);
            }
            return false;
        }
        
        // Check HTTP status
        if (response.status_code != 200) {
            std::string error = "HTTP error: " + std::to_string(response.status_code);
            if (error_callback_) {
                error_callback_(error);
            }
            return false;
        }
        
        // Parse response
        if (!parse_response(response.data, result)) {
            std::string error = "Failed to parse response JSON";
            if (error_callback_) {
                error_callback_(error);
            }
            return false;
        }
        
        // Update result with frame info
        result.timestamp = frame.timestamp;
        result.camera_id = frame.camera_id;
        result.frame_id = frame.frame_id;
        
        return true;
        
    } catch (const std::exception& e) {
        std::string error = "Exception in inference request: " + std::string(e.what());
        if (error_callback_) {
            error_callback_(error);
        }
        return false;
    }
}

bool InferenceClient::check_health() {
    try {
        std::string url = config_.host + "/healthz";
        curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl_, CURLOPT_HTTPGET, 1L);
        curl_easy_setopt(curl_, CURLOPT_MIMEPOST, nullptr);
        
        ResponseData response;
        curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl_, CURLOPT_HEADERDATA, &response);
        
        CURLcode res = curl_easy_perform(curl_);
        bool healthy = (res == CURLE_OK && response.status_code == 200);
        
        update_health_status(healthy);
        return healthy;
        
    } catch (const std::exception& e) {
        update_health_status(false);
        return false;
    }
}

void InferenceClient::update_health_status(bool healthy) {
    bool was_healthy = healthy_.exchange(healthy);
    
    if (was_healthy != healthy) {
        if (health_callback_) {
            health_callback_(healthy);
        }
        
        std::cout << "AI service health changed: " << (healthy ? "HEALTHY" : "UNHEALTHY") << std::endl;
    }
}

void InferenceClient::update_stats(bool success, double latency_ms) {
    stats_.total_requests++;
    
    if (success) {
        stats_.successful_requests++;
        stats_.last_success = std::chrono::system_clock::now();
        
        // Update average latency (exponential moving average)
        if (stats_.average_latency_ms == 0.0) {
            stats_.average_latency_ms = latency_ms;
        } else {
            stats_.average_latency_ms = 0.9 * stats_.average_latency_ms + 0.1 * latency_ms;
        }
    } else {
        stats_.failed_requests++;
        stats_.last_failure = std::chrono::system_clock::now();
    }
}

bool InferenceClient::parse_response(const std::string& json_response, InferenceResult& result) {
    try {
        nlohmann::json j = nlohmann::json::parse(json_response);
        
        result.plate_text = j.value("plate_text", "");
        result.confidence = j.value("confidence", 0.0);
        result.vehicle_color = j.value("vehicle_color", "");
        result.vehicle_type = j.value("vehicle_type", "");
        
        // Parse bounding box
        if (j.contains("bbox")) {
            const auto& bbox = j["bbox"];
            result.bbox.x = bbox.value("x1", 0);
            result.bbox.y = bbox.value("y1", 0);
            result.bbox.width = bbox.value("x2", 0) - result.bbox.x;
            result.bbox.height = bbox.value("y2", 0) - result.bbox.y;
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "JSON parsing error: " << e.what() << std::endl;
        return false;
    }
}

std::vector<uchar> InferenceClient::encode_image_jpeg(const cv::Mat& image, int quality) {
    std::vector<uchar> buffer;
    std::vector<int> params = {cv::IMWRITE_JPEG_QUALITY, quality};
    
    if (!cv::imencode(".jpg", image, buffer, params)) {
        return {};
    }
    
    return buffer;
}

size_t InferenceClient::write_callback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    ResponseData* response = static_cast<ResponseData*>(userdata);
    size_t total_size = size * nmemb;
    response->data.append(ptr, total_size);
    return total_size;
}

size_t InferenceClient::header_callback(char* buffer, size_t size, size_t nitems, void* userdata) {
    ResponseData* response = static_cast<ResponseData*>(userdata);
    size_t total_size = size * nitems;
    
    std::string header(buffer, total_size);
    
    // Extract status code
    if (header.substr(0, 4) == "HTTP") {
        std::istringstream iss(header);
        std::string http_version;
        iss >> http_version >> response->status_code;
    }
    
    return total_size;
}


