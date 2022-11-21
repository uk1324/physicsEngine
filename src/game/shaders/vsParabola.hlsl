struct ParabolaInstance {
	float invScale;
	float3x2 transform;
	float3 color;
	float3 info;
};

cbuffer ConstantBuffer {
	ParabolaInstance instanceData[100];
};

struct VsOut {
	float2 texturePos : TexturePos;
	float invScale : InvScale;
	float3x2 transform: Transform;
	float3 color : Color;
	float3 info: Info;
	float4 pos : Sv_Position;
};

VsOut main(float2 pos : Position, float2 texturePos : TexturePos, uint instanceId : Sv_InstanceId) {
	ParabolaInstance instance = instanceData[instanceId];
	VsOut o;
	o.invScale = instance.invScale;
	o.transform = instance.transform;
	o.texturePos = texturePos;
	o.color = instance.color;
	o.info = instance.info;
	o.pos = float4(pos, 0.0, 1.0);
	//o.pos = float4(mul(float3(pos, 1.0), instance.transform), 0.0, 1.0);
	return o;
}