#pragma pack_matrix(row_major)
#include "common.hlsli"

struct FS_INPUT {
    float4 pos : SV_POSITION;
    float3 posModel : TEXCOORD;
};

FS_INPUT vert(float4 pos : POSITION) {
    FS_INPUT output;
    
    float3x3 rotViewMat = (float3x3)viewMat;
    float4 clipPos = mul(float4(mul(pos.xyz, rotViewMat), 1), projMat);
    clipPos.z = 0;
    output.pos = clipPos;
    output.posModel = pos.xyz;

    return output;
}

#define PI 3.141592
#define PI2 6.283185
float4 frag(FS_INPUT input) : SV_Target {
    float3 normal = normalize(input.posModel);
    float2 texcoord = sampleEquirectangularMap(normal);
    float3 color = skyTexture.SampleLevel(skySampler, texcoord, 0).xyz;
    // color = (exposure.x * color) / (exposure.x * color + float3(1, 1, 1));
    // return float4(pow(abs(color), 1 / 2.2), 1);
    return float4(color, 1);
}