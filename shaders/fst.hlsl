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
  return tex.Sample(nearestSampler, input.uv);
}