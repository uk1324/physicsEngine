float4 main(float2 texturePos : TexturePos, float invScale : InvScale, float3 color : Color) : Sv_Target {
	float2 pos = (texturePos - float2(0.5, 0.5)) * 2.0;

	const float halfWidth = 0.006 * invScale;
	const float iterpolationWidth = 0.004 * invScale;

	float2 vectorParallelToLine = float2(1.0, 0.0);
	float distanceAlongLine = dot(vectorParallelToLine, pos);
	distanceAlongLine = clamp(distanceAlongLine, 0.0, 1.0 - halfWidth /* Read renderer.cpp extraLength */);
	float2 closestPointToPosOnLine = vectorParallelToLine * distanceAlongLine;

	float distanceToLine = distance(pos, closestPointToPosOnLine);

	float4 colorTransparent = float4(color, 0.0);
	return lerp(colorTransparent, float4(color, 1.0), smoothstep(halfWidth, halfWidth - iterpolationWidth, distanceToLine));
}