#ifndef CVHELPER_H
#define CVHELPER_H

#include <opencv2/opencv.hpp>
#include <QImage>
#include <QtDebug>

QDebug operator<<(QDebug d, const cv::Size& ss);

namespace CVHELPER {
  // Ratio tinggi border terhadap ukuran image max(x, y)
  constexpr float BORDER_TO_IMAGE_RATIO = 1.0f / 12.0f;
  constexpr float BORDER_PIXEL_TO_IMAGE_RATIO = 8.5f;
  constexpr int MINIMUM_IMAGE_DIMENSION_BOTH = 400;
  
  std::vector<cv::Point> scaledContour(const std::vector<cv::Point>& before, float scale);
  cv::Mat blend(const cv::Mat& background_image, const cv::Mat& foreground_image);
  cv::Mat cropToAlpha(const cv::Mat& source, int margin = 0);
  cv::Mat borderize(const cv::Mat& aa);
  QImage qImageFromMat(const cv::Mat& mat);
  cv::Mat matFromQImage(const QImage& image);
  bool matOk(const cv::Mat& mat, int matType=CV_8UC4);
  template<int toType>
    cv::Mat convertTo(const cv::Mat& input) {
        if (!matOk(input, input.type())) {
            return cv::Mat();
        }
        if (input.type() == toType) {
            return input.clone();
        }
        cv::Mat output;
        if (toType == CV_8UC4) {
            if (input.type() == CV_8UC3) {
                cv::cvtColor(input, output, cv::COLOR_BGR2BGRA);
            } else if (input.type() == CV_8UC1) {
                cv::cvtColor(input, output, cv::COLOR_GRAY2BGRA);
            } else if (input.type() == CV_16UC1) {
                cv::Mat temp;
                input.convertTo(temp, CV_8U, 255.0 / 65535.0);
                cv::cvtColor(temp, output, cv::COLOR_GRAY2BGRA);
            } else if (input.type() == CV_32FC1) {
                cv::Mat temp;
                input.convertTo(temp, CV_8U, 255.0);
                cv::cvtColor(temp, output, cv::COLOR_GRAY2BGRA);
            } else {
                return cv::Mat();
            }
        } else {
            input.convertTo(output, toType);
        }
        return output;
    }
};

#endif // CVHELPER_H