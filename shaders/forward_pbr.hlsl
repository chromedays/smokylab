#pragma pack_matrix(row_major)
#include "common.hlsli"

struct VS_INPUT {
  float4 pos : POSITION;
  float4 color : COLOR;
  float4 texcoord : TEXCOORD;
  float4 normal : NORMAL;
};

struct FS_INPUT {
  float4 pos : SV_POSITION;
  float2 texcoord : TEXCOORD0;
  float3 posWorld : TEXCOORD1;
  float3 normalWorld : NORMAL;
};

FS_INPUT vert(VS_INPUT input) {
  FS_INPUT output;

  float4 posWorld = mul(float4(input.pos.xyz, 1), modelMat);
  output.pos = mul(mul(posWorld, viewMat), projMat);
  output.texcoord = input.texcoord.xy;
  float3x3 normalMat = (float3x3)transpose(invModelMat);
  output.posWorld = posWorld.xyz;
  output.normalWorld = mul(input.normal.xyz, normalMat);
  return output;
}

float4 frag(FS_INPUT input) : SV_Target {
  float4 baseColor = baseColorFactor *
                     baseColorTexture.Sample(baseColorSampler, input.texcoord);
  //   baseColor.xyz = gammaToLinear(baseColor.xyz);
  return float4(baseColor.xyz, 1);
}