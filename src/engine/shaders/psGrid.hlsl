cbuffer ConstantBuffer {
	float cameraZoom;
	float smallCellSize;
};

float4 main(float2 worldPos : WorldPos) : Sv_Target{
	float halfSmallCellSize = smallCellSize / 2.0;
	// Translate so every cell contains a cross of lines instead of a square. When a square is used it contains 4 lines which makes coloring specific lines difficult. Centering it makes it so there are only 2 lines in each cell.
	worldPos += float2(halfSmallCellSize, halfSmallCellSize);
	float2 posInCell = fmod(abs(worldPos), smallCellSize) - float2(halfSmallCellSize, halfSmallCellSize); // Translate [0, 0] to center of cell.
	//posInCell *= sign(worldPos);

	float dVertical = abs(dot(posInCell, float2(1.0, 0.0)));
	float dHorizontal = abs(dot(posInCell, float2(0.0, 1.0)));
	float width = 0.001 / cameraZoom;
	float interpolationWidth = width / 5.0f;
	// Flip colors by making the second argument to smoothstep smaller than the first one.
	dVertical = smoothstep(width, width - interpolationWidth, dVertical);
	dHorizontal = smoothstep(width, width - interpolationWidth, dHorizontal);

	float3 col0 = float3(0.12, 0.12, 0.12);
	float3 col1 = col0 / 2.0;
	float2 cellPos = int2(floor(worldPos / smallCellSize));
	float3 colVertical = (cellPos.x % 4 == 0) ? col0 : col1;
	float3 colHorizontal = (cellPos.y % 4 == 0) ? col0 : col1;
	colVertical *= dVertical;
	colHorizontal *= dHorizontal;

	return float4(colVertical + colHorizontal, 1);
}