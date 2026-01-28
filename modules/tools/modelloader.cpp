#include "modelloader.h"
#include <iostream>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// ==================== LOAD MODEL ====================
bool ModelLoader::Load(const char* filepath, LoadedModel& outModel)
{
    Assimp::Importer importer;
    
    const aiScene* scene = importer.ReadFile(filepath,
        aiProcess_Triangulate |
        aiProcess_GenNormals |
        aiProcess_CalcTangentSpace |
        aiProcess_FlipUVs |
        aiProcess_JoinIdenticalVertices |
        aiProcess_OptimizeMeshes
    );
    
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::cerr << "[ModelLoader] ERROR: " << importer.GetErrorString() << "\n";
        return false;
    }
    
    outModel.path = filepath;
    outModel.name = scene->mRootNode->mName.C_Str();
    outModel.meshes.clear();
    
    ProcessNode(scene->mRootNode, (void*)scene, outModel);
    
    outModel.isLoaded = true;
    std::cout << "[ModelLoader] Loaded: " << filepath << " (" << outModel.meshes.size() << " meshes)\n";
    
    return true;
}

// ==================== PROCESS NODE ====================
void ModelLoader::ProcessNode(void* nodePtr, void* scenePtr, LoadedModel& model)
{
    aiNode* node = static_cast<aiNode*>(nodePtr);
    const aiScene* scene = static_cast<const aiScene*>(scenePtr);
    
    // Process all meshes in this node
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        model.meshes.push_back(ProcessMesh(mesh, (void*)scene));
    }
    
    // Process children
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        ProcessNode(node->mChildren[i], scenePtr, model);
    }
}

// ==================== PROCESS MESH ====================
LoadedMesh ModelLoader::ProcessMesh(void* meshPtr, void* scenePtr)
{
    aiMesh* mesh = static_cast<aiMesh*>(meshPtr);
    
    LoadedMesh result;
    result.name = mesh->mName.C_Str();
    
    // Reserve space
    result.vertices.reserve(mesh->mNumVertices);
    result.indices.reserve(mesh->mNumFaces * 3);
    
    // Bounds
    Quark::Vec3 minBounds(FLT_MAX, FLT_MAX, FLT_MAX);
    Quark::Vec3 maxBounds(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    
    // Process vertices
    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex = {};
        
        // Position
        vertex.position.x = mesh->mVertices[i].x;
        vertex.position.y = mesh->mVertices[i].y;
        vertex.position.z = mesh->mVertices[i].z;
        
        // Update bounds
        minBounds.x = (std::min)(minBounds.x, vertex.position.x);
        minBounds.y = (std::min)(minBounds.y, vertex.position.y);
        minBounds.z = (std::min)(minBounds.z, vertex.position.z);
        maxBounds.x = (std::max)(maxBounds.x, vertex.position.x);
        maxBounds.y = (std::max)(maxBounds.y, vertex.position.y);
        maxBounds.z = (std::max)(maxBounds.z, vertex.position.z);
        
        // Normal
        if (mesh->HasNormals())
        {
            vertex.normal.x = mesh->mNormals[i].x;
            vertex.normal.y = mesh->mNormals[i].y;
            vertex.normal.z = mesh->mNormals[i].z;
        }
        
        // UV (first channel)
        if (mesh->mTextureCoords[0])
        {
            vertex.texCoord.x = mesh->mTextureCoords[0][i].x;
            vertex.texCoord.y = mesh->mTextureCoords[0][i].y;
        }
        
        // Tangent & Bitangent
        if (mesh->HasTangentsAndBitangents())
        {
            vertex.tangent.x = mesh->mTangents[i].x;
            vertex.tangent.y = mesh->mTangents[i].y;
            vertex.tangent.z = mesh->mTangents[i].z;
            
            vertex.bitangent.x = mesh->mBitangents[i].x;
            vertex.bitangent.y = mesh->mBitangents[i].y;
            vertex.bitangent.z = mesh->mBitangents[i].z;
        }
        
        result.vertices.push_back(vertex);
    }
    
    // Process indices
    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace& face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
        {
            result.indices.push_back(face.mIndices[j]);
        }
    }
    
    // Set up MeshData
    result.data.vertices = result.vertices.data();
    result.data.vertexCount = static_cast<UINT32>(result.vertices.size());
    result.data.indices = result.indices.data();
    result.data.indexCount = static_cast<UINT32>(result.indices.size());
    result.data.boundingBox.minBounds = minBounds;
    result.data.boundingBox.maxBounds = maxBounds;

    
    return result;
}

// ==================== SUPPORTED EXTENSIONS ====================
const char* ModelLoader::GetSupportedExtensions()
{
    return "3D Models\0*.obj;*.fbx;*.gltf;*.glb;*.dae;*.3ds;*.blend\0All Files\0*.*\0";
}
