#define _CRT_SECURE_NO_WARNINGS
#include <raylib.h>
#define RAYGUI_IMPLEMENTATION
#include <raygui.h>

#include <iostream>

#include "../../src/math/vec2.hpp"

auto to(Vec2 v) -> Vector2 {
	return Vector2{ v.x, v.y };
}

auto main() -> int {
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(640, 480, "game");
	SetWindowState(FLAG_WINDOW_MAXIMIZED);

	Camera2D camera{
		.offset = Vector2{ 0.0f, 0.0f },
		.target = Vector2{ 0.0f, 0.0f },
		.rotation = 0.0f,
		.zoom = 1.0f,
	};

	while (!WindowShouldClose()) {
		Vec2 dir{ 0.0f };
		if (IsKeyDown(KEY_W)) {
			dir.y += 1.0f;
		}
		if (IsKeyDown(KEY_S)) {
			dir.y -= 1.0f;
		}
		if (IsKeyDown(KEY_A)) {
			dir.x += 1.0f;
		}
		if (IsKeyDown(KEY_D)) {
			dir.x -= 1.0f;
		}
		dir = dir.normalized();
		camera.offset.x += dir.x;
		camera.offset.y += dir.y;

		const auto scroll = GetMouseWheelMove();
		if (scroll != 0.0f) {
			if (scroll < 0.0f) {
				camera.zoom /= 2.0f;
			} else {
				camera.zoom *= 2.0f;
			}
		}

		BeginDrawing();
		ClearBackground(BLACK);

		BeginMode2D(camera);

		DrawCircleLines(20, 20, 50, BLUE);
		//DrawGrid(5, 0.2f);

		EndMode2D();

		EndDrawing();
	}

	CloseWindow();
}