#pragma pack_matrix(row_major)
#include "common.hlsli"

struct VS_INPUT {
  float4 pos : POSITION;
};

struct FS_INPUT {
  float4 pos : SV_POSITION;
};

FS_INPUT vert(VS_INPUT input) {
  FS_INPUT output;

  float4 posWorld = mul(float4(input.pos.xyz, 1), modelMat);
  output.pos = mul(mul(posWorld, viewMat), projMat);
  return output;
}

float4 frag(FS_INPUT input) : SV_Target { return float4(1, 0, 0, 1); }