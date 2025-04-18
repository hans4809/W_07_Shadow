#pragma once
#include "UberLitCommon.hlsli"

cbuffer FMaterialConstants : register(b0)
{
    float3 DiffuseColor;
    float TransparencyScalar;
    float3 MatAmbientColor;
    float DensityScalar;
    float3 SpecularColor;
    float SpecularScalar;
    float3 EmissiveColor;
    uint bHasNormalTexture;
};

cbuffer FLightingConstants : register(b1)
{
    uint NumDirectionalLights;
    uint NumPointLights;
    uint NumSpotLights;
    float pad;

    FDirectionalLight DirLights[4];
    FPointLight PointLights[16];
    FSpotLight SpotLights[8];
};

cbuffer FFlagConstants : register(b2)
{
    uint IsLit;
    uint IsNormal;
    float2 flagPad0;
};

cbuffer FConstatntBufferActor : register(b4)
{
    float4 UUID;
    uint IsSelectedActor;
    float3 padding;
};

cbuffer FSubUVConstant : register(b5)
{
    float indexU;
    float indexV;
    float2 subUVpadding;
};

cbuffer FComputeConstants : register(b6)
{
    row_major matrix InverseView;
    row_major matrix InverseProj;
    float screenWidth;
    float screenHeight;
    int numTilesX;
    int numTilesY;
};

cbuffer FMatrixConstants : register(b7)
{
    row_major float4x4 Model;
    row_major float4x4 ViewProj;
    row_major float4x4 MInverseTranspose;
    bool isSelected;
    float3 pad0;
};
