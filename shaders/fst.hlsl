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

Texture2D<float4> tex : register(t0);
SamplerState nearestSampler : register(s0);

float4 frag(FS_INPUT input) : SV_Target {
  float d = tex.Sample(nearestSampler, input.uv).r;
  float n = exposureNearFar.y;
  float f = exposureNearFar.z;
  float A = projMat._m22;
  float B = projMat._m32;
  float ld = ((B / (d + A)) - n) / (f - n);
  // float ld =  (n * (1 - d)) / (n + (f - n) * d);
  return float4(ld, ld, ld, 1);
}