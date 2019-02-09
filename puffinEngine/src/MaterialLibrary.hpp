#pragma once

#include <map>

#include "Device.hpp"

class MaterialLibrary {
public:
    MaterialLibrary();
    ~MaterialLibrary();

    void DeInit();
    void Init(Device* device);
        

private:
    Device* logicalDevice;
};