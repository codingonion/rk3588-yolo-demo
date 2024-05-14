//
// Created by kaylor on 3/4/24.
//

#include "image_process.h"

#include "kaylordut/log/logger.h"
#define N_CLASS_COLORS (20)
unsigned char class_colors[][3] = {
    {255, 56, 56},    // 'FF3838'
    {255, 157, 151},  // 'FF9D97'
    {255, 112, 31},   // 'FF701F'
    {255, 178, 29},   // 'FFB21D'
    {207, 210, 49},   // 'CFD231'
    {72, 249, 10},    // '48F90A'
    {146, 204, 23},   // '92CC17'
    {61, 219, 134},   // '3DDB86'
    {26, 147, 52},    // '1A9334'
    {0, 212, 187},    // '00D4BB'
    {44, 153, 168},   // '2C99A8'
    {0, 194, 255},    // '00C2FF'
    {52, 69, 147},    // '344593'
    {100, 115, 255},  // '6473FF'
    {0, 24, 236},     // '0018EC'
    {132, 56, 255},   // '8438FF'
    {82, 0, 133},     // '520085'
    {203, 56, 255},   // 'CB38FF'
    {255, 149, 200},  // 'FF95C8'
    {255, 55, 199}    // 'FF37C7'
};

ImageProcess::ImageProcess(int width, int height, int target_size) {
  scale_ = static_cast<double>(target_size) / std::max(height, width);
  padding_x_ = target_size - static_cast<int>(width * scale_);
  padding_y_ = target_size - static_cast<int>(height * scale_);
  new_size_ = cv::Size(static_cast<int>(width * scale_),
                       static_cast<int>(height * scale_));
  target_size_ = target_size;
  letterbox_.scale = scale_;
  letterbox_.x_pad = padding_x_ / 2;
  letterbox_.y_pad = padding_y_ / 2;
}

std::unique_ptr<cv::Mat> ImageProcess::Convert(const cv::Mat &src) {
  if (&src == nullptr) {
    return nullptr;
  }
  cv::Mat resize_img;
  cv::resize(src, resize_img, new_size_);
  auto square_img = std::make_unique<cv::Mat>(
      target_size_, target_size_, src.type(), cv::Scalar(114, 114, 114));
  cv::Point position(padding_x_ / 2, padding_y_ / 2);
  resize_img.copyTo((*square_img)(
      cv::Rect(position.x, position.y, resize_img.cols, resize_img.rows)));
  return std::move(square_img);
}

const letterbox_t &ImageProcess::get_letter_box() { return letterbox_; }

void ImageProcess::ImagePostProcess(cv::Mat &image,
                                    object_detect_result_list &od_results) {
  KAYLORDUT_LOG_INFO("ImagePostProcess is called");
  if (od_results.count >= 1) {
    int width = image.rows;
    int height = image.cols;
    auto *ori_img = image.ptr();
    int cls_id = od_results.results[0].cls_id;
    uint8_t *seg_mask = od_results.results_seg[0].seg_mask;
    float alpha = 0.5f;  // opacity
    if (seg_mask != nullptr) {
      for (int j = 0; j < height; j++) {
        for (int k = 0; k < width; k++) {
          int pixel_offset = 3 * (j * width + k);
          if (seg_mask[j * width + k] != 0) {
            ori_img[pixel_offset + 0] = (unsigned char) clamp(
                class_colors[seg_mask[j * width + k] % N_CLASS_COLORS][0] *
                    (1 - alpha) +
                    ori_img[pixel_offset + 0] * alpha,
                0, 255);  // r
            ori_img[pixel_offset + 1] = (unsigned char) clamp(
                class_colors[seg_mask[j * width + k] % N_CLASS_COLORS][1] *
                    (1 - alpha) +
                    ori_img[pixel_offset + 1] * alpha,
                0, 255);  // g
            ori_img[pixel_offset + 2] = (unsigned char) clamp(
                class_colors[seg_mask[j * width + k] % N_CLASS_COLORS][2] *
                    (1 - alpha) +
                    ori_img[pixel_offset + 2] * alpha,
                0, 255);  // b
          }
        }
      }
      free(seg_mask);
    }
  }
  KAYLORDUT_LOG_INFO("model type is {}", od_results.model_type);
  if (od_results.model_type == ModelType::DETECTION) {
    ProcessDetectionImage(image, od_results);
  } else if (od_results.model_type == ModelType::OBB) {
    ProcessOBBImage(image, od_results);
  }
}

void DrawRotatedRect(cv::Mat &image,
                     float x,
                     float y,
                     float w,
                     float h,
                     float theta,
                     const cv::Scalar &color,
                     int thickness) {
  // 定义旋转矩形的中心，尺寸和旋转角度
  cv::Point2f center(x, y);
  cv::Size2f size(w, h);

  // 创建旋转矩形对象
  cv::RotatedRect rotatedRect(center, size, theta);

  // 获取矩形的四个顶点
  cv::Point2f vertices[4];
  rotatedRect.points(vertices);

  // 绘制矩形的四条边
  for (int i = 0; i < 4; i++) {
    cv::line(image, vertices[i], vertices[(i + 1) % 4], color, thickness);
  }
}

void ImageProcess::ProcessOBBImage(cv::Mat &image, const object_detect_result_list &od_results) const {
  KAYLORDUT_LOG_INFO("ImageProcess::ProcessOBBImage is called, result count is {}",od_results.count);
  for (int i = 0; i < od_results.count; ++i) {
    auto obb_result = od_results.results_obb[i];
    KAYLORDUT_LOG_INFO("{} @ xywhθ = ({} {} {} {} {}) {}",
                       coco_cls_to_name(obb_result.cls_id),
                       obb_result.box.x,
                       obb_result.box.y,
                       obb_result.box.w,
                       obb_result.box.h,
                       obb_result.box.theta * 180.0 / CV_PI,
                       obb_result.prop);
    DrawRotatedRect(image,
                    obb_result.box.x,
                    obb_result.box.y,
                    obb_result.box.w,
                    obb_result.box.h,
                    obb_result.box.theta * 180.0 / CV_PI,
                    cv::Scalar(0, 255, 0),
                    2);
  }
}

void ImageProcess::ProcessDetectionImage(cv::Mat &image, object_detect_result_list &od_results) const {
  for (int i = 0; i < od_results.count; ++i) {
    object_detect_result *detect_result = &(od_results.results[i]);
    //    if (strcmp(coco_cls_to_name(detect_result->cls_id), "person") == 0){
    //    continue;}
    KAYLORDUT_LOG_INFO("{} @ ({} {} {} {}) {}",
                       coco_cls_to_name(detect_result->cls_id),
                       detect_result->box.left, detect_result->box.top,
                       detect_result->box.right, detect_result->box.bottom,
                       detect_result->prop);
    cv::rectangle(
        image, cv::Point(detect_result->box.left, detect_result->box.top),
        cv::Point(detect_result->box.right, detect_result->box.bottom),
        cv::Scalar(0, 0, 255), 2);
    char text[256];
    sprintf(text, "%s %.1f%%", coco_cls_to_name(detect_result->cls_id),
            detect_result->prop * 100);
    cv::putText(image, text,
                cv::Point(detect_result->box.left, detect_result->box.top + 20),
                cv::FONT_HERSHEY_COMPLEX, 1, cv::Scalar(255, 0, 0), 2,
                cv::LINE_8);
  }
}
