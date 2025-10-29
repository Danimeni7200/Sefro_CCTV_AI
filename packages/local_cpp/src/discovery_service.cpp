#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include "health_server.hpp"

// Discovery handler function
std::string discovery_handler(const std::string& query) {
    // Parse query parameters
    auto get_param = [&](const std::string& key) -> std::string {
        auto pos = query.find(key + "=");
        if (pos == std::string::npos) return {};
        auto start = pos + key.size() + 1;
        auto end = query.find('&', start);
        return query.substr(start, end == std::string::npos ? std::string::npos : end - start);
    };
    
    std::string ip = get_param("ip");
    std::string user = get_param("user");
    std::string pass = get_param("pass");
    std::string brand = get_param("brand");
    
    if (ip.empty() || user.empty()) {
        return "{\"success\":false,\"error\":\"ip and user required\"}";
    }
    
    // Build candidate RTSP URLs (brand-first)
    std::vector<std::string> candidates;
    auto add = [&](const std::string& path) {
        candidates.emplace_back("rtsp://" + user + ":" + pass + "@" + ip + ":554/" + path);
    };
    
    if (brand == "reolink") {
        add("h264Preview_01_sub");
        add("h264Preview_01_main");
    } else if (brand == "hikvision") {
        add("Streaming/Channels/101");
        add("Streaming/Channels/102");
    } else if (brand == "dahua") {
        add("cam/realmonitor?channel=1&subtype=1");
        add("cam/realmonitor?channel=1&subtype=0");
    }
    
    // Common fallbacks
    add("stream1");
    add("live/ch00_0");
    add("avstream/channel=1");
    
    // Build JSON response
    std::string json = "{\"success\":true,\"candidates\":[";
    for (size_t i = 0; i < candidates.size(); ++i) {
        json += "\"" + candidates[i] + "\"";
        if (i + 1 < candidates.size()) json += ",";
    }
    json += "]}";
    
    return json;
}

int main() {
    std::cout << "Starting C++ Discovery Service..." << std::endl;
    
    // Create health config
    HealthConfig config;
    config.bind_address = "127.0.0.1";
    config.port = 8086;
    config.enabled = true;
    
    // Create health server
    HealthServer server(config);
    
    // Set discovery handler
    server.set_discover_handler(discovery_handler);
    
    // Start server
    if (!server.start()) {
        std::cerr << "Failed to start discovery service" << std::endl;
        return 1;
    }
    
    std::cout << "C++ Discovery Service started on port 8086" << std::endl;
    std::cout << "Press Ctrl+C to stop..." << std::endl;
    
    // Keep running
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    return 0;
}