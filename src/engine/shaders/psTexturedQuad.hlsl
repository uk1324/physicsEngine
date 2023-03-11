Texture2D image : Texture : register(t0);
SamplerState samplerState : Sampler : register(s0);

float4 main(float2 texturePos : TexturePos) : Sv_Target{
	// float width, height;
	// image.GetDimensions(width, height);
	// return  float4(width, height, 0, 1);
	float4 val = image.Sample(samplerState, texturePos);
	float v = val.r;
	/*float d = val.r * (val.a == 0 ? 1.0 : -1.0);*/
	float d = val.r / 250.0;

	/*float3 col = (d > 0.0) ? float3(0.9, 0.6, 0.3) : float3(0.65, 0.85, 1.0);*/
	float3 col = (d > 0.0) ? float3(0.35, 0.35, 0.35) : float3(1.0, 1.0, 1.0);
	/*float3 col = (d > 0.0) ? float3(1.0, 1.0, 1.0) : float3(0.5, 0.5, 0.5);*/
	col *= 1.0 - exp(-6.0 * abs(d));
	col *= 0.8 + 0.2 * cos(150.0 * d);
	col = lerp(col, float3(1.0, 1.0, 1.0), 1.0 - smoothstep(0.0, 0.01, abs(d)));
	return float4(col, 1.0);
	//float step = 0.03f;
	//float3 color = float3(0, 0, 0);
	//if (fmod(v, step) < step / 2.0) {
	//	if (val.a == 0) {
	//		color = float3(0.5, 0.5, 0.5);
	//	} else {
	//		color = float3(1, 1, 1);
	//	}
	//}
	//return float4(color, 1);
//return float4(1, 1, 0, 1);

	//float2 size;
	//image.GetDimensions(size.x, size.y);
	//int2 sizeInt = int2(size) - int2(1, 1);

	//int radius = 1;
	//int2 pos = int2(floor(texturePos * size));
	//bool goal = !(image.Load(int3(pos, 0)).r > 0.5f);
	//while (radius < sizeInt.x) {
	//	int2 min = clamp(pos - int2(radius, radius), 0, sizeInt);
	//	int2 max = clamp(pos + int2(radius, radius), 0, sizeInt);
	//	for (int y = min.y; y <= max.y; y++) {
	//		for (int x = min.x; x <= max.x; x++) {
	//			//float2 p = float2(x, y);
	//			/*if (dot(p, p) > float(radius)) {
	//				continue;
	//			}*/
	//			if ((image.Load(int3(x, y, 0)).r > 0.5f) == goal) {
	//				////if (bitset.test(ry * texture.size().y + rx) == goal) {
	//				float v = fmod(float(radius) / 5.0, 1.0);
	//				if (goal == true) {
	//					v /= 2;
	//				}
	//				return float4(v, v, v, 1);
	//				//return float4(float2(x, y) / size, 0, 1);
	//			}
	//		}
	//	}
	//	radius++;
	//}
	//return float4(0, 1, 1, 1);
}