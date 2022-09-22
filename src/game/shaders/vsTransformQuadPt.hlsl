struct CircleInstance {
	float radiusInverse;
	float3x2 transform;
	float3 color;
};

cbuffer ConstantBuffer {
	CircleInstance instanceData[100];
};

struct VsOut {
	float2 texturePos : TexturePos;
	float3 color : Color;
	float widthScale: WidthScale;
	float4 pos : Sv_Position;
};

VsOut main(float2 pos : Position, float2 texturePos : TexturePos, uint instanceId : Sv_InstanceId) {

	CircleInstance instance = instanceData[instanceId];
	VsOut o;
	o.texturePos = texturePos;
	o.color = instance.color;
	// Can't just use the determinant of the transform because of the scale transform used to adapt to screen size.
	o.widthScale = instance.radiusInverse;
	o.pos = float4(mul(float3(pos, 1.0), instance.transform).xy, 0.0, 1.0);
	return o;
}