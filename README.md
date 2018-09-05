# Puffin Engine - Readme

[![Linux](https://img.shields.io/badge/Build-Linux-orange.svg)]
(#Install-vulkan-on-Linux)

[![GitHub license](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://github.com/Tshayka/PuffinEngine/blob/master/LICENSE.md)

## Install vulkan on Linux

### Intel
- $ tar -xzf /home/user/Downloads/vulkan-sdk.tar.gz
- $ cd /home/user/vulkan/1.1.xx.y/build_samples.sh
- $ sudo apt install libxcb1-dev xorg-dev 
- $ sudo apt install mesa-vulkan-drivers vulkan-utils libegl1-mesa-dev
    
### Nvidia
- $ sudo add-apt-repository ppa:graphics-drivers/ppa
- $ sudo apt upgrade
- $ sudo apt install nvidia-graphics-drivers-396 nvidia-settings vulkan vulkan-utils
    
### Set environmental variables
- $ export VULKAN_SDK=~/vulkan/1.1.xx.y/x86_64
- $ export PATH=$VULKAN_SDK/bin:$PATH
- $ export LD_LIBRARY_PATH=$VULKAN_SDK/lib:$LD_LIBRARY_PATH
- $ export VK_LAYER_PATH=$VULKAN_SDK/etc/explicit_layer.d

### Check vulkan
- $ vulkaninfo | less
- $ printenv
#### Run example
- $ cd /home/user/vulkan/1.1.xx.y/examples/build
- $ ./cube

## Building
- $ cmake -H. -Bbuild
- $ cmake --build build -- -j4

## Running program
./bin/puffinEngine

## Running tests
./bin/puffinEngineTest

## Third party libraries

- [GLFW](https://github.com/glfw/glfw)

## License

The source code and the documentation are licensed under the [GPLv3](https://www.gnu.org/licenses/gpl-3.0.html).