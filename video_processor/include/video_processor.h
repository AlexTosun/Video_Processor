#ifndef VIDEO_PROCESSOR_H
#define VIDEO_PROCESSOR_H

#include <opencv2/opencv.hpp>
#include <string>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/frame.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

class VideoProcessor {
private:
    cv::VideoCapture camera;
    cv::VideoWriter output;
    int width;
    int height;
    int fps;
    bool isRunning;

    AVCodecContext* codecContext;
    AVFrame* frameYUV420p;
    SwsContext* swsContext;
    AVPacket* packet;

    void invertColors(cv::Mat& frame);
    void flipFrame(cv::Mat& frame);
    void scaleFrame(cv::Mat& frame, int newWidth, int newHeight);
    bool initializeH264Encoder();
    bool encodeFrame(const cv::Mat& frame);

public:
    VideoProcessor(int cameraId, int w, int h, int f);
    ~VideoProcessor();
    
    bool startCapture();
    bool captureFrame(cv::Mat& frame);
    void processFrame(cv::Mat& frame);
    bool displayFrame(const cv::Mat& frame);
    void stop();
    bool isActive() const;
};

#endif // VIDEO_PROCESSOR_H
