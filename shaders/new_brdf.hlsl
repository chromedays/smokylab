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
    float3 posView : TEXCOORD2;
    float3 normalWorld : NORMAL;
};

struct FS_OUTPUT {
    float4 color : SV_TARGET0;
    float4 pos : SV_TARGET1;
    float4 normal : SV_TARGET2;
    float4 albedo : SV_TARGET3;
};

FS_INPUT vert(VS_INPUT input) {
    FS_INPUT output;
    
    float4 posWorld = mul(float4(input.pos.xyz, 1), modelMat);
    float4 posView = mul(posWorld, viewMat);
    output.pos = mul(posView, projMat);
    output.texcoord = input.texcoord.xy;
    float3x3 normalMat = (float3x3)transpose(invModelMat);
    output.posWorld = posWorld.xyz;
    output.posView = posView.xyz;
    output.normalWorld = mul(input.normal.xyz, normalMat);
    return output;
}

FS_OUTPUT frag(FS_INPUT input) {
    FS_OUTPUT output;

    float4 baseColor = baseColorFactor * baseColorTexture.Sample(baseColorSampler, input.texcoord);
    if (overrideOpacity.x > 0) {
        baseColor.w = overrideOpacity.y;
    }
    if (baseColor.w <= 0.999) {
        discard;
    }

    baseColor.xyz = gammaToLinear(baseColor.xyz);

    float3 L = -normalize(dirLightDirIntensity.xyz);
    float3 intensity = dirLightDirIntensity.w;
    float3 N = normalize(input.normalWorld);

    // float3 color = baseColor.xyz * 0.1 + baseColor.xyz * dotClamp(L, N) * intensity;
    // float3 color = baseColor.xyz;
    // return float4(color, baseColor.w);
    output.color = baseColor;
    output.pos = float4(input.posWorld.xyz, abs(input.posView.z));
    output.normal = float4(normalize(input.normalWorld.xyz), 0);
    output.albedo = baseColor;
    return output;
}