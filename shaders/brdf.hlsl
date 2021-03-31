#pragma pack_matrix(row_major)

struct VS_INPUT {
    float4 pos : POSITION;
    float4 color : COLOR;
    float4 texcoord : TEXCOORD;
    float4 normal : NORMAL;
};

struct FS_INPUT {
    float4 pos : SV_POSITION;
    float3 posWorld : TEXCOORD0;
    float3 color : COLOR;
};

cbuffer ViewUniforms : register(b0) {
    float4x4 viewMat;
    float4x4 projMat;
};

cbuffer DrawUniforms : register(b1) {
    float4x4 modelMat;
    float4x4 invModelMat;
};

FS_INPUT vert(VS_INPUT input) {
    FS_INPUT output;
    
    float4 posWorld = mul(float4(input.pos.xyz, 1), modelMat);
    output.pos = mul(mul(posWorld, viewMat), projMat);
    output.posWorld = posWorld.xyz;
    output.color = input.normal.xyz;
    return output;
}

float4 frag(FS_INPUT input) : SV_Target {
    return float4(input.color.xyz, 1);
}