float4 main(float2 texturePos : TexturePos) : Sv_Target {
	float2 pos = (texturePos - float2(0.5f, 0.5f)) * 2.0f;
	
	// MSAA only works for built-in primitives so this has to be done manually.
	float4 color = float4(1.0, 0.0f, 0.0f, 1.0f);
	float4 transparentColor = float4(color.rgb, 0.0f);
	return lerp(color, transparentColor, smoothstep(0.95f, 1.0f, dot(pos, pos)));
}