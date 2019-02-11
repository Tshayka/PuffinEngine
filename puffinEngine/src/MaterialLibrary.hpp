#pragma once

#include <map>

#include "Device.hpp"

class MaterialLibrary {
public:
    MaterialLibrary();
    ~MaterialLibrary();

    void DeInit();
    void Init(Device* device);

    void LoadTexture(std::string texture, TextureLayout& layer);
    void LoadSkyboxTexture(TextureLayout& layer);

    std::map<std::string, enginetool::SceneMaterial> materials;    

private:
    void FillLibrary();
    
    Device* logicalDevice;
};