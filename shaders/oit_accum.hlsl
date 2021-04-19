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

struct FS_OUTPUT {
    float4 accum : SV_TARGET0;
    float reveal : SV_TARGET1;
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

FS_OUTPUT frag(FS_INPUT input) {
    FS_OUTPUT output;
    
    float4 baseColor = baseColorFactor * baseColorTexture.Sample(baseColorSampler, input.texcoord);
    baseColor.xyz = gammaToLinear(baseColor.xyz);
    float d = input.pos.z;
    // float weight = baseColor.w * max(0.01, 3000 * pow(1 - d, 3))
    float weight = 1;
    output.accum = baseColor * weight;
    output.reveal = baseColor.w;

    return output;
}