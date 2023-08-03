#pragma once

#include <map>

#include "Buffer.hpp"

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
    void CreateCommandPool();
    void FillLibrary();

    VkCommandPool commandPool;
    
    Device* logicalDevice;
};