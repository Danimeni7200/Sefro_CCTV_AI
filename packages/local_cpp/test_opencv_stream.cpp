#include <iostream>
#include <opencv2/opencv.hpp>
#include <string>
#include <thread>
#include <chrono>

int main() {
    std::cout << "Testing OpenCV RTSP stream capture..." << std::endl;
    std::cout << "Stream URL: rtsp://admin:test1234@192.168.4.252:554/h264Preview_01_sub" << std::endl;
    std::cout << std::endl;
    
    cv::VideoCapture cap;
    std::string url = "rtsp://admin:test1234@192.168.4.252:554/h264Preview_01_sub";
    
    std::cout << "Opening stream..." << std::endl;
    cap.open(url);
    
    if (!cap.isOpened()) {
        std::cerr << "ERROR: Failed to open stream!" << std::endl;
        std::cerr << "OpenCV build info:" << std::endl;
        std::cout << cv::getBuildInformation() << std::endl;
        return 1;
    }
    
    std::cout << "✓ Stream opened successfully!" << std::endl;
    std::cout << "Capture properties:" << std::endl;
    std::cout << "  Width: " << cap.get(cv::CAP_PROP_FRAME_WIDTH) << std::endl;
    std::cout << "  Height: " << cap.get(cv::CAP_PROP_FRAME_HEIGHT) << std::endl;
    std::cout << "  FPS: " << cap.get(cv::CAP_PROP_FPS) << std::endl;
    std::cout << std::endl;
    
    // Try to read 5 frames
    std::cout << "Reading 5 frames..." << std::endl;
    for (int i = 0; i < 5; i++) {
        cv::Mat frame;
        if (cap.read(frame) && !frame.empty()) {
            std::cout << "✓ Frame " << i+1 << ": " << frame.cols << "x" << frame.rows << std::endl;
        } else {
            std::cout << "✗ Frame " << i+1 << ": Failed to read" << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    std::cout << std::endl;
    std::cout << "Test completed!" << std::endl;
    
    cap.release();
    return 0;
}


