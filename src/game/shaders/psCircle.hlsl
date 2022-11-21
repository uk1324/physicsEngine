#include "debugShapesPixelShaderConstantBuffer.hlsl"

float4 main(float2 texturePos : TexturePos, float widthScale : WidthScale, float3 col : Color) : Sv_Target{

	float2 pos = (texturePos - float2(0.5, 0.5)) * 2.0;

	float4 color = float4(col, 1.0);
	float4 colorTransparent = float4(color.rgb, 0.0);

	float distance = length(pos);
	const float size = 0.006 * widthScale;
	const float interpolationWidth = smoothingWidth * widthScale;
	return lerp(color, colorTransparent, smoothstep(1.0 - interpolationWidth, 1.0, distance));
}