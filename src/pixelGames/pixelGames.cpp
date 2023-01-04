#include <pixelGames/pixelGames.hpp>
#include <engine/window.hpp>
#include <engine/time.hpp>

PixelGames::PixelGames(Gfx& gfx)
	: texture{ gfx, Vec2T<i32>{ 101, 101 } } {}

auto PixelGames::update(Gfx& gfx, Renderer& renderer) -> void {
	elapsed += Time::deltaTime();

	for (i32 iY = 0; iY < texture.size().y; iY++) {
		for (i32 iX = 0; iX < texture.size().x; iX++) {
			const auto pos = Vec2{ static_cast<float>(iX), static_cast<float>(iY) } / Vec2{ texture.size() } - Vec2{ 0.5f };
			Vec3 col = { pos.x, pos.y, sin(elapsed) };
			const Vec3T<u8> colU8{ 
				static_cast<u8>(std::clamp(col.x * 255.0f, 0.0f, 255.0f)), 
				static_cast<u8>(std::clamp(col.y * 255.0f, 0.0f, 255.0f)),
				static_cast<u8>(std::clamp(col.z * 255.0f, 0.0f, 255.0f))
			};
			texture.set({ iX, iY }, colU8);
		}
	}

	Camera camera;
	camera.aspectRatio = Window::aspectRatio();

	renderer.drawDynamicTexture(Vec2{ 0.0f }, camera.aspectRatio / 3.0f, texture);

	renderer.update(gfx, camera, Window::size(), false);
}
