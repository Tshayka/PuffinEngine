#pragma once

#include <map>

#include "Device.hpp"

class MeshLibrary {
public:
    MeshLibrary();
    ~MeshLibrary();

    void DeInit();
    void Init(Device* device);
        
    std::vector<uint32_t> aabbIndices;
	std::vector<enginetool::VertexLayout> aabbVertices;
    std::vector<uint32_t> indices;
	std::vector<enginetool::VertexLayout> vertices;

    std::map<std::string, enginetool::ScenePart> meshes;

private:
    void FillLibrary();
    void GetAABBDrawData();
    void Load(enginetool::ScenePart& mesh);
    void PrepeareAABBs();
    Device* logicalDevice;
};