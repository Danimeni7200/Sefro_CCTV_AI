#include <opencv2/opencv.hpp>
#include <iostream>

int main() {
    std::cout << "OpenCV version: " << CV_VERSION << std::endl;
    cv::Mat test_image(100, 100, CV_8UC3, cv::Scalar(0,0,0));
    std::cout << "Created test image " << test_image.size() << std::endl;
    return 0;
}
