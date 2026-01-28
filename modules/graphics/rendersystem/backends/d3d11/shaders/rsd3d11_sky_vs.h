// ==================== rsd3d11_sky_vs.h ====================
// Sky Vertex Shader - Sky Sphere around camera
#pragma once

static const char* g_SkyVertexShaderSource = R"(
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
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD0;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
};

// ==================== VERTEX OUTPUT ====================
struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float3 viewDir : TEXCOORD0;
    float2 uv : TEXCOORD1;
};

// ==================== VERTEX SHADER MAIN ====================
// Sky sphere: centered at camera, rendered at far plane
VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    
    // Local position is the view direction (unit sphere normalized position)
    // The sphere vertices are already in local space, normalize to get direction
    output.viewDir = normalize(input.position);
    
    // World position = camera position + local position (sphere around camera)
    float3 worldPos = g_CameraPosition + input.position;
    
    // Project to clip space using row-vector * matrix order (D3D11 convention)
    // Same as main vertex shader: mul(position, matrix)
    float4 clipPos = mul(float4(worldPos, 1.0), g_ViewProjection);
    
    // Force Z = W to render at far plane (depth = 1.0)
    // This ensures sky is always behind everything
    clipPos.z = clipPos.w;
    
    output.position = clipPos;
    output.uv = input.texCoord;
    
    return output;
}
)";