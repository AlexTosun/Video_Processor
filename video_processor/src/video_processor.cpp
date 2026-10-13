#include "video_processor.h"
#include "config.h"
#include <spdlog/spdlog.h>

VideoProcessor::VideoProcessor(int cameraId, int w, int h, int f) {
    width = w;
    height = h;
    fps = f;
    isRunning = false;

    camera.open(cameraId, cv::CAP_V4L2);
    if (!camera.isOpened()) {
        return;
    }

    int fourcc;
    #if ENABLE_YUV422_FORMAT
        if(w == 640 && h == 480) {
            fourcc = cv::VideoWriter::fourcc('Y', 'U', 'Y', 'V');
        }
        else 
        {
            spdlog::error("Unsupported resolution for YUV422 format: {}x{}", w, h);
            return 1;
        }
    #else
        if(((((w == 640 && (h == 480 || h == 360)) || (w == 1280 && h == 720)) || (w == 1920 && h == 1080)) || (w == 2560 && h == 1440)) || (w == 2880 && (h == 1620 || 1800 ))) {
            fourcc = cv::VideoWriter::fourcc('M', 'J', 'P', 'G');
        }
        else 
        {
            spdlog::error("Unsupported resolution for MJPG format: {}x{}", w, h);
            return 1;
        }
    #endif
    
    camera.set(cv::CAP_PROP_FOURCC, fourcc);
    camera.set(cv::CAP_PROP_FRAME_WIDTH, width);
    camera.set(cv::CAP_PROP_FRAME_HEIGHT, height);
    camera.set(cv::CAP_PROP_FPS, fps);
    camera.set(cv::CAP_PROP_BUFFERSIZE, 1);

    initializeH264Encoder();

}

VideoProcessor::~VideoProcessor() {
    stop();
}

bool VideoProcessor::startCapture() {
    if (!camera.isOpened()) {
        return false;
    }
    cv::namedWindow("Video Stream",cv::WINDOW_AUTOSIZE);
    isRunning = true;
    return true;
}

bool VideoProcessor::captureFrame(cv::Mat& frame) {
    if (!isRunning) {
        return false;
    }
    
    camera >> frame;
    
    if (frame.empty()) {
        return false;
    }
    return true;
}

void VideoProcessor::processFrame(cv::Mat& frame) {
    if (frame.empty()) {
        return;
    }
    if (ENABLE_COLOR_INVERT) {
        invertColors(frame);
    }
    if (ENABLE_FLIP) {
        flipFrame(frame);
    }
    if (ENABLE_SCALE) {
        scaleFrame(frame, OUTPUT_WIDTH, OUTPUT_HEIGHT);
    }
    encodeFrame(frame);

}

void VideoProcessor::invertColors(cv::Mat& frame) {
    cv::bitwise_not(frame, frame);
}

void VideoProcessor::flipFrame(cv::Mat& frame) {
    cv::flip(frame, frame, 1);
}

void VideoProcessor::scaleFrame(cv::Mat& frame, int newWidth, int newHeight) {
    cv::resize(frame, frame, cv::Size(newWidth, newHeight));
}

bool VideoProcessor::displayFrame(const cv::Mat& frame) {
    cv::imshow("Video Stream", frame);
    
    int frameTime = 1000 / OUTPUT_FPS;
    int key = cv::waitKey(frameTime);
    
    if (key == 27) {
        return false;
    }
    return true;
}

void VideoProcessor::stop() {
    isRunning = false;
    if (packet) {
        av_packet_free(&packet);
        packet = nullptr;
    }
    if (frameYUV420p) {
        av_frame_free(&frameYUV420p);
        frameYUV420p = nullptr;
    }
    if (swsContext) {
        sws_freeContext(swsContext);
        swsContext = nullptr;
    }
    if (codecContext) {
        avcodec_free_context(&codecContext);
        codecContext = nullptr;
    }
    camera.release();
    cv::destroyAllWindows();
}

bool VideoProcessor::isActive() const {
    return isRunning;
}

bool VideoProcessor::initializeH264Encoder() {
    const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec) {
        return false;
    }
    codecContext = avcodec_alloc_context3(codec);
    if (!codecContext) {
        return false;
    }
    codecContext->codec_id = AV_CODEC_ID_H264;
    codecContext->codec_type = AVMEDIA_TYPE_VIDEO;
    codecContext->bit_rate = H264_BITRATE * 1000;
    codecContext->width = width;
    codecContext->height = height;
    codecContext->time_base = AVRational{1, OUTPUT_FPS};
    codecContext->framerate = AVRational{OUTPUT_FPS, 1};
    codecContext->gop_size = 30;
    codecContext->pix_fmt = AV_PIX_FMT_YUV420P;

    av_opt_set(codecContext->priv_data, "preset", H264_PRESET, 0);
    av_opt_set(codecContext->priv_data, "tune", "zerolatency", 0);

    if (avcodec_open2(codecContext, codec, nullptr) < 0) {
        avcodec_free_context(&codecContext);
        codecContext = nullptr;
        return false;
    }

    frameYUV420p = av_frame_alloc();
    if (!frameYUV420p) {
        avcodec_free_context(&codecContext);
        codecContext = nullptr;
        return false;
    }

    frameYUV420p->format = AV_PIX_FMT_YUV420P;
    frameYUV420p->width = width;
    frameYUV420p->height = height;

    if (av_frame_get_buffer(frameYUV420p, 32) < 0) {
        av_frame_free(&frameYUV420p);
        frameYUV420p = nullptr;
        avcodec_free_context(&codecContext);
        codecContext = nullptr;
        return false;
    }

    packet = av_packet_alloc();
    if (!packet) {
        av_frame_free(&frameYUV420p);
        frameYUV420p = nullptr;
        avcodec_free_context(&codecContext);
        codecContext = nullptr;
        return false;
    }

    swsContext = sws_getContext(width, height, AV_PIX_FMT_BGR24, width, height, AV_PIX_FMT_YUV420P, SWS_BILINEAR, nullptr, nullptr, nullptr);

    if (!swsContext) {
        av_packet_free(&packet);
        packet = nullptr;
        av_frame_free(&frameYUV420p);
        frameYUV420p = nullptr;
        avcodec_free_context(&codecContext);
        codecContext = nullptr;
        return false;
    }
    return true;
}

bool VideoProcessor::encodeFrame(const cv::Mat& frame) {
    if (!codecContext || !frameYUV420p || !packet || !swsContext) {
        return false;
    }

    const uint8_t* srcSlice[1] = { frame.data };
    int srcStride[1] = { static_cast<int>(frame.step) };
    if (av_frame_make_writable(frameYUV420p) < 0) {
        return false;
    }

    sws_scale(swsContext, srcSlice, srcStride, 0, frame.rows, frameYUV420p->data, frameYUV420p->linesize);

    frameYUV420p->pts = 0;

    if (avcodec_send_frame(codecContext, frameYUV420p) < 0) {
        return false;
    }

    int receiveResult = avcodec_receive_packet(codecContext, packet);
    if (receiveResult < 0) {
        return false;
    }

    // H264 bitstream format starts with 00 00 00 01, followed by a NAL header.
    // if (packet->size >= 5 && packet->data[0] == 0x00 && packet->data[1] == 0x00 && packet->data[2] == 0x00 && packet->data[3] == 0x01) {
    //     int nalType = packet->data[4] & 0x1F;
    //     spdlog::info("H264 format confirmed with size={} and nalType={}", packet->size, nalType);
    // } 
    // else 
    // {
    //     spdlog::warn("H264 format not confirmed withsize={}", packet->size);
    // }
    av_packet_unref(packet);
    return true;
}