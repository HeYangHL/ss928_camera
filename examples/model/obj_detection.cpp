#include "obj_detection.hpp"

void Obj_Detec::swapYUV_I420toNV12(unsigned char *i420bytes, unsigned char *nv12bytes, int width, int height)
{
    int nLenY = width * height;
    int nLenU = nLenY / 4;
    memcpy(nv12bytes, i420bytes, width * height);
    for (int i = 0; i < nLenU; i++)
    {
        nv12bytes[nLenY + 2 * i] = i420bytes[nLenY + i];             // U
        nv12bytes[nLenY + 2 * i + 1] = i420bytes[nLenY + nLenU + i]; // V
    }
}

void Obj_Detec::BGR2YUV_nv12(cv::Mat src, cv::Mat &dst)
{
    int w_img = src.cols;
    int h_img = src.rows;
    dst = cv::Mat(h_img * 1.5, w_img, CV_8UC1, cv::Scalar(0));
    cv::Mat src_YUV_I420(h_img * 1.5, w_img, CV_8UC1, cv::Scalar(0)); // YUV_I420
    cvtColor(src, src_YUV_I420, COLOR_BGR2YUV_I420);
    swapYUV_I420toNV12(src_YUV_I420.data, dst.data, w_img, h_img);
}

void Obj_Detec::draw(cv::Mat image, std::vector<objinfo> output)
{

    float w = float(image.cols) / 640;
    float h = float(image.rows) / 640;

    for (int i = 0; i < output.size(); i++)
    {
        cv::Point p1, p2;
        p1.x = output[i].x1 * w;
        p1.y = output[i].y1 * h;
        p2.x = output[i].x2 * w;
        p2.y = output[i].y2 * h;

        // printf("x1 : %f, y1 : %f, x2 : %f, y2 : %f\n", output[i].x1, output[i].y1, output[i].x2, output[i].y2);

        // printf("1 x : %d, y : %d\n", p1.x, p1.y);
        // printf("2 x : %d, y : %d\n", p2.x, p2.y);
        cv::rectangle(image, p1, p2, cv::Scalar(0, 0, 255), 2);

        char name[50];
        // sprintf(name, "%.1d_%.3f", output[i].classNum, output[i].score);
        sprintf(name, "%d_%.3f", output[i].classNum, output[i].score);
        cv::putText(image, name, cv::Point(p1.x, p1.y),
                    cv::FONT_HERSHEY_COMPLEX, 1, cv::Scalar(0, 0, 255));
    }
    std::ostringstream name;
    name << "output_" << output.size() << ".jpg";
    cv::imwrite(name.str(), image);
}

int main(void)
{
    Obj_Detec obj;
    int ret = 0;
    SVP_NNN svp;
    uint32_t g_modelWidth = 640;
    uint32_t g_modelHeight = 640;
    std::string VL_imgname = "./711_VL_1012.jpeg";

#ifdef PROC_OLD
    const char *det_vl_modelPath = "./model/yolov5s_yuv_original.om";
    ret = svp.sd3403_rfcn_svp_npu_model_init(det_vl_modelPath);

#else

    std::string det_vl_modelPath = "./model/yolov5s_yuv_original.om";

    ret = svp.sd3403_rfcn_svp_npu_model_init();
    ret = svp.sd3403_rfcn_init_ai(det_vl_modelPath);
#endif

    cv::Mat img = cv::imread(VL_imgname, 1);

    cv::Mat resizeImg;
    cv::resize(img, resizeImg, cv::Size(g_modelWidth, g_modelHeight));

    cv::Mat NV12Mat;
    obj.BGR2YUV_nv12(resizeImg, NV12Mat);

    unsigned char *data = NULL;
    data = NV12Mat.data;
    // int fd = open("out_711_VL_1012.yuv", O_CREAT | O_RDWR);
    // write(fd, data, 614400);
    // close(fd);
        std::vector<objinfo> output;
#ifdef PROC_OLD
        ret = svp.sd3403_rfcn_svp_npu_model_inference(data, output);
#else
        std::string rpn_file = "./model/yolov5_rpn.txt";
        ret = svp.sd3403_rfcn_svp_npu_model_detcection(data, rpn_file, output);
#endif
        printf("output num : %d\n", output.size());
        obj.draw(img, output);
        sleep(1);
    // printf("6666666666666666666666666666\n");

    // while(1);
#ifdef PROC_OLD
    ret = svp.sd3403_rfcn_svp_npu_model_deinit();
#else
    ret = svp.sd33403_rfcn_destroy_input();
    svp.sd3403_rfcn_destroy_resource();
#endif

    return 0;
}