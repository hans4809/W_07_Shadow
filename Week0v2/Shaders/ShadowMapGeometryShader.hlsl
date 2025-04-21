struct FLightVP
{
    row_major float4x4 LightVP;
};

StructuredBuffer<FLightVP> LightViewProjectionMatrix : register(t0);

#if DIRECTIONAL_LIGHT
#define MAX_CASCADES 4
struct VS_OUTPUT
{
    float4 WorldPos : TEXCOORD0;
};

struct GS_OUTPUT
{
    float4 Position : SV_POSITION;
    uint   Slice    : SV_RenderTargetArrayIndex;
};

cbuffer FCascadeCB : register(b0)
{
    row_major float4x4 ModelMatrix;
    row_major float4x4 LightVP[MAX_CASCADES]; // per-cascade VP matrices
    uint      NumCascades;
    float3    pad;
};

[maxvertexcount(3 * 4)]  // 3 vertices × MAX_CASCADES
void mainGS(triangle VS_OUTPUT triIn[3], inout TriangleStream<GS_OUTPUT> stream)
{
    // NumCascades 만큼 반복하면서 각 LightVP 행렬로 변환
    [unroll]
    for (uint c = 0; c < 4; ++c)
    {
        float4x4 vp = LightVP[c];
        for (int v = 0; v < 3; ++v)
        {
            GS_OUTPUT o;
            o.Position = mul(triIn[v].WorldPos, vp);
            o.Slice    = c;
            stream.Append(o);
        }
        stream.RestartStrip();
    }
}
#endif

#if POINT_LIGHT
struct GS_INPUT
{
    float3 worldPos : TEXCOORD0;
    uint   LightID     : TEXCOORD1;
};

struct GS_OUTPUT
{
    float4 Position                : SV_POSITION;
    uint   RenderTargetArrayIndex  : SV_RenderTargetArrayIndex;
};

[maxvertexcount(18)]
void mainGS(triangle GS_INPUT input[3], inout TriangleStream<GS_OUTPUT> triStream)
{
    uint lightIdx = input[0].LightID;
    for (uint face = 0; face < 6; ++face)
    {
        uint slice = lightIdx * 6 + face;
        float4x4 VP = LightViewProjectionMatrix[slice].LightVP;
        
        for (int v = 0; v < 3; ++v)
        {
            GS_OUTPUT outV;
            outV.Position               = mul(float4(input[v].worldPos, 1), VP);
            outV.RenderTargetArrayIndex = slice;
            triStream.Append(outV);
        }
        triStream.RestartStrip();
    }
}
#endif