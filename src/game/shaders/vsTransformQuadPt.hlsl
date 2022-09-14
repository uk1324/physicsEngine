struct CircleInstance {
	float3x2 transform;
	float3 color;
};

cbuffer ConstantBuffer {
	CircleInstance instanceData[100];
};

struct VsOut {
	float2 texturePos : TexturePos;
	float3 color : Color;
	float4 pos : Sv_Position;
};

VsOut main(float2 pos : Position, float2 texturePos : TexturePos, uint instanceId : Sv_InstanceId) {

	VsOut o;
	CircleInstance instance = instanceData[instanceId];
	o.texturePos = texturePos;
	o.color = instance.color;
	o.pos = float4(mul(float3(pos, 1.0f), instance.transform).xy, 0.0, 1.0);
	return o;
}