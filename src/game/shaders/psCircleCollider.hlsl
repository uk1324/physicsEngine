float4 main(
	float2 texturePos : TexturePos, 
	float widthScale : WidthScale, 
	float3 col: Color) : Sv_Target {

	float2 pos = (texturePos - float2(0.5, 0.5)) * 2.0;
	
	float4 color = float4(col, 1.0);
	float4 colorTransparent = float4(color.rgb, 0.0);

	float distance = length(pos);
	widthScale /= 2.0;
	const float size = 0.003 * widthScale;
	const float iterpolationWidth = 0.002 * widthScale;
	float4 circleColor = lerp(
		lerp(colorTransparent, color, smoothstep(1.0 - size - iterpolationWidth * 2, 1.0 - size - iterpolationWidth, distance)), 
		colorTransparent, 
		smoothstep(1.0 - iterpolationWidth, 1.0, distance)
	);

	if (distance < 1.0) {
		float2 normal = float2(0.0, 1.0);
		float2 direction = float2(normal.y, -normal.x);
		float projectionLength = dot(pos, direction); // Could just use determinant.
		if (projectionLength > 0.0 && projectionLength < 1.0 - iterpolationWidth) {
			float distance = abs(dot(pos, normal));
			circleColor += lerp(color, colorTransparent, smoothstep(size - iterpolationWidth, size, distance));
		}
	}

	return circleColor;
}