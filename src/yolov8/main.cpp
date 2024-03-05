#include "videofile.h"
#include "yolov8.h"
#include "image_process.h"
#include "kaylordut/log/logger.h"
int main(int argc, char *agrv[]) {
  Yolov8 yolov_8("./yolov8n.rknn");
//  Yolov8 yolov_8("./yolov8s-seg.rknn");
  VideoFile video_file(agrv[1]);
//    video_file.Display(125, 640);
  ImageProcess image_process(video_file.get_frame_width(), video_file.get_frame_height(), 640);
  auto original_img = video_file.GetNextFrame();
  auto convert_img = image_process.Convert(*original_img);
  cv::Mat rgb_img = cv::Mat::zeros(640, 640, convert_img->type());
  cv::namedWindow("Video", cv::WINDOW_AUTOSIZE);
  while (convert_img != nullptr) {
    cv::cvtColor(*convert_img, rgb_img, cv::COLOR_BGR2RGB);
    object_detect_result_list od_results;
    yolov_8.Inference(rgb_img.ptr(),  &od_results, image_process.get_letter_box());

    image_process.ImagePostProcess(*original_img, od_results);
    cv::imshow("Video", *original_img);
    cv::waitKey(1);
    original_img = video_file.GetNextFrame();
    convert_img = image_process.Convert(*original_img);
  }
  KAYLORDUT_LOG_INFO("exit loop");
  cv::destroyAllWindows();
  return 0;
}
