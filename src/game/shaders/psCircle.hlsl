float4 main(float2 texturePos : TexturePos) : Sv_Target{
	float2 pos = (texturePos - float2(0.5f, 0.5f)) * 2.0f;
	if (dot(pos, pos) > 1.0f)
		discard;
	return float4(1.0f, 0.0f, 0.0f, 0.0f);
}