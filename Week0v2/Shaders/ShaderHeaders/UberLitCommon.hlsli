#pragma once

// ---------------------------------------------
// 조명 구조체 정의
// ---------------------------------------------

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
