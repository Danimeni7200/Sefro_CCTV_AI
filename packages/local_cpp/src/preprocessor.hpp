#pragma once

#include <opencv2/opencv.hpp>
#include "frame.hpp"
#include "config.hpp"

class Preprocessor {
public:
    explicit Preprocessor(const PreprocessingConfig& config);
    
    // Process a frame
    Frame process(Frame&& input);
    
    // Check if frame meets quality requirements
    bool is_quality_acceptable(const cv::Mat& image) const;
    
    // Apply gamma correction
    void apply_gamma(cv::Mat& image, double gamma) const;
    
    // Apply denoising
    void apply_denoise(cv::Mat& image) const;
    
    // Apply sharpening
    void apply_sharpen(cv::Mat& image) const;
    
    // Resize with letterboxing
    cv::Mat resize_with_letterbox(const cv::Mat& image, int target_width, int target_height) const;
    
    // Calculate image quality score (0.0 to 1.0)
    double calculate_quality_score(const cv::Mat& image) const;

private:
    PreprocessingConfig config_;
    
    // Quality assessment
    double calculate_sharpness(const cv::Mat& image) const;
    double calculate_brightness(const cv::Mat& image) const;
    double calculate_contrast(const cv::Mat& image) const;
    
    // Preprocessing kernels
    cv::Mat get_sharpen_kernel() const;
};




#include <opencv2/opencv.hpp>
#include "frame.hpp"
#include "config.hpp"

class Preprocessor {
public:
    explicit Preprocessor(const PreprocessingConfig& config);
    
    // Process a frame
    Frame process(Frame&& input);
    
    // Check if frame meets quality requirements
    bool is_quality_acceptable(const cv::Mat& image) const;
    
    // Apply gamma correction
    void apply_gamma(cv::Mat& image, double gamma) const;
    
    // Apply denoising
    void apply_denoise(cv::Mat& image) const;
    
    // Apply sharpening
    void apply_sharpen(cv::Mat& image) const;
    
    // Resize with letterboxing
    cv::Mat resize_with_letterbox(const cv::Mat& image, int target_width, int target_height) const;
    
    // Calculate image quality score (0.0 to 1.0)
    double calculate_quality_score(const cv::Mat& image) const;

private:
    PreprocessingConfig config_;
    
    // Quality assessment
    double calculate_sharpness(const cv::Mat& image) const;
    double calculate_brightness(const cv::Mat& image) const;
    double calculate_contrast(const cv::Mat& image) const;
    
    // Preprocessing kernels
    cv::Mat get_sharpen_kernel() const;
};


