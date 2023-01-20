struct VsOut {
	float3 color : Color;
	float4 pos : Sv_Position;
};

VsOut main(float2 pos : Position, float3 color : Color) {
	VsOut o;
	o.color = color;
	o.pos = float4(pos, 0, 1);
	return o;
}