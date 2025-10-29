#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <thread>
#include <chrono>

// Simple HTTP client without CURL dependency
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

class SimpleHttpClient {
public:
    SimpleHttpClient() {
#ifdef _WIN32
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    }
    
    ~SimpleHttpClient() {
#ifdef _WIN32
        WSACleanup();
#endif
    }
    
    std::string sendRequest(const std::string& host, int port, const std::string& path, bool isPost = false) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            std::cerr << "Failed to create socket" << std::endl;
            return "";
        }
        
        sockaddr_in server;
        server.sin_family = AF_INET;
        server.sin_port = htons(port);
        
#ifdef _WIN32
        server.sin_addr.s_addr = inet_addr(host.c_str());
#else
        inet_aton(host.c_str(), &server.sin_addr);
#endif
        
        if (connect(sock, (sockaddr*)&server, sizeof(server)) < 0) {
            std::cerr << "Failed to connect to " << host << ":" << port << std::endl;
#ifdef _WIN32
            closesocket(sock);
#else
            close(sock);
#endif
            return "";
        }
        
        std::string request;
        if (isPost) {
            request = "POST " + path + " HTTP/1.1\r\n";
        } else {
            request = "GET " + path + " HTTP/1.1\r\n";
        }
        request += "Host: " + host + ":" + std::to_string(port) + "\r\n";
        request += "Connection: close\r\n\r\n";
        
        send(sock, request.c_str(), request.length(), 0);
        
        std::string response;
        char buffer[4096];
        int bytesReceived;
        
        // Read all data
        while ((bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
            buffer[bytesReceived] = '\0';
            response += buffer;
        }
        
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
        
        return response;
    }
};

int main(int argc, char* argv[]) {
    std::cout << "=== C++ Stream Test Client ===" << std::endl;
    std::cout << "Testing discovery_only.exe streaming endpoints..." << std::endl;
    std::cout << std::endl;
    
    SimpleHttpClient client;
    
    // Test 1: Health check
    std::cout << "1. Testing /health endpoint..." << std::endl;
    std::string healthResponse = client.sendRequest("127.0.0.1", 8086, "/health");
    std::cout << "Response: " << healthResponse.substr(0, 200) << std::endl;
    std::cout << std::endl;
    
    // Test 2: Add a stream
    std::cout << "2. Adding stream to service..." << std::endl;
    std::string rtspUrl = "rtsp://admin:test1234@192.168.4.252:554/h264Preview_01_sub";
    std::string streamId = "test_stream_123";
    std::string encodedUrl = "rtsp%3A%2F%2Fadmin%3Atest1234%40192.168.4.252%3A554%2Fh264Preview_01_sub";
    std::string addStreamPath = "/add_stream?id=" + streamId + "&url=" + encodedUrl;
    
    std::string addResponse = client.sendRequest("127.0.0.1", 8086, addStreamPath, true);
    std::cout << "Add stream response: " << addResponse.substr(0, 300) << std::endl;
    std::cout << std::endl;
    
    // Wait for the stream to start
    std::cout << "3. Waiting 5 seconds for stream to initialize..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    // Test 3: Try to fetch a frame
    std::cout << "4. Fetching frame from stream..." << std::endl;
    std::string framePath = "/stream/" + streamId;
    std::string frameResponse = client.sendRequest("127.0.0.1", 8086, framePath);
    
    if (frameResponse.length() > 100) {
        std::cout << "✓ Success! Received " << frameResponse.length() << " bytes" << std::endl;
        std::cout << "Response preview: " << frameResponse.substr(0, 200) << std::endl;
        
        // Save to file
        std::ofstream outfile("stream_frame.txt", std::ios::binary);
        if (outfile.is_open()) {
            outfile.write(frameResponse.c_str(), frameResponse.length());
            outfile.close();
            std::cout << "Response saved to stream_frame.txt" << std::endl;
        }
    } else {
        std::cout << "✗ Failed to fetch frame" << std::endl;
        std::cout << "Response: " << frameResponse << std::endl;
    }
    
    std::cout << std::endl;
    std::cout << "Test completed!" << std::endl;
    
    return 0;
}


