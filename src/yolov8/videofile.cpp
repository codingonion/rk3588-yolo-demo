//
// Created by kaylor on 3/4/24.
//

#include "videofile.h"
#include "kaylordut/log/logger.h"

VideoFile::VideoFile(const std::string &&filename) : filename_(filename) {
  capture_ = new cv::VideoCapture(filename_);
  if (!capture_->isOpened()) {
    KAYLORDUT_LOG_ERROR("Error opening video file");
    exit(EXIT_FAILURE);
  }
}

VideoFile::~VideoFile() {
  if (capture_ != nullptr) {
    KAYLORDUT_LOG_INFO("Release capture")
    capture_->release();
    delete capture_;
  }
}

void VideoFile::Display(const float framerate, const int target_size) {
  const int delay = 1000 / framerate;
  cv::namedWindow("Video", cv::WINDOW_AUTOSIZE);
  cv::Mat frame;
  ImageProcess
      image_process(capture_->get(cv::CAP_PROP_FRAME_WIDTH), capture_->get(cv::CAP_PROP_FRAME_HEIGHT), target_size);
  while (true) {
    *capture_ >> frame;
    if (frame.empty()) { break; }
    cv::imshow("Video", *(image_process.Convert(frame)));
//    cv::imshow("Video", frame);
    if (cv::waitKey(delay) >= 0) { break; }
  }
  cv::destroyAllWindows();
}

std::shared_ptr<cv::Mat> VideoFile::GetNextFrame(const int target_size) {
  static ImageProcess
      image_process(capture_->get(cv::CAP_PROP_FRAME_WIDTH), capture_->get(cv::CAP_PROP_FRAME_HEIGHT), target_size);
  cv::Mat frame;
  *capture_ >> frame;
  if (frame.empty()){return nullptr;}
  return std::move(image_process.Convert(frame));
}