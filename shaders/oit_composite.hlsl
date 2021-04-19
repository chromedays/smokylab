#pragma pack_matrix(row_major)
#include "common.hlsli"

struct FS_INPUT {
  float4 pos : SV_POSITION;
  float2 uv : TEXCOORD;
};

FS_INPUT vert(uint id: SV_VertexID) {
  FS_INPUT output;
  output.uv = float2(id & 2, (id << 1) & 2);
  output.pos = float4(output.uv * float2(2, -2) + float2(-1, 1), 0, 1);
  return output;
}

Texture2D<float4> accumTex : register(t0);
Texture2D<float> revealTex : register(t1);
SamplerState nearestSampler : register(s0);

float4 frag(FS_INPUT input) : SV_Target {
    float3 texcoord = float3(input.pos.xy, 0);
    float4 accum = accumTex.Load(texcoord);
    float reveal = revealTex.Load(texcoord);
    // return float4(accum.xyz / clamp(accum.a, 0.0001, 50000), reveal);
    return float4(accum.xyz  / clamp(accum.a, 0.0001, 50000), reveal);
} 