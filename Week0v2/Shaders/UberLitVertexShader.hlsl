#include "ShaderHeaders/GSamplers.hlsli"
#include "ShaderHeaders/UberLitCommon.hlsli"


struct VS_INPUT
{
    float4 position : POSITION; // 버텍스 위치
    float4 color : COLOR; // 버텍스 색상
    float3 normal : NORMAL; // 버텍스 노멀
    float3 tangent : TANGENT;
    float2 texcoord : TEXCOORD;
};

struct PS_INPUT
{
    float4 position : SV_POSITION; // 변환된 화면 좌표
    float3 worldPos : POSITION;
    float4 color : COLOR; // 전달할 색상
    float2 texcoord : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float3x3 TBN : TEXCOORD3;
};

PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;
    
    float4 worldPos = mul(input.position, Model);
    output.position = mul(worldPos, ViewProj);
    output.worldPos = worldPos.xyz;
    output.color = input.color;
    output.texcoord = input.texcoord;
    
    float3 normal = mul(float4(input.normal, 0), MInverseTranspose);
    
#if LIGHTING_MODEL_GOURAUD
    float3 viewDir = normalize(CameraPos - worldPos.xyz);
    
    // float3 totalLight = MatAmbientColor + EmissiveColor;
    //TODO : Lit 이면 기본 Ambient를 낮은 값으로 하고 Unlit이면 float(1.0,1.0,1.0)으로 하면 됩니다.
    //기본적으로 머터리얼을 읽어올 때는 1.0,1.0,1.0으로 읽어오는 것 같아요 
    float3 totalLight = float3(0.01f,0.01f,0.01f) + EmissiveColor;
    
    if(!IsLit)
    {
        output.color = float4(input.color.rgb, 1.0);
        return output;
    }
    // 정점 색상 계산 (디퓨즈 + 스페큘러)
    totalLight += CalculateDirectionalLight(DirLight, normal, viewDir, input.color.rgb,SpecularScalar,SpecularColor);

    // 점광 처리  
    for(uint j=0; j<NumPointLights; ++j)  
        totalLight += CalculatePointLight(PointLights[j], worldPos.xyz, normal, viewDir, input.color.rgb,SpecularScalar,SpecularColor);  

    //SpotLight 처리
    for(uint k=0; k<NumSpotLights; ++k)
        totalLight += CalculateSpotLight(SpotLights[k], worldPos.xyz, normal, viewDir, input.color.rgb,SpecularScalar,SpecularColor);

    // 정점 셰이더에서 계산된 색상을 픽셀 셰이더로 전달
    output.color = float4(totalLight * input.color.rgb, input.color.a * TransparencyScalar);  
        
    return output;
#endif
    float3 tangent = normalize(mul(input.tangent, Model));

    // 탄젠트-노멀 직교화 (Gram-Schmidt 과정) 해야 안전함
    tangent = normalize(tangent - normal * dot(tangent, normal));

    // 바이탄젠트 계산 (안전한 교차곱)
    float3 biTangent = cross(normal, tangent);

    // 최종 TBN 행렬 구성 (T, B, N는 각각 한 열 또는 행이 될 수 있음, 아래 예제는 행 벡터로 구성)
    // row_major float4x4 TBN = float4x4(T, B, N, float4(0,0,0,1));
    float3x3 TBN = float3x3(tangent, biTangent, normal);

    output.TBN = TBN;
    output.normal = normal;

    return output;
}
