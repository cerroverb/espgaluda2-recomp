// Ported from github.com/ChanseyIsTheBest/futari-nx (pull request 2, commit 7f5a36a), app/src/renut_engine/gpu/shaders/host/overlay_ps.hlsl. GPL-3.0 (LICENSE-GPL-3.0).
#include "utility.hlsli"

struct PSInput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
};

float4 main(PSInput input) : SV_Target0
{
    return input.color * SampleBindless(g_Push.textureIndex, g_Push.samplerIndex, input.uv);
}
