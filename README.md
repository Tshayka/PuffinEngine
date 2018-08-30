I. Install vulkan
    A.Intel
        1) tar -xzf /home/sandro/Pobrane/vulkan-sdk.tar.gz
        2) cd /home/sandro/1.1.82.1/build_samples.sh
        3) sudo apt install libxcb1-dev xorg-dev
        4) sudo apt install mesa-vulkan-drivers vulkan-utils
        5) cd home/sandro/1.1.82.1/examples/build
        6) ./cube
    
    B. Nvidia

II. Building
    1) cmake -H. -Bbuild
    2) cmake --build build -- -j4

III. Running program
./bin/puffinEngine

IV. Running tests
./bin/puffinEngineTest