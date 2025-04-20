
struct VS_OUTPUT
{
    float4 position : SV_POSITION;
};

float4 mainPS(VS_OUTPUT input) : SV_TARGET
{
    // 단순히 흰색으로 출력(실루엣 확인 용)
    return float4(0, 0, 0, 1);
}
