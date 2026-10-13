# Quick Start

## Virtual Environment (Optional)
Create a Python virtual environment named `video`:
```bash
python3 -m venv video
source video/bin/activate
```

## Install
```bash
python3 scripts/setup-env.py --install
```

## Build
```bash
mkdir -p build
cd build
cmake ..
make
```

## Run
```bash
./video_processor
```
Press **ESC** to exit.

# Settings
Edit `include/config.h`:

**Camera Formats:**
```cpp
#define ENABLE_YUV422_FORMAT false  // MJPG: 1920x1080 (default)
#define ENABLE_YUV422_FORMAT true   // YUV422: 640x480
```

**Video transformations:**
```cpp
#define ENABLE_COLOR_INVERT true
#define ENABLE_FLIP true
#define ENABLE_SCALE true
```

**Resolution hardcoded values:**
```cpp
#define INPUT_WIDTH 1920
#define INPUT_HEIGHT 1080
#define OUTPUT_WIDTH 1280
#define OUTPUT_HEIGHT 720
```

**Camera compatibility:**
If the selected camera format is not supported by the connected device, the application logs an error and exits controlled instead of core-dump.

Check the camera output with:
```bash
v4l2-ctl --list-formats-ext -d /dev/video0
```

**Supported MJPG resolutions:**
- 640x480
- 640x360
- 1280x720
- 1920x1080
- 2560x1440
- 2880x1620
- 2880x1800

**Supported YUV422 resolutions:**
- 640x480
- 640x360

# Requirements
- Linux (Ubuntu 24.04)
- camera device available
- CMake 3.10+