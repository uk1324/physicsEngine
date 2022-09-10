struct CircleInstance {
	float3x2 transform;
};

cbuffer ConstantBuffer {
	CircleInstance instanceData[100];
};

struct VsOut {
	float2 texturePos : TexturePos;
	float4 pos : Sv_Position;
};

VsOut main(float2 pos : Position, float2 texturePos : TexturePos, uint instanceId : Sv_InstanceId) {

	VsOut o;
	o.texturePos = texturePos;
	o.pos = float4(mul(float3(pos, 1.0f), instanceData[instanceId].transform).xy, 0.0, 1.0);
	return o;
}