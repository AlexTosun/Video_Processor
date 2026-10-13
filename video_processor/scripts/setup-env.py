#!/usr/bin/env python3
import subprocess
import sys
import argparse

def check_package(package_name):
    try:
        result = subprocess.run(['dpkg', '-l', package_name], capture_output=True, text=True)
        if result.returncode == 0:
            lines = result.stdout.split('\n')
            for line in lines:
                if line.startswith('ii'):
                    parts = line.split()
                    if len(parts) >= 3:
                        return True, parts[2]
        return False, None
    except Exception as e:
        print(f"Error checking {package_name}: {e}")
        return False, None

def update_apt():
    print("Updating package list")
    try:
        result = subprocess.run(['sudo', 'apt-get', 'update'], capture_output=False)
        return result.returncode == 0
    except Exception as e:
        print(f"Error updating package list: {e}")
        return False

def install_packages(packages):
    if not packages:
        return True
    
    print(f"Installing {len(packages)} packages")
    try:
        cmd = ['sudo', 'apt-get', 'install', '-y'] + packages
        result = subprocess.run(cmd, capture_output=False)
        return result.returncode == 0
    except Exception as e:
        print(f"Error installing packages: {e}")
        return False

def check_all_packages():
    required_packages = {
        'libopencv-dev': 'OpenCV Development Library',
        'libavcodec-dev': 'FFmpeg Video Codec Library',
        'libavformat-dev': 'FFmpeg Format Library',
        'libswscale-dev': 'FFmpeg Image Scaling Library',
        'libspdlog-dev': 'Spdlog Logging Library',
        'build-essential': 'C++ Compiler and Build Tools',
        'v4l-utils': 'Video4Linux Camera Utilities',
        'cmake': 'CMake Build System'
    }
    
    missing_packages = []
    
    for package, description in required_packages.items():
        is_installed, version = check_package(package)
        if is_installed:
            print(f"Found: {package}")
        else:
            print(f"Missing: {package}")
            missing_packages.append(package)
    
    return missing_packages, required_packages

def main():
    parser = argparse.ArgumentParser(description='Video Processor - Dependency Setup Script')
    parser.add_argument('--install', action='store_true', help='Automatically install missing packages')
    args = parser.parse_args()
    
    missing_packages, required_packages = check_all_packages()
    
    if missing_packages:
        print(f"Missing {len(missing_packages)} packages")
        
        if args.install:
            if update_apt():
                if install_packages(missing_packages):
                    print("Done! All packages installed")
                    return 0
                else:
                    print("Installation failed!")
                    return 1
            else:
                print("Failed to update package list!")
                return 1
        else:
            print("Run this to install:")
            print(f"python3 scripts/setup-env.py --install")
            return 1
    else:
        print("All packages are already installed")
        return 0

if __name__ == '__main__':
    sys.exit(main())
