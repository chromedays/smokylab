#pragma pack_matrix(row_major)
#include "common.hlsli"

struct FS_INPUT {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

struct FS_OUTPUT {
    float4 color0 : SV_TARGET0;
    // float4 color1 : SV_TARGET1;
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

    float3 color =  tex.Sample(nearestSampler, input.uv).xyz;
    color = (exposure.x * color) / (exposure.x * color + float3(1, 1, 1));
    color = pow(abs(color), 1 / 2.2);
    output.color0 = float4(color, 1);

    return output;
}