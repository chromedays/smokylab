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

Texture2D<float4> positionTex : register(t0);
Texture2D<float4> normalTex : register(t1);
Texture2D<float4> albedoTex : register(t2);
SamplerState nearestSampler : register(s0);

FS_OUTPUT frag(FS_INPUT input) {
    FS_OUTPUT output;
    
    // TODO: Fix potential overflow
    uint2 fragCoord = (uint2)input.pos.xy;

    float4 pos = positionTex.Load(int3(fragCoord.xy, 0));
    float3 P = pos.xyz;
    float d = pos.w;
    float3 N = normalize(normalTex.Load(int3(fragCoord.xy, 0)).xyz);
    float3 albedo = albedoTex.Load(int3(fragCoord.xy, 0)).xyz;

    int n = (int)ssaoFactors.x;
    float R = ssaoFactors.y;
    float c = R * 0.1;
    float s = ssaoFactors.z;
    float k = ssaoFactors.w;

    float randomRot =
        frac((float)(((30 * fragCoord.x) ^ fragCoord.y) + 10 * fragCoord.x * fragCoord.y) / PI2) * PI2;
    float thetaCoeff = PI2 * (7.0 * (float)n / 9.0);

    float S = 0;

    for (int i = 0; i < n; ++i) {
        float a = ((float)i + 0.5) / (float)n;
        float h = a * R / d;
        float theta = thetaCoeff * a + randomRot;
        float2 texcoord = input.uv + float2(h * cos(theta), h * sin(theta));
        float3 P_i = positionTex.Sample(nearestSampler, texcoord).xyz;
        float3 wi = P_i - P;
        S += (max(0, dot(N, wi) - 0.001 * d) * ((R - length(wi)) > 0 ? 1.0 : 0.0)) / max(c*c, dot(wi, wi));
    }
    S *= ((PI2 * c) / (float)n);

    float A = pow(saturate(1 - s * S), k);

    output.color = float4(A.xxx, 1);
    // output.color = float4(albedo * A, 1);
    // output.color = float4(d.xxx, 1);

    return output;
}