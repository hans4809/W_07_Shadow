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
    row_major float4x4 LightVP; // per-cascade VP matrices
};

struct VS_INPUT
{
    float4 position : POSITION;
};

struct VS_OUTPUT
{
    float4 position  : SV_POSITION;
};

VS_OUTPUT mainVS(VS_INPUT input)
{
    VS_OUTPUT output;
    float4 worldPos = mul(input.position, ModelMatrix);
    output.position = mul(worldPos, LightVP);
    return output;
}
#endif

#if SPOT_LIGHT
cbuffer FSpotCB : register(b0)
{
    row_major float4x4  ModelMatrix;
    uint SpotIndex;
};

struct VS_INPUT
{
    float4 position : POSITION;
    uint   LightID  : SV_InstanceID;        // 0..NumSpotLights-1
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
};

VS_OUTPUT mainVS(VS_INPUT input)
{
    VS_OUTPUT output;
    float4 worldPos = mul(input.position, ModelMatrix);
    row_major float4x4 VP = LightViewProjectionMatrix[SpotIndex].LightVP;
    output.position = mul(worldPos, VP);
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