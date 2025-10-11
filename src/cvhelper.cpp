#include <opencv2/opencv.hpp>
#include <QtDebug>
#include "cvhelper.hpp"

QDebug operator<<(QDebug d, const cv::Size& ss) {
  d << "Size:" << ss.width << "x" << ss.height;
  return d;
}

std::vector<cv::Point> CVHELPER::scaledContour(const std::vector<cv::Point>& before, float scale) {
    // Check for valid input
    if (before.empty() || scale == 0.0f) {
        return before; // Return unchanged contour if invalid
    }

    // Compute centroid
    cv::Moments m = cv::moments(before);
    double cx = m.m10 / m.m00; // Centroid x
    double cy = m.m01 / m.m00; // Centroid y

    // Create new contour
    std::vector<cv::Point> scaled_contour(before.size());
    
    // Scale each point relative to centroid
    for (size_t i = 0; i < before.size(); ++i) {
        float dx = before[i].x - cx; // Distance from centroid x
        float dy = before[i].y - cy; // Distance from centroid y
        scaled_contour[i].x = static_cast<int>(cx + dx * scale); // Scale and shift back
        scaled_contour[i].y = static_cast<int>(cy + dy * scale);
    }

    return scaled_contour;
}

cv::Mat CVHELPER::blend(const cv::Mat& background_image, const cv::Mat& foreground_image) {
    // Validate inputs
    if (background_image.empty() || foreground_image.empty()) {
        return cv::Mat();
    }
    if (foreground_image.channels() != 3 && foreground_image.channels() != 4) {
        throw std::invalid_argument("Foreground image must be 3-channel BGR or 4-channel BGRA");
    }
    if (background_image.channels() != 3 && background_image.channels() != 4) {
        throw std::invalid_argument("Background image must be 3-channel BGR or 4-channel BGRA");
    }

    // Convert background to BGRA if needed
    cv::Mat bg_bgra;
    if (background_image.channels() == 3) {
        cv::cvtColor(background_image, bg_bgra, cv::COLOR_BGR2BGRA);
    } else {
        bg_bgra = background_image.clone();
    }

    // Convert foreground to BGRA if needed
    cv::Mat fg_bgra;
    if (foreground_image.channels() == 3) {
        cv::cvtColor(foreground_image, fg_bgra, cv::COLOR_BGR2BGRA);
    } else {
        fg_bgra = foreground_image.clone();
    }

    // Create output image (BGRA, same size as background)
    cv::Mat result = bg_bgra.clone();

    // Determine region of interest (ROI) for foreground
    int fg_width = std::min(foreground_image.cols, background_image.cols);
    int fg_height = std::min(foreground_image.rows, background_image.rows);
    cv::Rect roi(0, 0, fg_width, fg_height);

    // Get ROI for blending
    cv::Mat fg_roi = fg_bgra(cv::Rect(0, 0, fg_width, fg_height));
    cv::Mat bg_roi = result(roi);

    // Blend pixel by pixel, respecting alpha channels
    for (int y = 0; y < fg_height; ++y) {
        for (int x = 0; x < fg_width; ++x) {
            // Get pixel values (BGRA)
            cv::Vec4b fg_pixel = fg_roi.at<cv::Vec4b>(y, x);
            cv::Vec4b& bg_pixel = bg_roi.at<cv::Vec4b>(y, x);

            // Normalize alpha values to [0, 1]
            float fg_alpha = (fg_bgra.channels() == 4) ? fg_pixel[3] / 255.0f : 1.0f; // Foreground alpha (1.0 if no alpha channel)
            float bg_alpha = bg_pixel[3] / 255.0f; // Background alpha

            // Compute output alpha
            float out_alpha = fg_alpha + bg_alpha * (1.0f - fg_alpha);
            if (out_alpha == 0) {
                out_alpha = 1.0f; // Avoid division by zero
            }

            // Blend colors (BGR) using alpha compositing
            for (int c = 0; c < 3; ++c) { // B, G, R channels
                float blended = (fg_alpha * fg_pixel[c] + bg_alpha * (1.0f - fg_alpha) * bg_pixel[c]) / out_alpha;
                bg_pixel[c] = cv::saturate_cast<uchar>(blended);
            }
            bg_pixel[3] = cv::saturate_cast<uchar>(out_alpha * 255.0f); // Set output alpha
        }
    }

    return result;
}

cv::Mat CVHELPER::cropToAlpha(const cv::Mat& source, int margin) {
    // Validate input
    if (source.empty()) {
        return cv::Mat();
    }
    if (source.channels() != 4) {
        throw std::invalid_argument("Source image must be 4-channel BGRA");
    }
    if (margin < 0) {
        margin = 0; // Ensure non-negative margin
    }

    // Extract alpha channel
    cv::Mat alpha;
    cv::extractChannel(source, alpha, 3); // Get alpha channel (index 3 in BGRA)

    // Find non-zero alpha pixels
    std::vector<cv::Point> non_zero_points;
    cv::findNonZero(alpha, non_zero_points);

    // If no non-transparent pixels, return empty Mat
    if (non_zero_points.empty()) {
        return cv::Mat();
    }

    // Compute bounding rectangle of non-transparent pixels
    int min_x = source.cols, min_y = source.rows, max_x = -1, max_y = -1;
    for (const auto& point : non_zero_points) {
        min_x = std::min(min_x, point.x);
        min_y = std::min(min_y, point.y);
        max_x = std::max(max_x, point.x);
        max_y = std::max(max_y, point.y);
    }
    
    // Apply margin, ensuring bounds stay within image
    min_x = std::max(0, min_x - margin);
    min_y = std::max(0, min_y - margin);
    max_x = std::min(source.cols - 1, max_x + margin);
    max_y = std::min(source.rows - 1, max_y + margin);

    // Compute ROI
    cv::Rect roi(min_x, min_y, max_x - min_x + 1, max_y - min_y + 1);
    
    // check wether image dimension is too small
    if(roi.width < MINIMUM_IMAGE_DIMENSION_BOTH || roi.height < MINIMUM_IMAGE_DIMENSION_BOTH) {
      float scale_factor_x, scale_factor_y, scale_factor;
      scale_factor_x = static_cast<float>(MINIMUM_IMAGE_DIMENSION_BOTH) / roi.width;
      scale_factor_y = static_cast<float>(MINIMUM_IMAGE_DIMENSION_BOTH) / roi.height;
      scale_factor = std::max(scale_factor_x, scale_factor_y);
      cv::Mat resized;
      cv::resize(source, resized, cv::Size(), scale_factor, scale_factor, cv::INTER_LANCZOS4);
      return cropToAlpha(resized);
    }
    // Return cropped image
    return source(roi).clone();
}

void showInCheckerBoard(const cv::Mat& img, const char name[] = "Checkerboard Display", int checkerSize = 20) {
    // Validate input
    if (img.empty()) {
        throw std::invalid_argument("Input image is empty");
    }
    if (img.channels() != 4) {
        throw std::invalid_argument("Input image must be 4-channel BGRA");
    }
    if (checkerSize <= 0) {
        checkerSize = 20; // Ensure positive checker size
    }

    // Create checkerboard background
    int rows = img.rows;
    int cols = img.cols;
    cv::Mat checkerboard(rows, cols, CV_8UC3, cv::Scalar(0, 0, 0)); // BGR background

    // Define checkerboard colors (light and dark gray)
    cv::Scalar color1(200, 200, 200); // Light gray
    cv::Scalar color2(100, 100, 100); // Dark gray

    // Draw checkerboard pattern
    for (int y = 0; y < rows; y += checkerSize) {
        for (int x = 0; x < cols; x += checkerSize) {
            // Determine checker color based on position
            bool isLight = ((x / checkerSize) + (y / checkerSize)) % 2 == 0;
            cv::Scalar color = isLight ? color1 : color2;

            // Define square region (clipped to image bounds)
            int width = std::min(checkerSize, cols - x);
            int height = std::min(checkerSize, rows - y);
            cv::Rect square(x, y, width, height);

            // Fill square with color
            checkerboard(square).setTo(color);
        }
    }

    // Convert checkerboard to BGRA for blending
    cv::Mat checkerboard_bgra;
    cv::cvtColor(checkerboard, checkerboard_bgra, cv::COLOR_BGR2BGRA);

    // Blend image over checkerboard using alpha channel
    cv::Mat result = checkerboard_bgra.clone();
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            cv::Vec4b pixel = img.at<cv::Vec4b>(y, x);
            float alpha = pixel[3] / 255.0f; // Normalize alpha
            cv::Vec4b& result_pixel = result.at<cv::Vec4b>(y, x);

            // Blend BGR channels
            for (int c = 0; c < 3; ++c) { // B, G, R
                result_pixel[c] = cv::saturate_cast<uchar>(alpha * pixel[c] + (1.0f - alpha) * result_pixel[c]);
            }
            result_pixel[3] = 255; // Fully opaque result
        }
    }

    // Display the result
    cv::imshow(name, result);
}

using contour_t = std::vector<cv::Point>;
using contours_t = std::vector<contour_t>;

/*
cv::Mat CVHELPER::borderize(const cv::Mat& aa) {
  auto img = cropToAlpha(aa);
  auto imgSize = img.size();
  
  // add 2 * BORDER_TO_IMAGE_RATIO
  int maximized = std::max(imgSize.width, imgSize.height) + (2 * BORDER_TO_IMAGE_RATIO);
  auto requiredSize = cv::Size(maximized, maximized);
  
  cv::Mat transparent(requiredSize, img.type(), cv::Scalar(255, 255, 255, 0));
  
  int x_offset = (int) ( maximized - imgSize.width / 2 );
  int y_offset = (int) ( maximized - imgSize.height / 2 );
  
  cv::Rect roi(x_offset, y_offset, img.cols, img.rows);
  cv::Mat transROI = transparent(roi);
  img.copyTo(transROI);
  img = transparent;
  
  cv::Mat alpha_source;
  cv::extractChannel(img, alpha_source, 3);
  
  // update imgSize
  imgSize = img.size();
  
  contours_t contours;
  cv::findContours(alpha_source, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE);
  cv::Mat newBG(imgSize, img.type(), cv::Scalar(255, 255, 255, 0));
  cv::drawContours(newBG, contours, -1, cv::Scalar(255, 255, 255, 255),
        maximized * BORDER_PIXEL_TO_IMAGE_RATIO, cv::FILLED);
  cv::drawContours(newBG, contours, -1, 
        cv::Scalar(255, 255, 255, 255), cv::FILLED);

  auto last = blend(newBG, img);
  
  cv::Mat last_alpha, blured_alpha;
  cv::extractChannel(newBG, last_alpha, 3);
  cv::erode(last_alpha, last_alpha, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(4, 4)), {-1, -1}, 2);
  cv::GaussianBlur(last_alpha, blured_alpha, {}, 1.5, 1.5, cv::BORDER_DEFAULT);
  std::vector<cv::Mat> fixed;
  cv::split(last, fixed);
  cv::Mat onlyRGB;
  // cv::merge(&last, 3, onlyRGB);
  
  // showInCheckerBoard(onlyRGB, "RGB Only"); 
  fixed[3] = blured_alpha;
  cv::merge(fixed, last);
  
  return last;
}
*/

cv::Mat CVHELPER::borderize(const cv::Mat& aa) {
    auto img = cropToAlpha(aa);
    auto imgSize = img.size();

    // Calculate the border size based on BORDER_TO_IMAGE_RATIO
    int borderSize = (int)(std::max(imgSize.width, imgSize.height) * BORDER_TO_IMAGE_RATIO * 2);
    int maximized = std::max(imgSize.width, imgSize.height) + borderSize;
    auto requiredSize = cv::Size(maximized, maximized);
    // qDebug() << "Add Border Width" << borderSize;

    // Create transparent matrix
    cv::Mat transparent(requiredSize, img.type(), cv::Scalar(255, 255, 255, 0));

    // Center the image in the transparent matrix
    int x_offset = (maximized - imgSize.width) / 2;
    int y_offset = (maximized - imgSize.height) / 2;

    // Ensure offsets are non-negative and ROI is valid
    x_offset = std::max(0, x_offset);
    y_offset = std::max(0, y_offset);
    cv::Rect roi(x_offset, y_offset, img.cols, img.rows);

    // Check if ROI is within bounds
    if (roi.x + roi.width > transparent.cols || roi.y + roi.height > transparent.rows) {
        throw std::runtime_error("ROI exceeds transparent matrix dimensions");
    }

    cv::Mat transROI = transparent(roi);
    img.copyTo(transROI);
    img = transparent;

    // Rest of your code remains unchanged
    cv::Mat alpha_source;
    cv::extractChannel(img, alpha_source, 3);

    imgSize = img.size();

    contours_t contours;
    cv::findContours(alpha_source, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_TC89_KCOS);
    cv::Mat newBG(imgSize, img.type(), cv::Scalar(255, 255, 255, 0));
    cv::drawContours(newBG, contours, -1, cv::Scalar(255, 255, 255, 255),
                      maximized / BORDER_PIXEL_TO_IMAGE_RATIO, cv::FILLED);
    // qDebug() << "Border Brush" << maximized / BORDER_PIXEL_TO_IMAGE_RATIO;
    cv::drawContours(newBG, contours, -1, cv::Scalar(255, 255, 255, 255), cv::FILLED);
    
    // showInCheckerBoard(newBG, "newBG");
    auto last = blend(newBG, img);

    cv::Mat last_alpha, blured_alpha;
    cv::extractChannel(newBG, last_alpha, 3);
    cv::GaussianBlur(last_alpha, blured_alpha, {9, 9}, 2.8, 2.8, cv::BORDER_REPLICATE);
    cv::erode(blured_alpha, blured_alpha, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(9, 9)), {-1, -1}, 3, cv::BORDER_REPLICATE);
    // cv::dilate(last_alpha, last_alpha, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3)), {-1, -1}, 7, cv::BORDER_REPLICATE);
    std::vector<cv::Mat> fixed;
    cv::split(last, fixed);
    cv::Mat onlyRGB;

    fixed[3] = blured_alpha;
    cv::merge(fixed, last);
    return last;
}

QImage CVHELPER::qImageFromMat(const cv::Mat& mat){
    // Handle different Mat types
    switch (mat.type()) {
        case CV_8UC1: // Grayscale image
        {
            QImage image(mat.cols, mat.rows, QImage::Format_Grayscale8);
            for (int y = 0; y < mat.rows; ++y) {
                memcpy(image.scanLine(y), mat.ptr(y), mat.cols);
            }
            return image;
        }
        case CV_8UC3: // RGB image
        {
            // Convert BGR to RGB
            cv::Mat rgb;
            cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
            QImage image(rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format_RGB888);
            return image.rgbSwapped();
        }
        case CV_8UC4: // RGBA image
        {
            QImage image(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_RGBA8888);
            return image.rgbSwapped();
        }
        default:
            // Unsupported format
            return QImage();
    }
}

cv::Mat CVHELPER::matFromQImage(const QImage& image)
{
    // Ensure image is not null
    if (image.isNull()) {
        return cv::Mat();
    }

    // Handle different QImage formats
    switch (image.format()) {
        case QImage::Format_Grayscale8:
        {
            cv::Mat mat(image.height(), image.width(), CV_8UC1);
            for (int y = 0; y < image.height(); ++y) {
                memcpy(mat.ptr(y), image.scanLine(y), image.width());
            }
            return mat;
        }
        case QImage::Format_RGB888:
        {
            cv::Mat mat(image.height(), image.width(), CV_8UC3);
            QImage rgbImage = image.rgbSwapped(); // Convert to BGR for OpenCV
            for (int y = 0; y < image.height(); ++y) {
                memcpy(mat.ptr(y), rgbImage.scanLine(y), image.width() * 3);
            }
            return mat;
        }
        case QImage::Format_ARGB32:
        case QImage::Format_RGBA8888:
        {
            cv::Mat mat(image.height(), image.width(), CV_8UC4);
            QImage rgbaImage = image.convertToFormat(QImage::Format_RGBA8888).rgbSwapped();
            for (int y = 0; y < image.height(); ++y) {
                memcpy(mat.ptr(y), rgbaImage.scanLine(y), image.width() * 4);
            }
            return mat;
        }
        default:
            // Convert to a supported format if possible
            QImage converted = image.convertToFormat(QImage::Format_RGB888);
            if (converted.isNull()) {
                return cv::Mat();
            }
            cv::Mat mat(converted.height(), converted.width(), CV_8UC3);
            converted = converted.rgbSwapped(); // Convert to BGR for OpenCV
            for (int y = 0; y < converted.height(); ++y) {
                memcpy(mat.ptr(y), converted.scanLine(y), converted.width() * 3);
            }
            return mat;
    }
}

bool CVHELPER::matOk(const cv::Mat& mat, int expectedType) {
        return !mat.empty() && mat.data != nullptr && mat.type() == expectedType;
    }
