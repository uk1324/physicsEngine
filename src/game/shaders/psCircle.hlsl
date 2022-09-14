float4 main(float2 texturePos : TexturePos, float3 col: Color) : Sv_Target {
	float2 pos = (texturePos - float2(0.5f, 0.5f)) * 2.0f;
	
	float4 color = float4(col, 1.0f);
	float4 colorTransparent = float4(color.rgb, 0.0f);
	// MSAA only works for built-in primitives so this has to be done manually.
	// TODO: Change the min and max based on the buffer size or maybe window size.
	return lerp(color, colorTransparent, smoothstep(0.95f, 1.0f, dot(pos, pos)));
}