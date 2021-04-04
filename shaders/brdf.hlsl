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

float ggxDistribution(float a, float NdotH) {
    float a2 = a * a;
    float denom = NdotH * NdotH * (a2 - 1) + 1;
    denom = PI * denom * denom;
    float D = a2 / denom;
    return D;
}

float ggxSmithG1(float NdotV, float a) {
    float a2 = a * a;
    float G1 = (2 * NdotV) / (NdotV + sqrt(a2 + (1 - a2) * (NdotV * NdotV)));
    return G1;
}

float ggxSmithGeometry(float NdotL, float NdotV, float a) {
    float G1 = ggxSmithG1(NdotL, a);
    float G2 = ggxSmithG1(NdotV, a);
    float G =  G1 * G2;
    return G;
}

float3 schlickFresnel(float VdotH, float3 F0) {
    float3 F = F0 + (1.0.xxx - F0) * pow((1 - VdotH), 5);
    return F;
}

float3 schlickFresnelRoughness(float VdotH, float3 F0, float a) {
    float3 F = F0 + (max((1 - a).xxx, F0) - F0) * pow((1 - VdotH), 5);
    return F;
}

float distributionTerm(float roughness, float3 N, float3 H) {
    return (roughness + 2) / (2 * PI) * pow(dot(N, H), roughness);
}

float4 frag(FS_INPUT input) : SV_Target {
    float4 baseColor = baseColorFactor * baseColorTexture.Sample(baseColorSampler, input.texcoord);
    float2 metallicRoughness =
        metallicRoughnessFactor.xy * metallicRoughnessTexture.Sample(metallicRoughnessSampler, input.texcoord).zy;
    float metallic = metallicRoughness.x;
    float roughness = metallicRoughness.y;

    if (metallic < 0.5) {
        baseColor.xyz = float3(0.04, 0, 0.04);
    }
    float3 irradiance = irrTexture.Sample(irrSampler, sampleEquirectangularMap(input.normalWorld)).xyz;
    float3 diffuse = (irradiance * baseColor.xyz);

    float3 F0 = 0.04.xxx;
    F0 = lerp(F0, baseColor.xyz, metallic.xxx);
    
    float3 N = normalize(input.normalWorld);
    float3 V = normalize(viewPos.xyz - input.posWorld.xyz);
    float3 R = normalize(2*dot(N, V)*N-V);
    float3 up = abs(N.y) < 0.999 ? float3(0.0, 1.0, 0.0) : float3(0.0, 0.0, 1.0);
    // float3 up = float3(0.0, 1.0, 0.0);
    float3 A = normalize(cross(R, up));
    float3 B = normalize(cross(A, R));

    float a = max(roughness * roughness, 0.00001);
    // float a = roughness * roughness;
    
    float NdotV = dotClamp(N, V);

    float3 specular = 0.0.xxx;
    float lodParam1 = 0.5 * log2((float)skySize.x * (float)skySize.y / (float)NUM_SAMPLES);
    for (int i = 0; i < NUM_SAMPLES; ++i) {
        float2 xi = randomPoints[i].xy;
        float aa = 2/(a*a)-2;
        xi = float2(xi.x*2, acos(pow(xi.y, 1.0 / (aa+1)))/PI);
        float3 L = sampleNormalFromLongLat(xi);
        L = normalize(L.x * A + L.y * R + L.z * B); // World L
        float3 H = normalize(L + V);

        float NdotH = dotClamp(N, H);
        float NdotL = dotClamp(N, L);
        float VdotH = dotClamp(V, H);

        float D = ggxDistribution(a, NdotH);
        float lod = max(lodParam1 - 0.5 * log2(max(D, 0.00001)), 0);
        float2 skyTexcoord = sampleEquirectangularMap(L);
        float3 iblSpecular = skyTexture.SampleLevel(skySampler, skyTexcoord, lod).xyz;
        float3 F = schlickFresnel(VdotH, F0);
        float G = ggxSmithGeometry(NdotL, NdotV, a);
        specular += (iblSpecular * F * G * NdotL) / (4 * NdotL * NdotV);
    }
    specular /= NUM_SAMPLES;

    float3 F = schlickFresnelRoughness(NdotV, F0, a);
    float3 ks = F;
    float3 kd = 1 - ks;
    kd *= (1 - metallic);

    float3 color = kd * diffuse + ks * specular;
    color = (exposure.x * color) / (exposure.x * color + float3(1, 1, 1));
    color = pow(color, 1 / 2.2);
    // color = specular;
    // {
    //     float3 L = float3(1, 1, 1);
    //     float3 H = normalize(L + V);
    //     float NdotH = dotClamp(N, H);
    //     color = ggxDistribution(a, NdotH).xxx;
    // }
    // color = ks;
    return float4(color, 1);
}