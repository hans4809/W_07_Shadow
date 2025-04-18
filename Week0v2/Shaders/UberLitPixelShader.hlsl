#include "ShaderHeaders/GSamplers.hlsli"
#include "ShaderHeaders/UberLitCommon.hlsli"

Texture2D Texture : register(t0);
Texture2D NormalTexture : register(t1);
StructuredBuffer<uint> TileLightIndices : register(t2);

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
}

cbuffer FConstatntBufferActor : register(b1)
{
    float4 UUID; // 임시
    uint IsSelectedActor;
    float3 padding;
}

cbuffer FLightingConstants : register(b2)
{
    uint NumDirectionalLights;
    uint NumPointLights;
    uint NumSpotLights;
    float pad;

    FDirectionalLight DirLights[4];
    FPointLight PointLights[16];
    FSpotLight SpotLights[8];
};

cbuffer FFlagConstants : register(b3)
{
    uint IsLit;
    uint IsNormal;
    float2 flagPad0;
}

cbuffer FSubUVConstant : register(b4)
{
    float indexU;
    float indexV;
    float2 subUVpadding;
}

cbuffer FCameraConstant : register(b5)
{
    row_major matrix ViewMatrix;
    row_major matrix ProjMatrix;
    row_major matrix ViewProjMatrix;
    
    float3 CameraPos;
    float NearPlane;
    float3 CameraForward;
    float FarPlane;
};

cbuffer FComputeConstants : register(b6){
    row_major matrix InverseView;
    row_major matrix InverseProj;
    float screenWidth;
    float screenHeight;
    int numTilesX;
    int numTilesY;
}

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
    float3 TotalLight = float3(0.01f,0.01f,0.01f); // 전역 앰비언트  
    if (IsSelectedActor == 1)
         TotalLight = TotalLight * 10.0f;
    TotalLight += EmissiveColor; // 자체 발광  

    // 방향광 처리  s
    for(uint i=0; i<NumDirectionalLights; ++i)  
        TotalLight += CalculateDirectionalLight(DirLights[i], Normal, ViewDir, baseColor.rgb,SpecularScalar,SpecularColor);  

    // 점광 처리  
    for(uint j=0; j<NumPointLights; ++j)
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
        TotalLight += CalculateSpotLight(SpotLights[k], input.worldPos, input.normal, ViewDir, baseColor.rgb, SpecularScalar, SpecularColor);
    
    // 최종 색상 
    output.color = float4(TotalLight * baseColor.rgb, baseColor.a * TransparencyScalar);
    return output;  
}