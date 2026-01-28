#pragma once

#include <string>
#include <vector>
#include "../graphics/rendersystem/meshdata.h"
#include "../headeronly/mathematics.h"

// ==================== LOADED MESH ====================
struct LoadedMesh
{
    std::string name;
    MeshData data;
    std::vector<Vertex> vertices;
    std::vector<UINT32> indices;
};

// ==================== LOADED MODEL ====================
struct LoadedModel
{
    std::string name;
    std::string path;
    std::vector<LoadedMesh> meshes;
    bool isLoaded = false;
};

// ==================== MODEL LOADER ====================
class ModelLoader
{
public:
    // Load a 3D model file (OBJ, FBX, GLTF, etc.)
    static bool Load(const char* filepath, LoadedModel& outModel);
    
    // Get supported extensions
    static const char* GetSupportedExtensions();
    
private:
    // Process Assimp scene recursively
    static void ProcessNode(void* node, void* scene, LoadedModel& model);
    static LoadedMesh ProcessMesh(void* mesh, void* scene);
};
