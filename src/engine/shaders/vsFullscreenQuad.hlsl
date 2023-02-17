struct VsOut {
	float2 worldPos : WorldPos;
	float4 pos : Sv_Position;
};

cbuffer ConstantBuffer {
	float3x2 transform;
};

VsOut main(float4 pos : Position) {
	VsOut o;
	o.pos = pos;
	o.worldPos = mul(float3(pos.xy, 1.0), transform);
	return o;
}