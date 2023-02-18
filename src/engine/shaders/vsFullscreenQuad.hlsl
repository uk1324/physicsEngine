struct VsOut {
	float2 worldPos : WorldPos;
	float4 pos : Sv_Position;
};

cbuffer ConstantBuffer {
	float3x2 transform;
};

VsOut main(float2 pos : Position) {
	VsOut o;
	o.pos = float4(pos, 0.0, 1.0);
	o.worldPos = mul(float3(pos, 1.0), transform);
	return o;
}