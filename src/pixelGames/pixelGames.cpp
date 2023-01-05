#include <pixelGames/pixelGames.hpp>
#include <engine/window.hpp>
#include <engine/time.hpp>
#include <math/utils.hpp>

#include <imgui/imgui.h>

#include <complex>

PixelGames::PixelGames(Gfx& gfx)
	: texture{ gfx, Vec2T<i32>{ 41, 41 } } {}


using namespace ImGui;

auto PixelGames::update(Gfx& gfx, Renderer& renderer) -> void {

	Begin("game");
	const auto old = doFourierTransform;
	Checkbox("fourier transform", &doFourierTransform);
	const auto doFourierTransformChanged = old != doFourierTransform || elapsed == 0.0f;

	auto get = [](void* data, int i) -> float {
		return std::abs((*reinterpret_cast<decltype(result)*>(data))[i]);
	};
	ImGui::PlotLines("test", static_cast<float(*)(void*, int)>(get), &result, result.size());

	End();

	elapsed += Time::deltaTime();
	ImGui::ShowDemoWindow();

	/*11:20 It might be important to note that you can't generally overlay multiple FFTs like that and have the frequencies align nicely. 
		This only works if your input signals are all the same length(and sampling rate) because the frequency domain spacing is inversely proportional to the length of the time series.*/

	if (doFourierTransformChanged) {
		if (doFourierTransform) {
			result.clear();
			using namespace std::complex_literals;
			//std::vector<std::complex<float>> input = { 1.0f, 2.0f - 1if, -1if, -1.0f + 2if };
			for (usize _ = 0; _ < texture.size().x; _++) {
				std::complex<float> value = 0.0f;
				for (usize i = 0; i < texture.size().x; i++) {
					auto input = static_cast<float>(texture.get(Vec2T{ static_cast<i32>(i), 2 }).x);
					input -= 128;
					// Could precomute the powers e^(tau * i / size * _).
					value += input * std::exp(-TAU<float> * 1if* static_cast<float>(i)* static_cast<float>(_) / static_cast<float>(texture.size().x));
				}
				result.push_back(value);
			}

			float min = std::numeric_limits<float>::infinity(), max = -std::numeric_limits<float>::infinity();
			for (i32 x = 0; x < texture.size().x; x++) {
				const auto value = std::abs(result[x]);
				min = std::min(min, value);
				max = std::max(max, value);
			}

			// Show the transform
			//for (i32 x = 0; x < texture.size().x; x++) {
			//	auto value = std::abs(result[x]);
			//	value = (static_cast<float>(value) + min) / (min + max);
			//	texture.set(Vec2T{ x, 3 }, Vec3T<u8>{ static_cast<u8>(value * 255.0f) });
			//}

			// Inverse the transform
			for (usize _ = 0; _ < texture.size().x; _++) {
				std::complex<float> value = 0.0f;
				for (usize i = 0; i < texture.size().x; i++) {
					value += result[i] * std::exp(TAU<float> * 1if * static_cast<float>(i) * static_cast<float>(_) / static_cast<float>(texture.size().x));
				}
				value /= texture.size().x;
				value += 128;
				texture.set(Vec2T{ static_cast<i32>(_), 2 }, Vec3T<u8>{ static_cast<u8>(std::abs(value)) });
			}


		} else {
			for (i32 iY = 0; iY < texture.size().y; iY++) {
				for (i32 iX = 0; iX < texture.size().x; iX++) {
					auto pos = (Vec2{ static_cast<float>(iX), static_cast<float>(iY) } / Vec2{ texture.size() } - Vec2{ 0.5f }) * 2.0f;
					pos.x *= 10.0f;

					float value;

					auto f = [](float x) -> float {
						/*return (sin(x * 2.0f * PI<float>) + 1.0f) / 2.0f;*/
						//return sin(x);
						//return sin(x) / x;
						return sin(x) + sin(3 * x);
						/*if (abs(x) < 1.0) {
							return 1.0f;
						}
						return -1.0f;*/
					};

					value = f(pos.x);
					value = (value + 1.0f) / 2.0f;
					Vec3 col = { value };
					const Vec3T<u8> colU8{
						static_cast<u8>(std::clamp(col.x * 255.0f, 0.0f, 255.0f)),
						static_cast<u8>(std::clamp(col.y * 255.0f, 0.0f, 255.0f)),
						static_cast<u8>(std::clamp(col.z * 255.0f, 0.0f, 255.0f))
					};
					texture.set({ iX, iY }, colU8);
				}
			}
		}
		
	}

	Camera camera;
	camera.aspectRatio = Window::aspectRatio();

	renderer.drawDynamicTexture(Vec2{ 0.0f }, camera.aspectRatio / 3.0f, texture);

	renderer.update(gfx, camera, Window::size(), false);
}
