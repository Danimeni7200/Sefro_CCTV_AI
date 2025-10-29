#include "logger.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <nlohmann/json.hpp>

Logger::Logger(const LoggingConfig& config) : config_(config) {
    // Create logs directory if it doesn't exist
    std::filesystem::path log_path(config.file);
    std::filesystem::create_directories(log_path.parent_path());
    
    // Open log file
    current_log_file_ = config.file;
    log_file_.open(current_log_file_, std::ios::app);
    
    last_rotation_ = std::chrono::system_clock::now();
    set_level(config.level);
    
    log_info("Logger initialized");
}

Logger::~Logger() {
    if (log_file_.is_open()) {
        log_file_.close();
    }
}

void Logger::log_inference(const InferenceResult& result) {
    if (!should_log("INFO")) return;
    
    std::string message = format_inference_result(result);
    write_log("INFO", message);
}

void Logger::log_frame(const Frame& frame, const std::string& status) {
    if (!should_log("DEBUG")) return;
    
    std::string message = format_frame_info(frame, status);
    write_log("DEBUG", message);
}

void Logger::log_error(const std::string& error) {
    if (!should_log("ERROR")) return;
    
    write_log("ERROR", error);
}

void Logger::log_info(const std::string& message) {
    if (!should_log("INFO")) return;
    
    write_log("INFO", message);
}

void Logger::log_warning(const std::string& message) {
    if (!should_log("WARNING")) return;
    
    write_log("WARNING", message);
}

void Logger::log_debug(const std::string& message) {
    if (!should_log("DEBUG")) return;
    
    write_log("DEBUG", message);
}

void Logger::set_level(const std::string& level) {
    log_level_ = level_to_int(level);
}

bool Logger::should_log(const std::string& level) const {
    return level_to_int(level) >= log_level_;
}

void Logger::write_log(const std::string& level, const std::string& message) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    
    rotate_log_if_needed();
    
    std::string timestamp = get_timestamp();
    std::string log_entry = "[" + timestamp + "] [" + level + "] " + message;
    
    // Write to file
    if (log_file_.is_open()) {
        log_file_ << log_entry << std::endl;
        log_file_.flush();
    }
    
    // Write to console if enabled
    if (config_.console_output) {
        std::cout << log_entry << std::endl;
    }
}

void Logger::rotate_log_if_needed() {
    if (!config_.rotate_daily) {
        return;
    }
    
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);
    
    // Check if we need to rotate (new day)
    static int last_day = -1;
    if (last_day == -1) {
        last_day = tm.tm_mday;
        return;
    }
    
    if (tm.tm_mday != last_day) {
        last_day = tm.tm_mday;
        
        // Close current file
        if (log_file_.is_open()) {
            log_file_.close();
        }
        
        // Create new filename with date
        std::stringstream ss;
        ss << std::put_time(&tm, "%Y%m%d");
        std::string date_str = ss.str();
        
        std::filesystem::path log_path(config_.file);
        std::string new_filename = log_path.parent_path().string() + "/" + 
                                 log_path.stem().string() + "_" + date_str + 
                                 log_path.extension().string();
        
        // Open new file
        current_log_file_ = new_filename;
        log_file_.open(current_log_file_, std::ios::app);
        
        log_info("Log rotated to: " + new_filename);
    }
}

std::string Logger::get_timestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    auto tm = *std::localtime(&time_t);
    
    std::stringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    ss << "." << std::setfill('0') << std::setw(3) << ms.count();
    
    return ss.str();
}

std::string Logger::format_inference_result(const InferenceResult& result) const {
    nlohmann::json j;
    j["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        result.timestamp.time_since_epoch()).count();
    j["camera_id"] = result.camera_id;
    j["frame_id"] = result.frame_id;
    j["plate_text"] = result.plate_text;
    j["confidence"] = result.confidence;
    j["bbox"] = {
        {"x", result.bbox.x},
        {"y", result.bbox.y},
        {"width", result.bbox.width},
        {"height", result.bbox.height}
    };
    j["vehicle_color"] = result.vehicle_color;
    j["vehicle_type"] = result.vehicle_type;
    
    return "INFERENCE: " + j.dump();
}

std::string Logger::format_frame_info(const Frame& frame, const std::string& status) const {
    nlohmann::json j;
    j["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        frame.timestamp.time_since_epoch()).count();
    j["camera_id"] = frame.camera_id;
    j["frame_id"] = frame.frame_id;
    j["status"] = status;
    j["image_size"] = {
        {"width", frame.image.cols},
        {"height", frame.image.rows}
    };
    
    return "FRAME: " + j.dump();
}

int Logger::level_to_int(const std::string& level) const {
    if (level == "DEBUG") return 0;
    if (level == "INFO") return 1;
    if (level == "WARNING") return 2;
    if (level == "ERROR") return 3;
    return 1; // Default to INFO
}



#include <iostream>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <nlohmann/json.hpp>

Logger::Logger(const LoggingConfig& config) : config_(config) {
    // Create logs directory if it doesn't exist
    std::filesystem::path log_path(config.file);
    std::filesystem::create_directories(log_path.parent_path());
    
    // Open log file
    current_log_file_ = config.file;
    log_file_.open(current_log_file_, std::ios::app);
    
    last_rotation_ = std::chrono::system_clock::now();
    set_level(config.level);
    
    log_info("Logger initialized");
}

Logger::~Logger() {
    if (log_file_.is_open()) {
        log_file_.close();
    }
}

void Logger::log_inference(const InferenceResult& result) {
    if (!should_log("INFO")) return;
    
    std::string message = format_inference_result(result);
    write_log("INFO", message);
}

void Logger::log_frame(const Frame& frame, const std::string& status) {
    if (!should_log("DEBUG")) return;
    
    std::string message = format_frame_info(frame, status);
    write_log("DEBUG", message);
}

void Logger::log_error(const std::string& error) {
    if (!should_log("ERROR")) return;
    
    write_log("ERROR", error);
}

void Logger::log_info(const std::string& message) {
    if (!should_log("INFO")) return;
    
    write_log("INFO", message);
}

void Logger::log_warning(const std::string& message) {
    if (!should_log("WARNING")) return;
    
    write_log("WARNING", message);
}

void Logger::log_debug(const std::string& message) {
    if (!should_log("DEBUG")) return;
    
    write_log("DEBUG", message);
}

void Logger::set_level(const std::string& level) {
    log_level_ = level_to_int(level);
}

bool Logger::should_log(const std::string& level) const {
    return level_to_int(level) >= log_level_;
}

void Logger::write_log(const std::string& level, const std::string& message) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    
    rotate_log_if_needed();
    
    std::string timestamp = get_timestamp();
    std::string log_entry = "[" + timestamp + "] [" + level + "] " + message;
    
    // Write to file
    if (log_file_.is_open()) {
        log_file_ << log_entry << std::endl;
        log_file_.flush();
    }
    
    // Write to console if enabled
    if (config_.console_output) {
        std::cout << log_entry << std::endl;
    }
}

void Logger::rotate_log_if_needed() {
    if (!config_.rotate_daily) {
        return;
    }
    
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);
    
    // Check if we need to rotate (new day)
    static int last_day = -1;
    if (last_day == -1) {
        last_day = tm.tm_mday;
        return;
    }
    
    if (tm.tm_mday != last_day) {
        last_day = tm.tm_mday;
        
        // Close current file
        if (log_file_.is_open()) {
            log_file_.close();
        }
        
        // Create new filename with date
        std::stringstream ss;
        ss << std::put_time(&tm, "%Y%m%d");
        std::string date_str = ss.str();
        
        std::filesystem::path log_path(config_.file);
        std::string new_filename = log_path.parent_path().string() + "/" + 
                                 log_path.stem().string() + "_" + date_str + 
                                 log_path.extension().string();
        
        // Open new file
        current_log_file_ = new_filename;
        log_file_.open(current_log_file_, std::ios::app);
        
        log_info("Log rotated to: " + new_filename);
    }
}

std::string Logger::get_timestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    auto tm = *std::localtime(&time_t);
    
    std::stringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    ss << "." << std::setfill('0') << std::setw(3) << ms.count();
    
    return ss.str();
}

std::string Logger::format_inference_result(const InferenceResult& result) const {
    nlohmann::json j;
    j["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        result.timestamp.time_since_epoch()).count();
    j["camera_id"] = result.camera_id;
    j["frame_id"] = result.frame_id;
    j["plate_text"] = result.plate_text;
    j["confidence"] = result.confidence;
    j["bbox"] = {
        {"x", result.bbox.x},
        {"y", result.bbox.y},
        {"width", result.bbox.width},
        {"height", result.bbox.height}
    };
    j["vehicle_color"] = result.vehicle_color;
    j["vehicle_type"] = result.vehicle_type;
    
    return "INFERENCE: " + j.dump();
}

std::string Logger::format_frame_info(const Frame& frame, const std::string& status) const {
    nlohmann::json j;
    j["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        frame.timestamp.time_since_epoch()).count();
    j["camera_id"] = frame.camera_id;
    j["frame_id"] = frame.frame_id;
    j["status"] = status;
    j["image_size"] = {
        {"width", frame.image.cols},
        {"height", frame.image.rows}
    };
    
    return "FRAME: " + j.dump();
}

int Logger::level_to_int(const std::string& level) const {
    if (level == "DEBUG") return 0;
    if (level == "INFO") return 1;
    if (level == "WARNING") return 2;
    if (level == "ERROR") return 3;
    return 1; // Default to INFO
}


