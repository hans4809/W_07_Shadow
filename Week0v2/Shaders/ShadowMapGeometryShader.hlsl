
StructuredBuffer<row_major float4x4> LightViewProjectionMatrix : register(t0);

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
        float4x4 VP = LightViewProjectionMatrix[slice];
        
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