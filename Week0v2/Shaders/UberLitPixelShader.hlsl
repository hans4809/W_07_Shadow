#include "ShaderHeaders/GSamplers.hlsli"
#include "ShaderHeaders/UberLitCommon.hlsli"

Texture2D Texture : register(t0);
Texture2D NormalTexture : register(t1);
StructuredBuffer<uint> TileLightIndices : register(t2);

StructuredBuffer<FLightVP> SpotVP : register(t3);

struct PS_INPUT
{
    float4 position : SV_POSITION; // 변환된 화면 좌표
    float3 worldPos : POSITION;
    float4 color : COLOR; // 전달할 색상
    float2 texcoord : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float3x3 TBN: TEXCOORD3;
};

struct PS_OUTPUT
{
    float4 color : SV_Target0;
    float4 UUID : SV_Target1;
};

// 타일 크기 설정
/*static const uint TILE_SIZE_X = 16;
static const uint TILE_SIZE_Y = 16;*/

PS_OUTPUT mainPS(PS_INPUT input)
{
    PS_OUTPUT output;
    output.UUID = UUID;
    float2 uvAdjusted = input.texcoord;
    
    // 기본 색상 추출  
    float4 baseColor = Texture.Sample(linearSampler, uvAdjusted) + float4(DiffuseColor, 1.0);  

    if (!IsLit && !IsNormal)
    {
        output.color = float4(baseColor.rgb, 1.0);
        return output;
    }
    
#if LIGHTING_MODEL_GOURAUD
    if (IsSelectedActor == 1)
        input.color = input.color * 5;

    output.color = float4(baseColor.rgb * input.color.rgb, 1.0);
    return output;
#endif
    float4 normalTex = ((NormalTexture.Sample(linearSampler, uvAdjusted)- 0.5) * 2);

    float2 tileSize = float2(TILE_SIZE_X, TILE_SIZE_Y);
    
    uint2 pixelCoord = uint2(input.position.xy);
    uint2 tileCoord = pixelCoord / tileSize; // 각 성분별 나눔
    uint tileIndex = tileCoord.x + tileCoord.y * numTilesX;
    
    if(!IsLit)
    {
        output.color = float4(baseColor.rgb, 1.0);
        return output;
    }
    
    float3 Normal = input.normal;
    
    if (bHasNormalTexture)
    {
        Normal = normalize(mul(normalTex.rgb, input.TBN));

        if (length(Normal) < 0.001) // tangent 값이 없을때 ( uv 없을때 )
        {
            Normal = input.normal;
        }        
    }
    
    if (IsNormal)
    {
        //Normal = input.normal;
        Normal = Normal * 0.5 + 0.5;
        output.color = float4(Normal.rgb, 1.0);
        return output;
    }
    
    float3 ViewDir = normalize(CameraPos - input.worldPos); // CameraPos도 안 들어오고, ViewDir은 카메라의 Foward 아닌가요?
    
    //float3 TotalLight = MatAmbientColor; // 전역 앰비언트
    // TODO : Lit이면 낮은 값 Unlit이면 float3(1.0f,1.0f,1.0f)면 됩니다.
    float3 TotalLight = CalculateAmbientLight(AmbientLight, input.color.rgb); // 전역 앰비언트  
    if (IsSelectedActor == 1)
         TotalLight = TotalLight * 10.0f;
    TotalLight += EmissiveColor; // 자체 발광  

    // 방향광 처리  s
    TotalLight += CalculateDirectionalLight(DirLight, Normal, ViewDir, baseColor.rgb,SpecularScalar,SpecularColor);  

    // 점광 처리
    [loop]
    for(uint j = 0; j < NumPointLights; ++j)
    {
        uint listIndex = tileIndex * MAX_POINTLIGHT_COUNT + j;
        uint lightIndex = TileLightIndices[listIndex];
        if (lightIndex == 0xFFFFFFFF)
        {
            break;
        }
        
        TotalLight += CalculatePointLight(PointLights[lightIndex], input.worldPos, Normal, ViewDir, baseColor.rgb, SpecularScalar, SpecularColor);
    }
    
    for (uint k = 0; k < NumSpotLights; ++k)
    {
        float shadow = CalculateShadowSpotLight(SpotVP[k], input.worldPos,k);
        float3 light = CalculateSpotLight(SpotLights[k], input.worldPos, input.normal, ViewDir, baseColor.rgb, SpecularScalar, SpecularColor);
        TotalLight += shadow * light;
    }
    
    // 최종 색상 
    output.color = float4(TotalLight * baseColor.rgb, baseColor.a * TransparencyScalar);
    return output;  
}