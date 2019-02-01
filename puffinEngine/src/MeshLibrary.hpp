#pragma once

#include <map>

#include "Device.hpp"

class MeshLibrary {
public:
    MeshLibrary();
    ~MeshLibrary();

    void DeInit();
    void Init(Device* device);
    
    static void LoadFromFile(const std::string &filename, enginetool::ScenePart& mesh, std::vector<uint32_t>& indices, std::vector<enginetool::VertexLayout>& vertices);
    void Load(enginetool::ScenePart& mesh);
    

    std::vector<uint32_t> indices;
	std::vector<enginetool::VertexLayout> vertices;

    std::map<std::string, enginetool::ScenePart> meshes;
    std::string testValue = "I came from MeshLibrary";
private:
    void FillLibrary();
    Device* logicalDevice;
};