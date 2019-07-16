
#include <cstdio>
#include <string>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>

typedef unsigned char uint8;

int saveImg(uint8* img, int height, int width, int channel, std::string filename) {
    cv::Mat data = cv::Mat(height, width, CV_8UC3, img);
    cv::imwrite(filename, data);
    printf("Saved image to %s\n", filename.c_str());
}

int readImg(uint8*& img, int& height, int& width, int& channel, char* filename) {
    IplImage* imgIpl = cvLoadImage(filename, 1);
    cv::Mat matimg = cv::cvarrToMat(imgIpl);
    
    height = matimg.rows;
    width = matimg.cols;
    channel = matimg.channels();

    img = new uint8[matimg.rows*matimg.cols*matimg.channels()];
    for (int i=0; i<matimg.rows; i++) {
        for (int j=0; j<matimg.cols; j++) {
            cv::Vec3b* data = matimg.ptr<cv::Vec3b>(i);
            // printf("data at (%d,%d): %d, %d, %d\n", i, j, data[j].val[0], data[j].val[1], data[j].val[2]);
            for (int c=0; c<matimg.channels(); c++) {
                img[matimg.channels()*(i*matimg.cols+j)+c] = data[j].val[c];
            }
        }
    }
    printf("Image loaded. height=%d, width=%d, channel=%d\n", 
        matimg.rows, matimg.cols, matimg.channels());
}

// int main(int argc, char* argv[]) {
//     uint8* img;
//     int height, width, channel;
//     readImg(img, height, width, channel, argv[1]);
//     printf("Loaded image. height=%d, width=%d, channel=%d.\n", height, width, channel);
//     saveImg(img, height, width, channel, argv[2]);
//     return 0;

// }