# Puffin Engine - Readme

![Linux](https://img.shields.io/badge/Build-Linux-orange.svg)
![License](https://img.shields.io/badge/License-GPLv3-blue.svg)

## Install vulkan on Linux

### Intel:
- $ tar -xzf /home/sandro/Pobrane/vulkan-sdk.tar.gz
- $ cd /home/sandro/1.1.82.1/build_samples.sh
- $ sudo apt install libxcb1-dev xorg-dev 
- $ sudo apt install mesa-vulkan-drivers vulkan-utils libegl1-mesa-dev
- $ cd home/sandro/1.1.82.1/examples/build
- $ ./cube
    
### Nvidia:
- $ sudo add-apt-repository ppa:graphics-drivers/ppa
- $ sudo apt upgrade
- $ sudo apt install nvidia-graphics-drivers-396 nvidia-settings vulkan vulkan-utils
    
### Set environmental
- $ export VULKAN_SDK=~/vulkan/1.1.xx.y/x86_64
- $ export PATH=$VULKAN_SDK/bin:$PATH
- $ export LD_LIBRARY_PATH=$VULKAN_SDK/lib:$LD_LIBRARY_PATH
- $ export VK_LAYER_PATH=$VULKAN_SDK/etc/explicit_layer.d

### Check vulkan
- $ vulkaninfo | less
- $ printenv

## Building
1) cmake -H. -Bbuild
2) cmake --build build -- -j4

## Running program
./bin/puffinEngine

## Running tests
./bin/puffinEngineTest

## Third party libraries

- [GLFW](https://github.com/glfw/glfw)

## License

