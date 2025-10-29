#include "preprocessor.hpp"
#include <iostream>
#include <algorithm>

Preprocessor::Preprocessor(const PreprocessingConfig& config) : config_(config) {}

Frame Preprocessor::process(Frame&& input) {
    if (input.image.empty()) {
        return std::move(input);
    }
    
    // Check quality first
    if (!is_quality_acceptable(input.image)) {
        // Return empty frame to indicate it should be skipped
        input.image = cv::Mat();
        return std::move(input);
    }
    
    cv::Mat processed = input.image.clone();
    
    // Resize with letterboxing
    if (config_.letterbox) {
        processed = resize_with_letterbox(processed, config_.target_width, config_.target_height);
    } else {
        cv::resize(processed, processed, cv::Size(config_.target_width, config_.target_height));
    }
    
    // Apply gamma correction
    if (std::abs(config_.gamma - 1.0) > 0.01) {
        apply_gamma(processed, config_.gamma);
    }
    
    // Apply denoising
    if (config_.denoise) {
        apply_denoise(processed);
    }
    
    // Apply sharpening
    if (config_.sharpen) {
        apply_sharpen(processed);
    }
    
    // Update the frame with processed image
    input.image = std::move(processed);
    return std::move(input);
}

bool Preprocessor::is_quality_acceptable(const cv::Mat& image) const {
    if (image.empty()) {
        return false;
    }
    
    double quality = calculate_quality_score(image);
    return quality >= config_.quality_threshold;
}

void Preprocessor::apply_gamma(cv::Mat& image, double gamma) const {
    cv::Mat lookup_table(1, 256, CV_8U);
    uchar* p = lookup_table.ptr();
    for (int i = 0; i < 256; ++i) {
        p[i] = cv::saturate_cast<uchar>(pow(i / 255.0, gamma) * 255.0);
    }
    cv::LUT(image, lookup_table, image);
}

void Preprocessor::apply_denoise(cv::Mat& image) const {
    cv::Mat denoised;
    cv::fastNlMeansDenoising(image, denoised, 10, 7, 21);
    image = std::move(denoised);
}

void Preprocessor::apply_sharpen(cv::Mat& image) const {
    cv::Mat kernel = get_sharpen_kernel();
    cv::Mat sharpened;
    cv::filter2D(image, sharpened, -1, kernel);
    image = std::move(sharpened);
}

cv::Mat Preprocessor::resize_with_letterbox(const cv::Mat& image, int target_width, int target_height) const {
    int src_width = image.cols;
    int src_height = image.rows;
    
    // Calculate scaling factor to fit image in target size
    double scale = std::min(static_cast<double>(target_width) / src_width,
                           static_cast<double>(target_height) / src_height);
    
    int new_width = static_cast<int>(src_width * scale);
    int new_height = static_cast<int>(src_height * scale);
    
    // Resize image
    cv::Mat resized;
    cv::resize(image, resized, cv::Size(new_width, new_height));
    
    // Create target image with black background
    cv::Mat result = cv::Mat::zeros(target_height, target_width, image.type());
    
    // Calculate position to center the resized image
    int x_offset = (target_width - new_width) / 2;
    int y_offset = (target_height - new_height) / 2;
    
    // Copy resized image to center of target
    cv::Rect roi(x_offset, y_offset, new_width, new_height);
    resized.copyTo(result(roi));
    
    return result;
}

double Preprocessor::calculate_quality_score(const cv::Mat& image) const {
    if (image.empty()) {
        return 0.0;
    }
    
    // Convert to grayscale for analysis
    cv::Mat gray;
    if (image.channels() == 3) {
        cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = image;
    }
    
    // Calculate different quality metrics
    double sharpness = calculate_sharpness(gray);
    double brightness = calculate_brightness(gray);
    double contrast = calculate_contrast(gray);
    
    // Normalize and combine metrics (weights can be tuned)
    double normalized_sharpness = std::min(sharpness / 1000.0, 1.0); // Normalize to 0-1
    double normalized_brightness = 1.0 - std::abs(brightness - 0.5) * 2.0; // Prefer middle brightness
    double normalized_contrast = std::min(contrast / 100.0, 1.0); // Normalize to 0-1
    
    // Weighted combination
    double quality = 0.5 * normalized_sharpness + 0.3 * normalized_brightness + 0.2 * normalized_contrast;
    
    return std::max(0.0, std::min(1.0, quality));
}

double Preprocessor::calculate_sharpness(const cv::Mat& image) const {
    cv::Mat laplacian;
    cv::Laplacian(image, laplacian, CV_64F);
    cv::Scalar mu, sigma;
    cv::meanStdDev(laplacian, mu, sigma);
    return sigma.val[0] * sigma.val[0]; // Variance of Laplacian
}

double Preprocessor::calculate_brightness(const cv::Mat& image) const {
    cv::Scalar mean = cv::mean(image);
    return mean.val[0] / 255.0; // Normalize to 0-1
}

double Preprocessor::calculate_contrast(const cv::Mat& image) const {
    cv::Mat gray;
    if (image.channels() == 3) {
        cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = image;
    }
    
    cv::Scalar mu, sigma;
    cv::meanStdDev(gray, mu, sigma);
    return sigma.val[0]; // Standard deviation as contrast measure
}

cv::Mat Preprocessor::get_sharpen_kernel() const {
    return (cv::Mat_<float>(3, 3) << 
        0, -1, 0,
        -1, 5, -1,
        0, -1, 0);
}



#include <iostream>
#include <algorithm>

Preprocessor::Preprocessor(const PreprocessingConfig& config) : config_(config) {}

Frame Preprocessor::process(Frame&& input) {
    if (input.image.empty()) {
        return std::move(input);
    }
    
    // Check quality first
    if (!is_quality_acceptable(input.image)) {
        // Return empty frame to indicate it should be skipped
        input.image = cv::Mat();
        return std::move(input);
    }
    
    cv::Mat processed = input.image.clone();
    
    // Resize with letterboxing
    if (config_.letterbox) {
        processed = resize_with_letterbox(processed, config_.target_width, config_.target_height);
    } else {
        cv::resize(processed, processed, cv::Size(config_.target_width, config_.target_height));
    }
    
    // Apply gamma correction
    if (std::abs(config_.gamma - 1.0) > 0.01) {
        apply_gamma(processed, config_.gamma);
    }
    
    // Apply denoising
    if (config_.denoise) {
        apply_denoise(processed);
    }
    
    // Apply sharpening
    if (config_.sharpen) {
        apply_sharpen(processed);
    }
    
    // Update the frame with processed image
    input.image = std::move(processed);
    return std::move(input);
}

bool Preprocessor::is_quality_acceptable(const cv::Mat& image) const {
    if (image.empty()) {
        return false;
    }
    
    double quality = calculate_quality_score(image);
    return quality >= config_.quality_threshold;
}

void Preprocessor::apply_gamma(cv::Mat& image, double gamma) const {
    cv::Mat lookup_table(1, 256, CV_8U);
    uchar* p = lookup_table.ptr();
    for (int i = 0; i < 256; ++i) {
        p[i] = cv::saturate_cast<uchar>(pow(i / 255.0, gamma) * 255.0);
    }
    cv::LUT(image, lookup_table, image);
}

void Preprocessor::apply_denoise(cv::Mat& image) const {
    cv::Mat denoised;
    cv::fastNlMeansDenoising(image, denoised, 10, 7, 21);
    image = std::move(denoised);
}

void Preprocessor::apply_sharpen(cv::Mat& image) const {
    cv::Mat kernel = get_sharpen_kernel();
    cv::Mat sharpened;
    cv::filter2D(image, sharpened, -1, kernel);
    image = std::move(sharpened);
}

cv::Mat Preprocessor::resize_with_letterbox(const cv::Mat& image, int target_width, int target_height) const {
    int src_width = image.cols;
    int src_height = image.rows;
    
    // Calculate scaling factor to fit image in target size
    double scale = std::min(static_cast<double>(target_width) / src_width,
                           static_cast<double>(target_height) / src_height);
    
    int new_width = static_cast<int>(src_width * scale);
    int new_height = static_cast<int>(src_height * scale);
    
    // Resize image
    cv::Mat resized;
    cv::resize(image, resized, cv::Size(new_width, new_height));
    
    // Create target image with black background
    cv::Mat result = cv::Mat::zeros(target_height, target_width, image.type());
    
    // Calculate position to center the resized image
    int x_offset = (target_width - new_width) / 2;
    int y_offset = (target_height - new_height) / 2;
    
    // Copy resized image to center of target
    cv::Rect roi(x_offset, y_offset, new_width, new_height);
    resized.copyTo(result(roi));
    
    return result;
}

double Preprocessor::calculate_quality_score(const cv::Mat& image) const {
    if (image.empty()) {
        return 0.0;
    }
    
    // Convert to grayscale for analysis
    cv::Mat gray;
    if (image.channels() == 3) {
        cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = image;
    }
    
    // Calculate different quality metrics
    double sharpness = calculate_sharpness(gray);
    double brightness = calculate_brightness(gray);
    double contrast = calculate_contrast(gray);
    
    // Normalize and combine metrics (weights can be tuned)
    double normalized_sharpness = std::min(sharpness / 1000.0, 1.0); // Normalize to 0-1
    double normalized_brightness = 1.0 - std::abs(brightness - 0.5) * 2.0; // Prefer middle brightness
    double normalized_contrast = std::min(contrast / 100.0, 1.0); // Normalize to 0-1
    
    // Weighted combination
    double quality = 0.5 * normalized_sharpness + 0.3 * normalized_brightness + 0.2 * normalized_contrast;
    
    return std::max(0.0, std::min(1.0, quality));
}

double Preprocessor::calculate_sharpness(const cv::Mat& image) const {
    cv::Mat laplacian;
    cv::Laplacian(image, laplacian, CV_64F);
    cv::Scalar mu, sigma;
    cv::meanStdDev(laplacian, mu, sigma);
    return sigma.val[0] * sigma.val[0]; // Variance of Laplacian
}

double Preprocessor::calculate_brightness(const cv::Mat& image) const {
    cv::Scalar mean = cv::mean(image);
    return mean.val[0] / 255.0; // Normalize to 0-1
}

double Preprocessor::calculate_contrast(const cv::Mat& image) const {
    cv::Mat gray;
    if (image.channels() == 3) {
        cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = image;
    }
    
    cv::Scalar mu, sigma;
    cv::meanStdDev(gray, mu, sigma);
    return sigma.val[0]; // Standard deviation as contrast measure
}

cv::Mat Preprocessor::get_sharpen_kernel() const {
    return (cv::Mat_<float>(3, 3) << 
        0, -1, 0,
        -1, 5, -1,
        0, -1, 0);
}


