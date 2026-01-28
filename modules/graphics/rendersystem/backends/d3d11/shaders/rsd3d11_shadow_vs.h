#pragma once

static const char* g_ShadowVertexShaderSource = R"(
cbuffer FrameConstants : register(b0)
{
    matrix g_View;
    matrix g_Projection;
    matrix g_ViewProjection;
    // ... other constants ignored in shadow pass
};

// ==================== VERTEX INPUT ====================
struct VS_INPUT
{
    // Per-vertex
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD0;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
    
    // Per-instance (Must match PBR Input Layout)
    matrix world : WORLD;
    matrix worldInvTranspose : INVWORLD;
    float4 customData : CUSTOM;
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    
    // Use Instance Data for World Matrix
    float4 worldPos = mul(float4(input.position, 1.0f), input.world);
    
    // Use Light ViewProjection (bound to b0 g_ViewProjection)
    output.position = mul(worldPos, g_ViewProjection); 
    
    return output;
}
)";
