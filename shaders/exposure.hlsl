#pragma pack_matrix(row_major)
#include "common.hlsli"

struct FS_INPUT {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

struct FS_OUTPUT {
    float4 color : SV_TARGET;
};

FS_INPUT vert(uint id: SV_VertexID) {
    FS_INPUT output;
    output.uv = float2(id & 2, (id << 1) & 2);
    output.pos = float4(output.uv * float2(2, -2) + float2(-1, 1), 0, 1);
    return output;
}

Texture2D<float4> tex : register(t0);
SamplerState nearestSampler : register(s0);

FS_OUTPUT frag(FS_INPUT input) {
    FS_OUTPUT output;

    float exposure = exposureNearFar.x;

    float3 color =  tex.Sample(nearestSampler, input.uv).xyz;
    color = (exposure * color) / (exposure * color + float3(1, 1, 1));
    color = pow(abs(color), 1 / 2.2);
    output.color = float4(color, 1);

    // uint x = (uint)input.pos.x;
    // uint y = (uint)input.pos.y;
    // uint result = (30 * (x ^ y)) + 10 * x * y;
    // output.color0 = frac(result / PI2).xxxx;
    // ((30 * (uint)input.pos.x) ^ (uint)input.pos.y) + 10 * 
    // output.color0 = float4(input.pos.xy, 0, 1);
    return output;
}