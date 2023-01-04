Texture2D image : Texture : register(t0);
SamplerState samplerState : Sampler : register(s0);

float4 main(float2 texturePos : TexturePos) : Sv_Target{
	// float width, height;
	// image.GetDimensions(width, height);
	// return  float4(width, height, 0, 1);
	return float4(image.Sample(samplerState, texturePos).rgb, 1);
	//return float4(1, 1, 0, 1);
}