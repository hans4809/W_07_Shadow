#ifndef UBERLITCOMMON_HLSLI
#define UBERLITCOMMON_HLSLI

// ---------------------------------------------
// 조명 구조체 정의
// ---------------------------------------------
// 최대 라이트 수 정의 (컴파일 타임 상수)
#define NUM_POINT_LIGHT 8
#define NUM_SPOT_LIGHT 16
//tile 기반 최대치
#define MAX_POINTLIGHT_COUNT 16

// 타일 크기 (조명 타일링 기준)
#define TILE_SIZE_X 16
#define TILE_SIZE_Y 16

struct FAmbientLight
{
    float4 Color;

    float Intensity;
    float3 pad;
};
struct FDirectionalLight
{
    float4 Color;
    
    float Intensity;
    float3 Direction;
};

struct FPointLight
{
    float4 Color;
    
    float Intensity;
    float3 Position;

    float Radius;
    float AttenuationFalloff;
    float2 pad;
};

struct FSpotLight
{
    float4 Color;

    float Intensity;
    float3 Position;

    float3 Direction;
    float InnerAngle;

    float OuterAngle;
    float Radius;
    float AttenuationFalloff;
    float pad;
};

struct FLightVP
{
    row_major float4x4 LightVP;
};

// ---------------------------------------------
// 조명 계산 함수 정의
// ---------------------------------------------
float3 CalculateAmbientLight(FAmbientLight Light, float3 Albedo)
{
    return Light.Color.rgb * Albedo * Light.Intensity;
}

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
    float3 LightDir = Light.Position - WorldPos;
    float Distance = length(LightDir);
    LightDir = normalize(LightDir);

    float3 SpotDirection = normalize(-Light.Direction);

    float CosInner = cos(Light.InnerAngle);
    float CosOuter = cos(Light.OuterAngle);
    float CosAngle = dot(SpotDirection, LightDir);

    if (Distance > Light.Radius || CosAngle < CosOuter)
        return float3(0, 0, 0);

    float SpotAttenuation = saturate((CosAngle - CosOuter) / (CosInner - CosOuter));

    // 변경된 거리 기반 감쇠
    float DistanceAttenuation = Light.Intensity / (1.0 + Light.AttenuationFalloff * Distance * Distance);
    //Radius 기반 감쇠(일단 제외)
    //DistanceAttenuation *= 1.0 - smoothstep(0.0, Light.Radius, Distance);

    float NdotL = max(dot(Normal, SpotDirection), 0.0);
    float3 Diffuse = Light.Color.rgb * Albedo * NdotL;

#if defined(LIGHTING_MODEL_LAMBERT)
    return Diffuse * SpotAttenuation * DistanceAttenuation;
#else
    float3 HalfVec = normalize(SpotDirection + ViewDir);
    float NdotH = max(dot(Normal, HalfVec), 0.0);
    float Specular = pow(NdotH, SpecularScalar * 128.0) * SpecularScalar;
    float3 specularColor = Light.Color.rgb * Specular * SpecularColor;

    return (Diffuse + specularColor) * SpotAttenuation * DistanceAttenuation;
#endif
}

Texture2DArray<float> SpotShadowMap : register(t4);
TextureCubeArray<float> PointShadowMap : register(t6);
SamplerComparisonState ShadowSampler : register(s4); // Shadow sampler

float3 CalculateShadowSpotLight(FLightVP light, float3 PixelWorldPos, uint index)
{
    //float4 lightSpace = mul(light.LightVP, float4(worldPos, 1.0));
    float4 lightSpace = mul(float4(PixelWorldPos, 1.0), light.LightVP);
    lightSpace.xyz /= lightSpace.w;

    float2 uv =
    {
        0.5f + lightSpace.x * 0.5f,
        0.5f - lightSpace.y * 0.5f
    };
    //float z = lightSpace.z * 0.5 + 0.5;
    float z = lightSpace.z;

    if (uv.x < 0 || uv.x > 1 || uv.y < 0 || uv.y > 1)
        return 1.0;

    return SpotShadowMap.SampleCmpLevelZero(ShadowSampler, float3(uv, index), z);
}

float SamplePointShadow(FLightVP light, FPointLight Light, float3 PixelWorldPos, uint index)
{
    float3 lightPos = Light.Position;
    float3 LightToWorld = PixelWorldPos - lightPos;

    float4 lightSpace = mul(float4(PixelWorldPos, 1.0), light.LightVP);
    lightSpace.xyz /= lightSpace.w;
    float z = lightSpace.z;

    float3 dir = normalize(LightToWorld);

    return PointShadowMap.SampleCmp(ShadowSampler, float4(dir, index), z);
}

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
    uint NumPointLights;
    uint NumSpotLights;
    float2 pad;
    
    FAmbientLight AmbientLight;
    FDirectionalLight DirLight;
    FPointLight PointLights[NUM_POINT_LIGHT];
    FSpotLight SpotLights[NUM_SPOT_LIGHT];
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
#endif