# Puffin Engine - Readme

[![Install vulkan on Linux](https://img.shields.io/badge/Build-Linux-orange.svg)](#install-vulkan-on-linux)
[![GitHub license](https://img.shields.io/badge/License-GPLv3-blue.svg)](#license)


## Screenshots

![vulkan-tutorial](https://github.com/Tshayka/PuffinEngine/blob/build-branch/printscreen/engineMainPhoto.png)

![vulkan-tutorial](https://github.com/Tshayka/PuffinEngine/blob/build-branch/printscreen/engineWirePhoto.png)

## Install graphic cards

### Intel
- $ sudo apt install libxcb1-dev xorg-dev 
- $ sudo apt install mesa-vulkan-drivers vulkan-utils libegl1-mesa-dev
    
### Nvidia
- $ sudo apt-get purge nvidia* 
- $ sudo add-apt-repository ppa:graphics-drivers/ppa
- $ sudo apt upgrade
- $ sudo apt install nvidia-graphics-drivers-390 
- $ sudo apt-mark hold nvidia-390
- $ lsmod | grep nvidia 

## Install vulkan on Linux
- Add necessary libraries: $ sudo apt install libglm-dev cmake libxcb-dri3-0 libxcb-present0 libpciaccess0 \
libpng-dev libxcb-keysyms1-dev libxcb-dri3-dev libx11-dev \
libmirclient-dev libwayland-dev libxrandr-dev
- You may also need: $ sudo apt install libxcursor-dev libxinerama-dev libxi-dev
- Download Vulkan file (vulkansdk-linux-x86_64-1.1.xx.y.tar.gz) from [LunarG](https://vulkan.lunarg.com/sdk/home#linux)
- Unpack to home directory: $ tar -xzf /home/user/Downloads/vulkan-sdk.tar.gz
- Change your directory: $ cd /home/user/vulkan/1.1.xx.y/
- Run install script: $ ./build_samples.sh

### Set environmental variables 
You must add them inside working directory, because they won't be global:
- $ export VULKAN_SDK=/home/user/vulkan/1.1.xx.y/x86_64
- $ export PATH=$VULKAN_SDK/bin:$PATH
- $ export LD_LIBRARY_PATH=$VULKAN_SDK/lib:$LD_LIBRARY_PATH
- $ export VK_LAYER_PATH=$VULKAN_SDK/etc/explicit_layer.d

### Check vulkan
- $ vulkaninfo | less
- $ printenv

#### Run example
- $ cd /home/user/vulkan/1.1.xx.y/examples
- $ mkdir build
- $ cd build
- $ cmake ..
- $ make
- $ ./cube

## Building
- $ cmake -H. -Bbuild
- $ cmake --build build -- -j4

## Running program
- $ ./bin/PuffinEngine

## Running tests
- $ ./bin/PuffinEngineTest

## Compiling shaders
- $ ./puffinEngine/shaders/compile.sh

## Third party libraries

The following third party packages are included, and carry, their own copyright notices and license terms: 
- [GLFW](https://github.com/glfw/glfw)
- [GLM](https://github.com/glm/glm)
- [GLI](https://github.com/gli/gli)
- [Dear ImGui](https://github.com/ocornut/imgui)
- [JSON for Modern C++](https://github.com/nlohmann/json)
- [Bitmap fonts for C/C++](https://nothings.org/stb/font/)
- [stb](https://github.com/nothings/stb)
- [tinyobjloader](https://github.com/syoyo/tinyobjloader)
- [Icons](https://www.flaticon.com)
- [Fonts](https://fonts.google.com/specimen/Exo+2)

## License
The source code and the documentation are licensed under the [GPLv3](https://www.gnu.org/licenses/gpl-3.0.html).