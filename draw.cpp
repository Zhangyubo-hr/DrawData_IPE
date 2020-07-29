#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/videoio.hpp"
#include <sys/time.h>

#include "config.h"
#include "draw.h"

int main()
{
    // ------ 变量初始化 ------
    const char *filename = FILENAME;
    const char *videoname = VIDEONAME;
    FILE *fp = fopen(filename, "rb");
    short int *data;
    short int database[TOTAL_NUM_FRAME][20];
    struct timeval start, end;
    int timeuse = -1;

    // ------ 读数据 ------
    // 计时器开始
    gettimeofday(&start, NULL);
    // 预读取所有数据
    for (int i = 0; i < TOTAL_NUM_FRAME;)
        readData(database[i++], fp);
    // 计时器暂停
    gettimeofday(&end, NULL);
    timeuse = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
    printf("Data read at %d us.\n", timeuse);

#if (DISP_METHOD == 3)
    // ------ 求解圆的中心点和半径 ------
    short cicle_center[TOTAL_NUM_FRAME][2];
    short cicle_radius[TOTAL_NUM_FRAME];
    short text_x[TOTAL_NUM_FRAME], text_y[TOTAL_NUM_FRAME];
    // 计时器开始
    gettimeofday(&start, NULL);
    for (int i = 0; i < TOTAL_NUM_FRAME;)
    {
        // 取第几个演员的坐标
        short *pdata = database[i] + 4*(ACTOR_NUM - 1);
        // 获取框的中心点；使用移位运算极限加速
        cicle_center[i][1] = (*pdata + *(pdata + 2)) >> 1;       // 水平方向
        cicle_center[i][0] = (*(pdata + 1) + *(pdata + 3)) >> 1; // 竖直方向
        // 获取圆的半径，并量化为整数
        cicle_radius[i++] = (short)Eclidian(*(pdata + 2) - *pdata, *(pdata + 3) - *(pdata + 1)) >> 1;
    }
    // 计时器暂停
    gettimeofday(&end, NULL);
    timeuse = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
    printf("Calculate radius at %d us.\n", timeuse);

    // ------ 平滑操作 ------
    // 计时器开始
    gettimeofday(&start, NULL);
    for (short i = 0; ++i < TOTAL_NUM_FRAME;)
    {
        // 对半径的容差，注意变大和变小是分别处理的
        short _diff = cicle_radius[i] - cicle_radius[i - 1];
        // 半径最多增加多少
        short difference = cicle_radius[i - 1] >> 4; // 最多允许比上一帧增加1/16
        if (_diff > difference)
            cicle_radius[i] = cicle_radius[i - 1] + difference;
        else
        {
            // 半径最多减少多少(注意要用负的)
            difference = -(cicle_radius[i - 1] >> 5); // 最多允许比上一帧减少1/32
            if (_diff < difference)
                cicle_radius[i] = cicle_radius[i - 1] + difference;
        }
        // 对圆心位置的容差
        // 竖直方向平移量
        difference = 3;
        if (L1_norm(cicle_center[i][0], cicle_center[i - 1][0]) > difference)
            cicle_center[i][0] = cicle_center[i - 1][0] + ((cicle_center[i][0] > cicle_center[i - 1][0]) ? difference : (0 - difference));
        // 水平方向平移量，因为体操运动员运动速度快所以应该设置大一点
        difference = 20;
        if (L1_norm(cicle_center[i][1], cicle_center[i - 1][1]) > difference)
            cicle_center[i][1] = cicle_center[i - 1][1] + ((cicle_center[i][1] > cicle_center[i - 1][1]) ? difference : (0 - difference));

        // 提前获取运动员编号的文字显示位置初始值
        text_x[i] = cicle_center[i][1]; // 水平方向
        // 竖直方向稍微比圆的顶部向下一点点
        text_y[i] = cicle_center[i][0] - cicle_radius[i] + (cicle_radius[i] >> 2);
    }
    // 对文字位置的平滑操作，设置关键帧的距离smooth_d
    short smooth_d = 15;
    // 水平方向
    for (short i = 1; ++i < (TOTAL_NUM_FRAME / smooth_d);)
    {
        // 代入一次方程拟合
        float gradient = (float)(text_x[smooth_d * i] - text_x[smooth_d * i - smooth_d]) / smooth_d;
        for (register short j = -smooth_d; ++j < 0;)
            text_x[smooth_d * i + j] = text_x[smooth_d * i] + (short int)(j * gradient);
    }
    // 竖直方向
    smooth_d = 30;
    for (short i = 1; ++i < (TOTAL_NUM_FRAME / smooth_d);)
    {
        float gradient = (float)(text_y[smooth_d * i] - text_y[smooth_d * i - smooth_d]) / smooth_d;
        for (register short j = -smooth_d; ++j < 0;)
            text_y[smooth_d * i + j] = text_y[smooth_d * i] + (short int)(j * gradient);
    }
    // 计时器暂停
    gettimeofday(&end, NULL);
    timeuse = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
    printf("Smooth at %d us.\n", timeuse);
#endif

    cv::Mat image;
    cv::VideoCapture capture;

    // ------ 打开视频文件 ------
    capture.open(videoname);
#if SAVE_VIDEO
    // 从原视频直接获取fps
    double fps = capture.get(cv::CAP_PROP_FPS);
    // 保持视频尺寸(1920*1080)
    cv::Size videoSize(capture.get(cv::CAP_PROP_FRAME_WIDTH), capture.get(cv::CAP_PROP_FRAME_HEIGHT));
    // 创建写入视频文件
    cv::VideoWriter writer("output.mp4", cv::VideoWriter::fourcc(FOUR_CC), fps, videoSize);
#endif
    if (capture.isOpened())
    {
        // 计时器开始
        gettimeofday(&start, NULL);
        for (int count_frame = 0; count_frame < TOTAL_NUM_FRAME;)
        {
            capture >> image;
            // 导入当前的坐标
            data = database[count_frame];
            // 读到的坐标是(x1,y1,x2,y2)格式
            if (image.empty())
            {
                fclose(fp);
                cv::waitKey(0);
                break;
            }
#if (DISP_METHOD == 1)
            for (int i = 0; i < 5; i++)
            {
                // Method 1: 直接显示框
                short *pdata = data + i * 4;
                // 定义矩形框
                cv::Rect rect(*pdata, *(pdata + 1), *(pdata + 2) - *pdata, *(pdata + 3) - *(pdata + 1));
                // 绘制框
                cv::rectangle(image, rect, cv::Scalar(64 + i * 48, 128 + i * 32, 192 - i * 48, (double)0.5), 5, 8, 0);
                // image.at<u_char>(0,0,0) = 1;
            }
#else
#if (DISP_METHOD == 2)
            for (int i = 0; i < 5; i++)
            {
                // Method 2: 画圆
                short *pdata = data + i * 4;
                // 获取框的中心点
                short cx = (*pdata + *(pdata + 2)) / 2;
                short cy = (*(pdata + 1) + *(pdata + 3)) / 2;
                // 获取圆的半径
                double radius = 0.5 * Eclidian(*(pdata + 2) - *pdata, *(pdata + 3) - *(pdata + 1));
                cv::circle(image, cv::Point(cx, cy), radius, cv::Scalar(0, 255, 0), 3);
            }
#else
#if (DISP_METHOD == 3)
            // Method 3: Attention
            // 对图像进行逐像素处理
            if (image.isContinuous())
            {
                // Mat在内存空间中连续，可用快速算法
                // 其实不连续也可以用copy()函数，但是这样做没啥实际意义
                uchar *img_ptr = reinterpret_cast<uchar *>(image.data);
                // 使用register变量，极限加速
                for (register short xi = 0; xi < image.rows; xi++)
                {
                    if (L1_norm(xi, cicle_center[count_frame][0]) < cicle_radius[count_frame])
                    {
                        for (register short yi = 0; yi < image.cols; yi++)
                        {
                            // 判断是否在圆的外切矩形框内
                            if (L1_norm(yi, cicle_center[count_frame][1]) < cicle_radius[count_frame])
                            {
                                // 设置倍率
                                float dist = Eclidian(xi - cicle_center[count_frame][0], yi - cicle_center[count_frame][1]);
                                float rate = MIN(3.5 - 3 * MIN(dist / cicle_radius[count_frame], 1), 1);
                                // 对3个channel写入内存地址
                                for (register uchar _c = 0; _c++ < 3;)
                                    // 赋值运算右边执行之后左边的++返回的是加之前的结果
                                    *(img_ptr++) = (uchar)(rate * (float)(*img_ptr));
                            }
                            else
                            {
                                for (register uchar _c = 0; _c++ < 3;)
                                    // 使用移位运算直接二进制操作
                                    *(img_ptr++) = (*img_ptr) >> 1;
                            }
                        }
                    }
                    else
                        // 剩下的都是背景
                        for (register int yi = 0; yi++ < image.cols;)
                            for (register uchar _c = 0; _c++ < 3;)
                                *(img_ptr++) = (*img_ptr) >> 1;
                }
            }
            else
            {
                printf("Warning: You are using a slow algorithm.\n");
                // 传统方法：遍历所有像素，并设置像素值
                // 时间：快速算法约18ms一张图，传统方法大约63ms
                for (int xi = 0; xi < image.rows;)
                {
                    //获取第i行首像素指针
                    cv::Vec3b *p = image.ptr<cv::Vec3b>(xi++);
                    for (int yi = 0; yi < image.cols; ++yi)
                    {
                        // 设置倍率
                        float dist = Eclidian(xi - cicle_center[count_frame][0], yi - cicle_center[count_frame][0]);
                        float rate = MIN(3.5 - 3 * MIN(dist / cicle_radius[count_frame], 1), 1);
                        // 对内存地址的数据直接进行修改
                        for (int _channel = 0; _channel < 3; ++_channel)
                            // BGR
                            p[yi][_channel] = (unsigned char)(rate * (float)p[yi][_channel]);
                    }
                }
            }
            // 绘制人头顶上的数字
            char text_n[2];
            sprintf(text_n, "%d", ACTOR_NUM);
            cv::putText(image, text_n, cv::Point(text_x[count_frame], text_y[count_frame]), cv::FONT_HERSHEY_COMPLEX, 1, cv::Scalar(255, 255, 255), 2);
            cv::putText(image, text_n, cv::Point(text_x[count_frame], text_y[count_frame]), cv::FONT_HERSHEY_COMPLEX, 1, cv::Scalar(0, 255, 0), 1);
#endif
#endif
#endif
#if FRAME_NUM_DISP
            // 显示左上角的帧数
            char text_t[16];
            sprintf(text_t, "Frame %d", count_frame);
            cv::putText(image, text_t, cv::Point(50, 50), cv::FONT_HERSHEY_COMPLEX, 1, cv::Scalar(255, 255, 255), 2);
            cv::putText(image, text_t, cv::Point(50, 50), cv::FONT_HERSHEY_COMPLEX, 1, cv::Scalar(0, 255, 0), 1);
#endif
            // 计时器结束
            gettimeofday(&end, NULL);
            timeuse = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
            // 给出了时间开销下限，防止计算用时除以0报错浮点数例外
            timeuse = MAX(timeuse, (1 << 10) + 1);
#if (!SAVE_VIDEO) or DISPLAY_VIDEO
            // 只有在不保存视频的情况下才判断是否显示视频，否则强制显示
            imshow(WINDOW_TITLE, image);
#endif
#if SAVE_VIDEO
            writer << image;
#endif
#if (!SAVE_VIDEO) or DISPLAY_VIDEO
            if (cv::waitKey(1) >= 0)
            {
                fclose(fp);
                break;
            }
#endif
            // 打印状态信息
            StatusBar(count_frame++, timeuse);
        }
    }
    return 0;
}