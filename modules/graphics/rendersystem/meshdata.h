#pragma once
#include "../../headeronly/globaltypes.h"
#include "../../headeronly/mathematics.h"

// ==================== VERTEX DATA ====================
struct Vertex
{
    Quark::Vec3 position;
    Quark::Vec3 normal;
    Quark::Vec2 texCoord;
    Quark::Vec3 tangent;
    Quark::Vec3 bitangent;
};

// ==================== MESH DATA ====================
struct MeshData
{
    Vertex* vertices = nullptr;
    UINT32 vertexCount = 0;
    UINT32* indices = nullptr;
    UINT32 indexCount = 0;
    Quark::AABB boundingBox;
};

