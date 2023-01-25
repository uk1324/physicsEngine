struct VsOut {
	float2 texturePos : TexturePos;
	float4 pos : Sv_Position;
};

cbuffer ConstantBuffer {
	float3x2 transform;
};

VsOut main(float2 pos : Position, float2 texturePos : TexturePos) {
	VsOut o;
	o.texturePos = texturePos;
	o.pos = float4(mul(float3(pos, 1.0), transform), 0.0, 1.0);
	return o;
}