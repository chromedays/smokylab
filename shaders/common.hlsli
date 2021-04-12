#define PI 3.141592
#define PI2 6.283185

float2 sampleEquirectangularMap(float3 normal) {
    float2 texcoord = float2(0.5 - atan2(normal.z, normal.x) / PI2, acos(normal.y) / PI);
    return texcoord;
}

float3 sampleNormalFromLongLat(float2 uv) {
    float3 normal = normalize(float3(cos(PI2* (0.5 - uv.x))*sin(PI * uv.y), cos(PI * uv.y), sin(PI2*(0.5 - uv.x))*sin(PI*uv.y)));
    return normal;
}

float dotClamp(float3 a, float3 b) {
    return max(dot(a, b), 0.00001);
}

float3 linearToGamma(float3 color) {
    return pow(abs(color), 1 / 2.2);
}

float3 gammaToLinear(float3 color) {
    return pow(abs(color), 2.2);
}

#define NUM_SAMPLES 40

cbuffer ViewUniforms : register(b0) {
    float4x4 viewMat;
    float4x4 projMat;
    float4 viewPos;
    int4 skySize;
    float4 randomPoints[NUM_SAMPLES];
    float4 exposureNearFar;
    float4 dirLightDirIntensity;
};

cbuffer DrawUniforms : register(b1) {
    float4x4 modelMat;
    float4x4 invModelMat;
};

cbuffer MaterialUniforms : register(b2) {
    float4 baseColorFactor;
    float4 metallicRoughnessFactor;
};

Texture2D<float4> skyTexture : register(t0);
SamplerState skySampler : register(s0);
Texture2D<float4> irrTexture : register(t1);
SamplerState irrSampler : register(s1);

Texture2D<float4> baseColorTexture : register(t2);
SamplerState baseColorSampler : register(s2);

Texture2D<float4> metallicRoughnessTexture : register(t3);
SamplerState metallicRoughnessSampler : register(s3);