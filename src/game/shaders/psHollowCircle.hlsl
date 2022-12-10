#include "debugShapesPixelShaderConstantBuffer.hlsl"

float4 main(float2 texturePos : TexturePos, float widthScale : WidthScale, float3 col : Color) : Sv_Target {
	float2 pos = (texturePos - float2(0.5, 0.5)) * 2.0;

	float4 color = float4(col, 1.0);
	float4 colorTransparent = float4(color.rgb, 0.0);

	float distance = length(pos);
	widthScale /= 2.0;
	const float size = lineWidth * widthScale;
	const float interpolationWidth = smoothingWidth * widthScale;
	float4 circleColor = lerp(
		lerp(colorTransparent, color, smoothstep(1.0 - size - interpolationWidth * 2, 1.0 - size - interpolationWidth, distance)),
		colorTransparent,
		smoothstep(1.0 - interpolationWidth, 1.0, distance)
	);

	return circleColor;
}