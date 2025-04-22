#include "ShaderHeaders/UberLitCommon.hlsli"

Texture2D Texture : register(t0);
Texture2D NormalTexture : register(t1);
StructuredBuffer<uint> TileLightIndices : register(t2);
Texture2DArray DirLightShadowMap : register(t7);

StructuredBuffer<FLightVP> SpotVP : register(t3);
StructuredBuffer<FLightVP> PointVP : register(t5);

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


// dir 벡터를 기반으로 사용해야 할 큐브맵 face 인덱스와 
// 해당 face 내에서의 UV 좌표를 계산하는 함수
uint ComputeCubeface(float3 dir)
{
    // 방향 벡터 각 성분의 절댓값 계산 (주축 판별용)
    float adx = abs(dir.x);
    float ady = abs(dir.y);
    float adz = abs(dir.z);
    
    // X축이 가장 크게 기여하는 경우
    if (adx >= ady && adx >= adz)
    {
        if (dir.x > 0)
        {
            // +X 면 (face 0)
            return 0;
        }
        else
        {
            // -X 면 (face 1)
            return 1;
        }
    }
    // Y축이 가장 크게 기여하는 경우
    else if (ady >= adx && ady >= adz)
    {
        if (dir.y > 0)
        {
            // +Y 면 (face 2)
            return 2;
        }
        else
        {
            // -Y 면 (face 3)
            return 3;
        }
    }
    // Z축이 가장 크게 기여하는 경우
    else
    {
        if (dir.z > 0)
        {
            // +Z 면 (face 4)
            return 4;
        }
        else
        {
            // -Z 면 (face 5)
            return 5;
        }
    }
}

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
    
    if (!IsLit)
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

    // 1. cascadeIndex 선택 (좌표계 일치 확인)
    float4 viewPos = mul(float4(input.worldPos, 1.0), ViewMatrix);
    float pixelDepth = viewPos.z;
    
    int cascadeIndex = 0;
    if (pixelDepth > DirLight.CascadeSplits1)
        cascadeIndex = 1;
    if (pixelDepth > DirLight.CascadeSplits2)
        cascadeIndex = 2;
    if (pixelDepth > DirLight.CascadeSplits3)
        cascadeIndex = 3;

    // 2. Shadow Map 좌표 변환
    float4 shadowCoord = mul(float4(input.worldPos, 1.0), DirLight.ViewProjectionMatrix[cascadeIndex]);
    
    float2 shadowUV =
    {
        0.5f + shadowCoord.x / shadowCoord.w / 2.f,
        0.5f - shadowCoord.y / shadowCoord.w / 2.f
    };

    float shadowDepth = shadowCoord.z / shadowCoord.w;
    
    // 4. Shadow Map 샘플링
    float bias = 0.005; // 실험적으로 조정
    shadowDepth -= bias;
    float mapDepth = DirLightShadowMap.SampleCmpLevelZero(linearComparisionSampler, float3(shadowUV, cascadeIndex), shadowDepth).r;
    
    // 방향광 처리
    TotalLight += mapDepth * CalculateDirectionalLight(DirLight, Normal, ViewDir, baseColor.rgb,SpecularScalar,SpecularColor);  

    // 점광 처리
    [loop]
    for(uint j = 0; j < NumPointLights; ++j)
    {
        // uint listIndex = tileIndex * MAX_POINTLIGHT_COUNT + j;
        // uint lightIndex = TileLightIndices[listIndex];
        // if (lightIndex == 0xFFFFFFFF)
        // {
        //     break;
        // }

        float3 L = input.worldPos - PointLights[j].Position;
        float dist = length(L);
        float3 dir = L/dist;

        uint face = ComputeCubeface(dir);
        uint pointVPIndex = j * 6 + face;

        float shadow = SamplePointShadow(PointVP[pointVPIndex], PointLights[j], input.worldPos, j);
        float3 light = CalculatePointLight(PointLights[j], input.worldPos, Normal, ViewDir, baseColor.rgb, SpecularScalar, SpecularColor);
        TotalLight += shadow * light;
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