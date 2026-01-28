// ==================== rsd3d11_main_vs.h ====================
// DirectX 11 Vertex Shaders (HLSL Source)
#pragma once

// PBR Vertex Shader with Instancing
static const char* g_PBRVertexShaderSource = R"(
// ==================== FRAME CONSTANTS ====================
cbuffer FrameConstants : register(b0)
{
    matrix g_View;
    matrix g_Projection;
    matrix g_ViewProjection;
    matrix g_InvView;
    matrix g_InvProjection;
    
    float3 g_CameraPosition;
    float g_Time;
    
    float4 g_AmbientLight;
    
    float g_DeltaTime;
    uint g_ActiveLightCount;
    uint g_ShadowAtlasSize;
    uint g_LocalShadowAtlasSize;
};

// ==================== VERTEX INPUT ====================
struct VS_INPUT
{
    // Per-vertex data (from Vertex Buffer, Slot 0)
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD0;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
    
    // Per-instance data (from Instance Buffer, Slot 1)
    // PerInstanceData struct: worldMatrix(64) + worldInvTranspose(64) + customData(16) = 144 bytes
    matrix world : WORLD;               // 4x float4 = 64 bytes
    matrix worldInvTranspose : INVWORLD;// 4x float4 = 64 bytes
    float4 customData : CUSTOM;         // 1x float4 = 16 bytes
};

// ==================== VERTEX OUTPUT ====================
struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float2 texCoord : TEXCOORD2;
    float3 tangent : TEXCOORD3;
    float3 bitangent : TEXCOORD4;
    float instanceFlags : TEXCOORD5;  // RenderObjectFlags from CPU
};

// ==================== VERTEX SHADER MAIN ====================
VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    
    // Use Instance Data for matrices
    matrix worldMatrix = input.world;
    matrix worldInvTranspose = input.worldInvTranspose;
    
    // World space position: position * world (row vector * matrix)
    float4 worldPos = mul(float4(input.position, 1.0f), worldMatrix);
    output.worldPos = worldPos.xyz;
    
    // Transform normal to world space using inverse transpose
    // For correct lighting: N' = (M^-1)^T * N
    output.normal = normalize(mul((float3x3)worldInvTranspose, input.normal));
    
    // Transform tangent and bitangent to world space
    // Tangent/bitangent use regular world matrix (they follow surface, not normals)
    output.tangent = normalize(mul((float3x3)worldMatrix, input.tangent));
    output.bitangent = normalize(mul((float3x3)worldMatrix, input.bitangent));
    
    // Texture coordinates (pass through)
    output.texCoord = input.texCoord;
    
    // Pass instance flags to pixel shader (customData.x contains RenderObjectFlags)
    output.instanceFlags = input.customData.x;
    
    // Clip space position (using FrameConstants ViewProjection)
    output.position = mul(worldPos, g_ViewProjection);
    
    return output;
}
)";

