#include "video_processor.h"
#include "config.h"
#include <spdlog/spdlog.h>

int main() {
    spdlog::info("Starting Video Processor");
    spdlog::info("Press ESC to exit");

    VideoProcessor processor(CAMERA_ID, INPUT_WIDTH, INPUT_HEIGHT, INPUT_FPS);

    if (!processor.startCapture()) {
        spdlog::error("Error: Failed to start camera capture!");
        spdlog::error("Make sure camera is connected and not in use");
        return 1;
    }

    spdlog::info("Camera initialized successfully!");
    
    #if ENABLE_YUV422_FORMAT
        spdlog::info("Format: YUV422 (YUYV)");
    #else
        spdlog::info("Format: MJPEG");
    #endif
    
    spdlog::info("Resolution: {}x{}", INPUT_WIDTH, INPUT_HEIGHT);
    spdlog::info("FPS: {}", INPUT_FPS);

    while (processor.isActive()) {
        cv::Mat frame;
        if (!processor.captureFrame(frame)) {
            spdlog::warn("Warning: Empty frame captured, skipping");
            continue;
        }

        processor.processFrame(frame);

        if (!processor.displayFrame(frame)) {
            spdlog::info("Exit triggered!");
            break;
        }
    }

    spdlog::info("Video Processor stopped");

    return 0;
}
