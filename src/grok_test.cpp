#include <opencv2/opencv.hpp>
#include <iostream>

int main() {
    // Load the image with alpha channel
    cv::Mat img = cv::imread("test_data/01.png", cv::IMREAD_UNCHANGED);
    if (img.empty()) {
        std::cout << "Error: Could not load image!" << std::endl;
        return -1;
    }

    // Check if the image has an alpha channel
    if (img.channels() != 4) {
        std::cout << "Error: Image does not have an alpha channel!" << std::endl;
        return -1;
    }

    // Split into color (BGR) and alpha channels
    std::vector<cv::Mat> channels;
    cv::split(img, channels);
    cv::Mat alpha = channels[3]; // Alpha channel (0-255)

    // Create a binary mask from the alpha channel (threshold at 1)
    cv::Mat mask;
    cv::threshold(alpha, mask, 1, 255, cv::THRESH_BINARY);
    mask.convertTo(mask, CV_8UC1); // Ensure mask is single-channel

    // Dilate the mask to create a border (adjust kernel size for thicker border)
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));
    cv::Mat dilatedMask;
    cv::dilate(mask, dilatedMask, kernel);
    dilatedMask.convertTo(dilatedMask, CV_8UC1); // Ensure dilatedMask is single-channel

    // Create a white background (3 channels for BGR)
    cv::Mat whiteBg = cv::Mat::ones(img.size(), CV_8UC3) * 255;

    // Create the border by subtracting the original mask from the dilated mask
    cv::Mat border;
    cv::subtract(dilatedMask, mask, border); // Both should be single-channel
    border.convertTo(border, CV_8UC1); // Ensure border is single-channel

    // Convert image to 3 channels (remove alpha) for consistency
    cv::Mat imgBGR;
    cv::cvtColor(img, imgBGR, cv::COLOR_BGRA2BGR);

    // Apply the image to the white background using the mask
    cv::Mat result = whiteBg.clone();
    imgBGR.copyTo(result, mask);

    // Create a colored border (3 channels)
    cv::Mat borderColored = cv::Mat::zeros(img.size(), CV_8UC3);
    borderColored.setTo(cv::Scalar(0, 0, 255), border); // Red border (BGR)

    // Add the border to the result
    cv::addWeighted(result, 1.0, borderColored, 1.0, 0.0, result);

    // Save the result
    cv::imwrite("bordered_01.png", result);

    std::cout << "Image processed and saved as bordered_02.png" << std::endl;
    return 0;
}