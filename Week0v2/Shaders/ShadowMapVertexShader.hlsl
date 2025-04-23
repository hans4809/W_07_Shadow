struct FLightVP
{
    row_major float4x4 LightVP;
};

StructuredBuffer<FLightVP> LightViewProjectionMatrix : register(t0);
#if DIRECTIONAL_LIGHT
#define MAX_CASCADES 4

cbuffer FCascadeCB : register(b0)
{
    row_major float4x4 ModelMatrix;
    row_major float4x4 LightVP[MAX_CASCADES]; // per-cascade VP matrices
    uint      NumCascades;
    float3    pad;
};

struct VS_INPUT
{
    float4 position : POSITION;
};

struct VS_OUTPUT
{
    float4 worldPos  : TEXCOORD0;
};

VS_OUTPUT mainVS(VS_INPUT input)
{
    VS_OUTPUT output;
    output.worldPos = mul(input.position, ModelMatrix);
    return output;
}
#endif

#if SPOT_LIGHT
cbuffer FSpotCB : register(b0)
{
    row_major float4x4  ModelMatrix;
    row_major float4x4  VPMatrix;
    uint      NumSpotLights;
    float3    pad;
};

struct VS_INPUT
{
    float4 position : POSITION;
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    uint   RenderTargetArrayIndex : SV_RenderTargetArrayIndex;
};

VS_OUTPUT mainVS(VS_INPUT input, uint instanceID : SV_InstanceID)
{
    VS_OUTPUT output;
    float4 cameraWarped = mul(input.position, ModelMatrix);
    cameraWarped = mul(cameraWarped, VPMatrix); // ← P′ 곱함
    uint idx = instanceID;
    row_major float4x4 VP = LightViewProjectionMatrix[idx].LightVP;
    output.position = mul(cameraWarped, VP);
    output.RenderTargetArrayIndex = idx;
    return output;
}
#endif

#if POINT_LIGHT
cbuffer FPointCB : register(b0)
{
    row_major float4x4 ModelMatrix;
    uint     NumPoints;
}

struct VS_INPUT
{
    float4 position : POSITION;
    uint   LightID   : SV_InstanceID; // 0..numPointLights-1
};

struct VS_OUTPUT
{
    float3 worldPos : TEXCOORD0;
    uint   LightID     : TEXCOORD1;
};

VS_OUTPUT mainVS(VS_INPUT input)
{
    VS_OUTPUT output;
    output.worldPos = mul(float4(input.position), ModelMatrix).xyz;
    output.LightID = input.LightID;
    return output;
}
#endif