#pragma once

#include <opencv2/opencv.hpp>
#include <chrono>
#include <string>

struct Frame {
    cv::Mat image;
    std::chrono::system_clock::time_point timestamp;
    std::string camera_id;
    uint64_t frame_id;
    
    Frame() = default;
    Frame(cv::Mat img, const std::string& cam_id, uint64_t id) 
        : image(std::move(img)), timestamp(std::chrono::system_clock::now()), 
          camera_id(cam_id), frame_id(id) {}
    
    // Move constructor
    Frame(Frame&& other) noexcept 
        : image(std::move(other.image)), timestamp(other.timestamp),
          camera_id(std::move(other.camera_id)), frame_id(other.frame_id) {}
    
    // Move assignment
    Frame& operator=(Frame&& other) noexcept {
        if (this != &other) {
            image = std::move(other.image);
            timestamp = other.timestamp;
            camera_id = std::move(other.camera_id);
            frame_id = other.frame_id;
        }
        return *this;
    }
    
    // Disable copy
    Frame(const Frame&) = delete;
    Frame& operator=(const Frame&) = delete;
};

struct InferenceResult {
    std::string plate_text;
    double confidence;
    cv::Rect bbox;
    std::chrono::system_clock::time_point timestamp;
    std::string camera_id;
    uint64_t frame_id;
    std::string vehicle_color;
    std::string vehicle_type;
    
    InferenceResult() = default;
    InferenceResult(const std::string& plate, double conf, const cv::Rect& box,
                   const std::chrono::system_clock::time_point& ts,
                   const std::string& cam_id, uint64_t fid)
        : plate_text(plate), confidence(conf), bbox(box), timestamp(ts),
          camera_id(cam_id), frame_id(fid) {}
};




#include <opencv2/opencv.hpp>
#include <chrono>
#include <string>

struct Frame {
    cv::Mat image;
    std::chrono::system_clock::time_point timestamp;
    std::string camera_id;
    uint64_t frame_id;
    
    Frame() = default;
    Frame(cv::Mat img, const std::string& cam_id, uint64_t id) 
        : image(std::move(img)), timestamp(std::chrono::system_clock::now()), 
          camera_id(cam_id), frame_id(id) {}
    
    // Move constructor
    Frame(Frame&& other) noexcept 
        : image(std::move(other.image)), timestamp(other.timestamp),
          camera_id(std::move(other.camera_id)), frame_id(other.frame_id) {}
    
    // Move assignment
    Frame& operator=(Frame&& other) noexcept {
        if (this != &other) {
            image = std::move(other.image);
            timestamp = other.timestamp;
            camera_id = std::move(other.camera_id);
            frame_id = other.frame_id;
        }
        return *this;
    }
    
    // Disable copy
    Frame(const Frame&) = delete;
    Frame& operator=(const Frame&) = delete;
};

struct InferenceResult {
    std::string plate_text;
    double confidence;
    cv::Rect bbox;
    std::chrono::system_clock::time_point timestamp;
    std::string camera_id;
    uint64_t frame_id;
    std::string vehicle_color;
    std::string vehicle_type;
    
    InferenceResult() = default;
    InferenceResult(const std::string& plate, double conf, const cv::Rect& box,
                   const std::chrono::system_clock::time_point& ts,
                   const std::string& cam_id, uint64_t fid)
        : plate_text(plate), confidence(conf), bbox(box), timestamp(ts),
          camera_id(cam_id), frame_id(fid) {}
};


