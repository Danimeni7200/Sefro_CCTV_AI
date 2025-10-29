#include <iostream>
#include <string>
#include <curl/curl.h>
#include <sstream>

// Callback function to write response data
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* response) {
    size_t total_size = size * nmemb;
    response->append((char*)contents, total_size);
    return total_size;
}

int main() {
    CURL* curl;
    CURLcode res;
    std::string response;

    // Initialize curl
    curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Failed to initialize CURL" << std::endl;
        return 1;
    }

    // Set up the request
    std::string url = "http://127.0.0.1:8086/discover?ip=192.168.4.252&user=admin&pass=test1234&brand=reolink";
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    // Perform the request
    std::cout << "Sending discovery request to C++ service..." << std::endl;
    res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        std::cerr << "CURL error: " << curl_easy_strerror(res) << std::endl;
        curl_easy_cleanup(curl);
        return 1;
    }

    // Get response code
    long response_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    
    std::cout << "Response Code: " << response_code << std::endl;
    std::cout << "Response Body: " << response << std::endl;

    // Cleanup
    curl_easy_cleanup(curl);
    return 0;
}