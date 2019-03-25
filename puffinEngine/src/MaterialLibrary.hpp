#pragma once

#include <map>
#include <sstream>

#include "Device.hpp"
#include "Material.cpp"

class MaterialLibrary {
public:
    MaterialLibrary();
    ~MaterialLibrary();

    struct bmchar {
        uint32_t x, y;
        uint32_t width;
        uint32_t height;
        int32_t xoffset;
        int32_t yoffset;
        int32_t xadvance;
        uint32_t page;
    };

    void DeInit();
    void Init(Device* device);

    void LoadTexture(std::string texture, TextureLayout& layer);
    void LoadSkyboxTexture(TextureLayout& layer);

    std::map<std::string, enginetool::SceneMaterial> materials;
    std::map<std::string, TextureLayout> icons;
    TextureLayout font;
    std::vector<bmchar> fontChars;

private:
    void CreateCommandPool();
    void FillLibrary();
    int32_t NextValuePair(std::stringstream *stream);
    void ParseFont(const std::string& file);

    VkCommandPool commandPool;
    
    Device* logicalDevice;
};