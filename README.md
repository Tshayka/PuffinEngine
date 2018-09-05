
![PuffinEngine](https://img.shields.io/travis/:user/:repo.svg)

I. Install vulkan on Linux
    A. Intel
        1) tar -xzf /home/sandro/Pobrane/vulkan-sdk.tar.gz
        2) cd /home/sandro/1.1.82.1/build_samples.sh
        3) sudo apt install libxcb1-dev xorg-dev 
        4) sudo apt install mesa-vulkan-drivers vulkan-utils libegl1-mesa-dev
        5) cd home/sandro/1.1.82.1/examples/build
        6) ./cube
    
    B. Nvidia
        1) sudo add-apt-repository ppa:graphics-drivers/ppa
        2) sudo apt upgrade
        3) sudo apt install nvidia-graphics-drivers-396 nvidia-settings vulkan vulkan-utils
    
    C. Set environmental
        1) export VULKAN_SDK=~/vulkan/1.1.xx.y/x86_64
        2) export PATH=$VULKAN_SDK/bin:$PATH
        3) export LD_LIBRARY_PATH=$VULKAN_SDK/lib:$LD_LIBRARY_PATH
        4) export VK_LAYER_PATH=$VULKAN_SDK/etc/explicit_layer.d

    D. To make sure, check vulkan
        1) vulkaninfo | less
        2) printenv

II. Building
    1) cmake -H. -Bbuild
    2) cmake --build build -- -j4

III. Running program
./bin/puffinEngine

IV. Running tests
./bin/puffinEngineTest