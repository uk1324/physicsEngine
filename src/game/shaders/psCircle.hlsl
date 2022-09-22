float4 main(
	float2 texturePos : TexturePos, 
	float widthScale : WidthScale, 
	float3 col: Color) : Sv_Target {

	float2 pos = (texturePos - float2(0.5, 0.5)) * 2.0;
	
	float4 color = float4(col, 1.0);
	float4 colorTransparent = float4(color.rgb, 0.0);
	// MSAA only works for built-in primitives so this has to be done manually.
	// TODO: Change the min and max based on the buffer size or maybe window size.
	// Maybe actually preform MSAA and sample at subpixel locations.

	float distance = length(pos);
	const float width = 0.006 * widthScale;
	const float iterpolationWidth = 0.004 * widthScale;
	float4 circleColor = lerp(
		lerp(colorTransparent, color, smoothstep(1.0 - width - iterpolationWidth * 2, 1.0 - width - iterpolationWidth, distance)), 
		colorTransparent, 
		smoothstep(1.0 - iterpolationWidth, 1.0, distance)
	);

	if (distance < 1.0) {
		float2 normal = float2(1.0, 0.0);
		float2 direction = float2(-normal.y, normal.x);
		float projectionLength = dot(pos, direction);
		if (projectionLength > 0.0 && projectionLength < 1.0 - iterpolationWidth) {
			float distance = abs(dot(pos, normal));
			circleColor += lerp(color, colorTransparent, smoothstep(width - iterpolationWidth, width, distance));
		}
	}

	return circleColor;
}