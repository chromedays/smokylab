#pragma pack_matrix(row_major)

struct VS_INPUT {
  float4 pos : POSITION;
};

struct FS_INPUT {
  float4 pos : SV_POSITION;
};

struct FS_OUTPUT {
  float4 color : SV_TARGET;
};

FS_INPUT vert(VS_INPUT input) {
  FS_INPUT output;
  float4 posWorld = mul(float4(input.pos));
}

FS_OUTPUT frag(FS_INPUT input) {
  FS_OUTPUT output;
  output.color = float4(1, 0, 0, 1);
}
