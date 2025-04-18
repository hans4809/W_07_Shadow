#pragma once

// ---------------------------------------------
// 조명 구조체 정의
// ---------------------------------------------
// 최대 라이트 수 정의 (컴파일 타임 상수)
#define NUM_POINT_LIGHT 4
#define NUM_SPOT_LIGHT 4
//tile 기반 최대치
#define MAX_POINTLIGHT_COUNT 16

// 타일 크기 (조명 타일링 기준)
#define TILE_SIZE_X 16
#define TILE_SIZE_Y 16
struct FDirectionalLight
{
    float3 Direction;
    float Intensity;
    float4 Color;
};

struct FPointLight
{
    float3 Position;
    float Radius;
    float4 Color;
    float Intensity;
    float AttenuationFalloff;
    float2 pad;
};

struct FSpotLight
{
    float3 Position;
    float Intensity;
    float4 Color;
    float3 Direction;
    float InnerAngle;
    float OuterAngle;
    float3 pad;
};

// ---------------------------------------------
// 조명 계산 함수 정의
// ---------------------------------------------

float3 CalculateDirectionalLight(
    FDirectionalLight Light,
    float3 Normal,
    float3 ViewDir,
    float3 Albedo,
    float SpecularScalar,
    float3 SpecularColor)
{
    float3 LightDir = normalize(-Light.Direction);
    float NdotL = max(dot(Normal, LightDir), 0.0);
    float3 Diffuse = Light.Color.rgb * Albedo * NdotL;

#if defined(LIGHTING_MODEL_LAMBERT)
    return Diffuse * Light.Intensity;
#else // Blinn-Phong
    float3 HalfVec = normalize(LightDir + ViewDir);
    float NdotH = max(dot(Normal, HalfVec), 0.0);
    float Specular = pow(NdotH, SpecularScalar * 128.0) * SpecularScalar;
    float3 specularColor = Light.Color.rgb * Specular * SpecularColor;

    return (Diffuse + specularColor) * Light.Intensity;
#endif
}

float3 CalculatePointLight(
    FPointLight Light,
    float3 WorldPos,
    float3 Normal,
    float3 ViewDir,
    float3 Albedo,
    float SpecularScalar,
    float3 SpecularColor)
{
    float3 LightDir = Light.Position - WorldPos;
    float Distance = length(LightDir);
    LightDir = normalize(LightDir);

    if (Distance > Light.Radius)
        return float3(0, 0, 0);

    float Attenuation = Light.Intensity / (1.0 + Light.AttenuationFalloff * Distance * Distance);
    Attenuation *= 1.0 - smoothstep(0.0, Light.Radius, Distance);

    float NdotL = max(dot(Normal, LightDir), 0.0);
    float3 Diffuse = Light.Color.rgb * Albedo * NdotL;

#if defined(LIGHTING_MODEL_LAMBERT)
    return Diffuse * Attenuation;
#else
    float3 HalfVec = normalize(LightDir + ViewDir);
    float NdotH = max(dot(Normal, HalfVec), 0.0);
    float Specular = pow(NdotH, SpecularScalar * 128.0) * SpecularScalar;
    float3 specularColor = Light.Color.rgb * Specular * SpecularColor;

    return (Diffuse + specularColor) * Attenuation;
#endif
}

float3 CalculateSpotLight(
    FSpotLight Light,
    float3 WorldPos,
    float3 Normal,
    float3 ViewDir,
    float3 Albedo,
    float SpecularScalar,
    float3 SpecularColor)
{
    float3 LightDir = normalize(Light.Position - WorldPos);
    float Distance = length(Light.Position - WorldPos);
    float3 SpotDirection = normalize(-Light.Direction);

    float CosInner = cos(Light.InnerAngle);
    float CosOuter = cos(Light.OuterAngle);
    float CosAngle = dot(SpotDirection, LightDir);

    if (CosAngle < CosOuter)
        return float3(0, 0, 0);

    float SpotAttenuation = saturate((CosAngle - CosOuter) / (CosInner - CosOuter));
    float DistanceAttenuation = 1.0 / (1.0 + Distance * Distance * 0.01);

    float NdotL = max(dot(Normal, SpotDirection), 0.0);
    float3 Diffuse = Light.Color.rgb * Albedo * NdotL;

#if defined(LIGHTING_MODEL_LAMBERT)
    return Diffuse * Light.Intensity * SpotAttenuation * DistanceAttenuation;
#else
    float3 HalfVec = normalize(SpotDirection + ViewDir);
    float NdotH = max(dot(Normal, HalfVec), 0.0);
    float Specular = pow(NdotH, SpecularScalar * 128.0) * SpecularScalar;
    float3 specularColor = Light.Color.rgb * Specular * SpecularColor;

    return (Diffuse + specularColor) * Light.Intensity * SpotAttenuation * DistanceAttenuation;
#endif
}
/*cbuffer FMaterialConstants : register(b0)
{
    float3 DiffuseColor;
    float TransparencyScalar;
    float3 MatAmbientColor;
    float DensityScalar;
    float3 SpecularColor;
    float SpecularScalar;
    float3 EmissiveColor;
    uint bHasNormalTexture;
};*/
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
cbuffer FCameraConstant : register(b3)
{
    row_major matrix ViewMatrix;
    row_major matrix ProjMatrix;
    row_major matrix ViewProjMatrix;

    float3 CameraPos;
    float NearPlane;
    float3 CameraForward;
    float FarPlane;
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