#ifndef _OBJ_DETECTION_HPP_
#define _OBJ_DETECTION_HPP_

#include "opencv2/opencv.hpp"
#include "opencv2/imgproc.hpp"
#include "svp_npu.hpp"
#include <string>

using namespace cv;

class Obj_Detec{
public:
    void swapYUV_I420toNV12(unsigned char *i420bytes, unsigned char *nv12bytes, int width, int height);
    void BGR2YUV_nv12(cv::Mat src, cv::Mat &dst);
    void draw(cv::Mat image, std::vector<objinfo> output);
};


#endif