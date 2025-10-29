#include "config.hpp"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <chrono>

Config::Config() = default;

Config::Config(const Config& other) {
    stream = other.stream;
    ai_service = other.ai_service;
    pipeline = other.pipeline;
    preprocessing = other.preprocessing;
    privacy = other.privacy;
    logging = other.logging;
    health = other.health;
    config_changed.store(other.config_changed.load());
    on_config_changed = other.on_config_changed;
    watching_.store(false);
    config_file_.clear();
}

Config& Config::operator=(const Config& other) {
    if (this == &other) return *this;
    stream = other.stream;
    ai_service = other.ai_service;
    pipeline = other.pipeline;
    preprocessing = other.preprocessing;
    privacy = other.privacy;
    logging = other.logging;
    health = other.health;
    config_changed.store(other.config_changed.load());
    on_config_changed = other.on_config_changed;
    watching_.store(false);
    config_file_.clear();
    return *this;
}

Config::~Config() {
    stop_watch();
}

bool Config::load(const std::string& filepath) {
    try {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "Failed to open config file: " << filepath << std::endl;
            return false;
        }
        
        nlohmann::json j;
        file >> j;
        from_json(j);
        
        config_file_ = filepath;
        last_modified_ = std::filesystem::last_write_time(filepath);
        
        std::cout << "Config loaded from: " << filepath << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading config: " << e.what() << std::endl;
        return false;
    }
}

bool Config::save(const std::string& filepath) const {
    try {
        std::ofstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "Failed to open config file for writing: " << filepath << std::endl;
            return false;
        }
        
        nlohmann::json j = to_json();
        file << j.dump(2);
        
        std::cout << "Config saved to: " << filepath << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error saving config: " << e.what() << std::endl;
        return false;
    }
}

void Config::start_watch(const std::string& filepath) {
    if (watching_.load()) {
        stop_watch();
    }
    
    config_file_ = filepath;
    watching_ = true;
    watch_thread_ = std::thread(&Config::watch_loop, this);
}

void Config::stop_watch() {
    if (watching_.load()) {
        watching_ = false;
        if (watch_thread_.joinable()) {
            watch_thread_.join();
        }
    }
}

void Config::watch_loop() {
    while (watching_.load()) {
        if (check_file_changed()) {
            if (load(config_file_)) {
                config_changed = true;
                if (on_config_changed) {
                    on_config_changed();
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

bool Config::check_file_changed() {
    try {
        auto current_modified = std::filesystem::last_write_time(config_file_);
        if (current_modified > last_modified_) {
            last_modified_ = current_modified;
            return true;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error checking file modification: " << e.what() << std::endl;
    }
    return false;
}

nlohmann::json Config::to_json() const {
    return nlohmann::json{
        {"stream", {
            {"url", stream.url},
            {"camera_id", stream.camera_id},
            {"fps_cap", stream.fps_cap},
            {"reconnect_delay_ms", stream.reconnect_delay_ms},
            {"max_reconnect_attempts", stream.max_reconnect_attempts},
            {"use_hardware_decode", stream.use_hardware_decode}
        }},
        {"ai_service", {
            {"host", ai_service.host},
            {"timeout_ms", ai_service.timeout_ms},
            {"retry_count", ai_service.retry_count},
            {"retry_delay_ms", ai_service.retry_delay_ms}
        }},
        {"pipeline", {
            {"queue_size", pipeline.queue_size},
            {"drop_policy", pipeline.drop_policy},
            {"max_inference_queue", pipeline.max_inference_queue}
        }},
        {"preprocessing", {
            {"target_width", preprocessing.target_width},
            {"target_height", preprocessing.target_height},
            {"letterbox", preprocessing.letterbox},
            {"gamma", preprocessing.gamma},
            {"denoise", preprocessing.denoise},
            {"sharpen", preprocessing.sharpen},
            {"quality_threshold", preprocessing.quality_threshold}
        }},
        {"privacy", {
            {"mask_plate_on_storage", privacy.mask_plate_on_storage},
            {"anonymize", privacy.anonymize},
            {"store_original_image", privacy.store_original_image}
        }},
        {"logging", {
            {"level", logging.level},
            {"file", logging.file},
            {"rotate_daily", logging.rotate_daily},
            {"console_output", logging.console_output}
        }},
        {"health", {
            {"port", health.port},
            {"bind_address", health.bind_address},
            {"metrics_interval_ms", health.metrics_interval_ms}
        }}
    };
}

void Config::from_json(const nlohmann::json& j) {
    if (j.contains("stream")) {
        const auto& s = j["stream"];
        if (s.contains("url")) stream.url = s["url"];
        if (s.contains("camera_id")) stream.camera_id = s["camera_id"];
        if (s.contains("fps_cap")) stream.fps_cap = s["fps_cap"];
        if (s.contains("reconnect_delay_ms")) stream.reconnect_delay_ms = s["reconnect_delay_ms"];
        if (s.contains("max_reconnect_attempts")) stream.max_reconnect_attempts = s["max_reconnect_attempts"];
        if (s.contains("use_hardware_decode")) stream.use_hardware_decode = s["use_hardware_decode"];
    }
    
    if (j.contains("ai_service")) {
        const auto& a = j["ai_service"];
        if (a.contains("host")) ai_service.host = a["host"];
        if (a.contains("timeout_ms")) ai_service.timeout_ms = a["timeout_ms"];
        if (a.contains("retry_count")) ai_service.retry_count = a["retry_count"];
        if (a.contains("retry_delay_ms")) ai_service.retry_delay_ms = a["retry_delay_ms"];
    }
    
    if (j.contains("pipeline")) {
        const auto& p = j["pipeline"];
        if (p.contains("queue_size")) pipeline.queue_size = p["queue_size"];
        if (p.contains("drop_policy")) pipeline.drop_policy = p["drop_policy"];
        if (p.contains("max_inference_queue")) pipeline.max_inference_queue = p["max_inference_queue"];
    }
    
    if (j.contains("preprocessing")) {
        const auto& pp = j["preprocessing"];
        if (pp.contains("target_width")) preprocessing.target_width = pp["target_width"];
        if (pp.contains("target_height")) preprocessing.target_height = pp["target_height"];
        if (pp.contains("letterbox")) preprocessing.letterbox = pp["letterbox"];
        if (pp.contains("gamma")) preprocessing.gamma = pp["gamma"];
        if (pp.contains("denoise")) preprocessing.denoise = pp["denoise"];
        if (pp.contains("sharpen")) preprocessing.sharpen = pp["sharpen"];
        if (pp.contains("quality_threshold")) preprocessing.quality_threshold = pp["quality_threshold"];
    }
    
    if (j.contains("privacy")) {
        const auto& pr = j["privacy"];
        if (pr.contains("mask_plate_on_storage")) privacy.mask_plate_on_storage = pr["mask_plate_on_storage"];
        if (pr.contains("anonymize")) privacy.anonymize = pr["anonymize"];
        if (pr.contains("store_original_image")) privacy.store_original_image = pr["store_original_image"];
    }
    
    if (j.contains("logging")) {
        const auto& l = j["logging"];
        if (l.contains("level")) logging.level = l["level"];
        if (l.contains("file")) logging.file = l["file"];
        if (l.contains("rotate_daily")) logging.rotate_daily = l["rotate_daily"];
        if (l.contains("console_output")) logging.console_output = l["console_output"];
    }
    
    if (j.contains("health")) {
        const auto& h = j["health"];
        if (h.contains("port")) health.port = h["port"];
        if (h.contains("bind_address")) health.bind_address = h["bind_address"];
        if (h.contains("metrics_interval_ms")) health.metrics_interval_ms = h["metrics_interval_ms"];
    }
}

#include <iostream>
#include <filesystem>
#include <chrono>

Config::Config() = default;

Config::Config(const Config& other) {
    stream = other.stream;
    ai_service = other.ai_service;
    pipeline = other.pipeline;
    preprocessing = other.preprocessing;
    privacy = other.privacy;
    logging = other.logging;
    health = other.health;
    config_changed.store(other.config_changed.load());
    on_config_changed = other.on_config_changed;
    watching_.store(false);
    config_file_.clear();
}

Config& Config::operator=(const Config& other) {
    if (this == &other) return *this;
    stream = other.stream;
    ai_service = other.ai_service;
    pipeline = other.pipeline;
    preprocessing = other.preprocessing;
    privacy = other.privacy;
    logging = other.logging;
    health = other.health;
    config_changed.store(other.config_changed.load());
    on_config_changed = other.on_config_changed;
    watching_.store(false);
    config_file_.clear();
    return *this;
}

Config::~Config() {
    stop_watch();
}

bool Config::load(const std::string& filepath) {
    try {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "Failed to open config file: " << filepath << std::endl;
            return false;
        }
        
        nlohmann::json j;
        file >> j;
        from_json(j);
        
        config_file_ = filepath;
        last_modified_ = std::filesystem::last_write_time(filepath);
        
        std::cout << "Config loaded from: " << filepath << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading config: " << e.what() << std::endl;
        return false;
    }
}

bool Config::save(const std::string& filepath) const {
    try {
        std::ofstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "Failed to open config file for writing: " << filepath << std::endl;
            return false;
        }
        
        nlohmann::json j = to_json();
        file << j.dump(2);
        
        std::cout << "Config saved to: " << filepath << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error saving config: " << e.what() << std::endl;
        return false;
    }
}

void Config::start_watch(const std::string& filepath) {
    if (watching_.load()) {
        stop_watch();
    }
    
    config_file_ = filepath;
    watching_ = true;
    watch_thread_ = std::thread(&Config::watch_loop, this);
}

void Config::stop_watch() {
    if (watching_.load()) {
        watching_ = false;
        if (watch_thread_.joinable()) {
            watch_thread_.join();
        }
    }
}

void Config::watch_loop() {
    while (watching_.load()) {
        if (check_file_changed()) {
            if (load(config_file_)) {
                config_changed = true;
                if (on_config_changed) {
                    on_config_changed();
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

bool Config::check_file_changed() {
    try {
        auto current_modified = std::filesystem::last_write_time(config_file_);
        if (current_modified > last_modified_) {
            last_modified_ = current_modified;
            return true;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error checking file modification: " << e.what() << std::endl;
    }
    return false;
}

nlohmann::json Config::to_json() const {
    return nlohmann::json{
        {"stream", {
            {"url", stream.url},
            {"camera_id", stream.camera_id},
            {"fps_cap", stream.fps_cap},
            {"reconnect_delay_ms", stream.reconnect_delay_ms},
            {"max_reconnect_attempts", stream.max_reconnect_attempts},
            {"use_hardware_decode", stream.use_hardware_decode}
        }},
        {"ai_service", {
            {"host", ai_service.host},
            {"timeout_ms", ai_service.timeout_ms},
            {"retry_count", ai_service.retry_count},
            {"retry_delay_ms", ai_service.retry_delay_ms}
        }},
        {"pipeline", {
            {"queue_size", pipeline.queue_size},
            {"drop_policy", pipeline.drop_policy},
            {"max_inference_queue", pipeline.max_inference_queue}
        }},
        {"preprocessing", {
            {"target_width", preprocessing.target_width},
            {"target_height", preprocessing.target_height},
            {"letterbox", preprocessing.letterbox},
            {"gamma", preprocessing.gamma},
            {"denoise", preprocessing.denoise},
            {"sharpen", preprocessing.sharpen},
            {"quality_threshold", preprocessing.quality_threshold}
        }},
        {"privacy", {
            {"mask_plate_on_storage", privacy.mask_plate_on_storage},
            {"anonymize", privacy.anonymize},
            {"store_original_image", privacy.store_original_image}
        }},
        {"logging", {
            {"level", logging.level},
            {"file", logging.file},
            {"rotate_daily", logging.rotate_daily},
            {"console_output", logging.console_output}
        }},
        {"health", {
            {"port", health.port},
            {"bind_address", health.bind_address},
            {"metrics_interval_ms", health.metrics_interval_ms}
        }}
    };
}

void Config::from_json(const nlohmann::json& j) {
    if (j.contains("stream")) {
        const auto& s = j["stream"];
        if (s.contains("url")) stream.url = s["url"];
        if (s.contains("camera_id")) stream.camera_id = s["camera_id"];
        if (s.contains("fps_cap")) stream.fps_cap = s["fps_cap"];
        if (s.contains("reconnect_delay_ms")) stream.reconnect_delay_ms = s["reconnect_delay_ms"];
        if (s.contains("max_reconnect_attempts")) stream.max_reconnect_attempts = s["max_reconnect_attempts"];
        if (s.contains("use_hardware_decode")) stream.use_hardware_decode = s["use_hardware_decode"];
    }
    
    if (j.contains("ai_service")) {
        const auto& a = j["ai_service"];
        if (a.contains("host")) ai_service.host = a["host"];
        if (a.contains("timeout_ms")) ai_service.timeout_ms = a["timeout_ms"];
        if (a.contains("retry_count")) ai_service.retry_count = a["retry_count"];
        if (a.contains("retry_delay_ms")) ai_service.retry_delay_ms = a["retry_delay_ms"];
    }
    
    if (j.contains("pipeline")) {
        const auto& p = j["pipeline"];
        if (p.contains("queue_size")) pipeline.queue_size = p["queue_size"];
        if (p.contains("drop_policy")) pipeline.drop_policy = p["drop_policy"];
        if (p.contains("max_inference_queue")) pipeline.max_inference_queue = p["max_inference_queue"];
    }
    
    if (j.contains("preprocessing")) {
        const auto& pp = j["preprocessing"];
        if (pp.contains("target_width")) preprocessing.target_width = pp["target_width"];
        if (pp.contains("target_height")) preprocessing.target_height = pp["target_height"];
        if (pp.contains("letterbox")) preprocessing.letterbox = pp["letterbox"];
        if (pp.contains("gamma")) preprocessing.gamma = pp["gamma"];
        if (pp.contains("denoise")) preprocessing.denoise = pp["denoise"];
        if (pp.contains("sharpen")) preprocessing.sharpen = pp["sharpen"];
        if (pp.contains("quality_threshold")) preprocessing.quality_threshold = pp["quality_threshold"];
    }
    
    if (j.contains("privacy")) {
        const auto& pr = j["privacy"];
        if (pr.contains("mask_plate_on_storage")) privacy.mask_plate_on_storage = pr["mask_plate_on_storage"];
        if (pr.contains("anonymize")) privacy.anonymize = pr["anonymize"];
        if (pr.contains("store_original_image")) privacy.store_original_image = pr["store_original_image"];
    }
    
    if (j.contains("logging")) {
        const auto& l = j["logging"];
        if (l.contains("level")) logging.level = l["level"];
        if (l.contains("file")) logging.file = l["file"];
        if (l.contains("rotate_daily")) logging.rotate_daily = l["rotate_daily"];
        if (l.contains("console_output")) logging.console_output = l["console_output"];
    }
    
    if (j.contains("health")) {
        const auto& h = j["health"];
        if (h.contains("port")) health.port = h["port"];
        if (h.contains("bind_address")) health.bind_address = h["bind_address"];
        if (h.contains("metrics_interval_ms")) health.metrics_interval_ms = h["metrics_interval_ms"];
    }
}
