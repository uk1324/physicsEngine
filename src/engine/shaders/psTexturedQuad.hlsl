Texture2D image : Texture : register(t0);
SamplerState samplerState : Sampler : register(s0);

float4 main(float2 texturePos : TexturePos) : Sv_Target {
	return float4(image.Sample(samplerState, texturePos).rgb, 1);
	//float4 val = image.Sample(samplerState, texturePos);
	//float v = val.r;
	//float d = val.r / 250.0;

	//float3 col = (d > 0.0) ? float3(0.35, 0.35, 0.35) : float3(1.0, 1.0, 1.0);
	//col *= 1.0 - exp(-6.0 * abs(d));
	//col *= 0.8 + 0.2 * cos(150.0 * d);
	//col = lerp(col, float3(1.0, 1.0, 1.0), 1.0 - smoothstep(0.0, 0.01, abs(d)));
	//return float4(col, 1.0);
}