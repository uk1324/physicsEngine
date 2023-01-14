#include <pixelGames/fourierTransformDemo.hpp>
#include <engine/window.hpp>
#include <engine/time.hpp>
#include <math/fourier.hpp>
#include <math/aabb.hpp>
#include <math/utils.hpp>
#include <customImguiWidgets.hpp>
#include <engine/input.hpp>

#include <imgui/imgui.h>

FourierTransformDemo::FourierTransformDemo(Gfx& gfx)
	: texture{ gfx, Vec2T<usize>{ IMAGE_SIZE } } {
	for (auto pixel : texture.indexed()) {
		pixel = PixelRgba{ (pixel.pos.x > texture.size().x / 2) ? u8(0) : u8(255) };
	}
}

using namespace ImGui;

auto FourierTransformDemo::update(Gfx& gfx, Renderer& renderer) -> void {

	Begin("fourier transform");

	TextWrapped("The low frequencies lie on edges");
	// When the whole image is white the only frequency is in the top left corner.
	TextWrapped("Put a few pixels on an empty image and transform");
	TextWrapped("Draw some simple shape and then erase the low frequencies from the transform");
	const auto changed = Checkbox("transformed", &transformed);
	auto redraw = changed;

	if (transformed) {
		if ((displayMagnitudeIfFalsePhaseIfTrue && Button("display magnitude"))
			|| (!displayMagnitudeIfFalsePhaseIfTrue && Button("display phase"))) {
			displayMagnitudeIfFalsePhaseIfTrue = !displayMagnitudeIfFalsePhaseIfTrue;
			redraw = true;
		}
	} else {
		if (const auto image = imageFileSelect("load image")) {
			texture.copyAndResize(*image);
			texture.grayscale();
		}
	}
	imageSaveFileSelect(texture, "save image");
	SliderInt("brush size", &brushSize, 0, 10);
	DragScalar("brush color", ImGuiDataType_U8, &brushColor);
	if (Button("clear")) {
		for (auto pixel : texture.indexed()) {
			pixel = PixelRgba{ 0 };
			fourierTransform[pixel.pos.x][pixel.pos.y] = 0.0;
		}
	}
	End();

	if (changed) {
		std::vector<std::complex<double>> fftTemp;
		fftTemp.resize(IMAGE_SIZE);
		if (transformed) {
			for (i64 x = 0; x < texture.size().x; x++) {
				// Increase the precision by using normalizing. When using the 0 to 255 range there are visible artifacts.
				for (i64 y = 0; y < texture.size().y; y++) fftTemp[y] = texture.get({ x, y }).r / 255.0;
				fft(fftTemp);
				for (i64 y = 0; y < texture.size().y; y++) fourierTransform[x][y] = fftTemp[y];
			}

			for (i64 y = 0; y < texture.size().y; y++) {
				for (i64 x = 0; x < texture.size().x; x++) fftTemp[x] = fourierTransform[x][y];
				fft(fftTemp);
				for (i64 x = 0; x < texture.size().x; x++) fourierTransform[x][y] = fftTemp[x];
			}

		} else {
			for (i64 y = 0; y < texture.size().y; y++) {
				for (i64 x = 0; x < texture.size().x; x++) fftTemp[x] = fourierTransform[x][y];
				inverseFft(fftTemp);
				for (i64 x = 0; x < texture.size().x; x++) fourierTransform[x][y] = fftTemp[x];
			}

			for (i64 x = 0; x < texture.size().x; x++) {
				for (i64 y = 0; y < texture.size().y; y++) fftTemp[y] = fourierTransform[x][y];
				inverseFft(fftTemp);
				for (i64 y = 0; y < texture.size().y; y++) fourierTransform[x][y] = fftTemp[y];
			}

			for (i64 y = 0; y < texture.size().y; y++) {
				for (i64 x = 0; x < texture.size().x; x++) {
					// Imaginary parts cancel out.
					texture.set({ x, y }, PixelRgba{ static_cast<u8>(fourierTransform[x][y].real() * 255.0) });
				}
			}
		}
	}

	if (transformed && redraw) {
		// Some programs centered the dft (google).
		for (auto p : texture.indexed()) {
			p = PixelRgba{ static_cast<u8>(std::abs(fourierTransform[p.pos.x][p.pos.y])) };
		}

		for (auto p : texture.indexed()) {
			u8 value;
			if (displayMagnitudeIfFalsePhaseIfTrue) {
				value = static_cast<u8>((std::arg(fourierTransform[p.pos.x][p.pos.y]) + PI<double>) / TAU<float> * 255.0f);
			} else {
				value = static_cast<u8>(std::abs(fourierTransform[p.pos.x][p.pos.y]));
			}
			p = PixelRgba{ value };
		}
	}

	camera.aspectRatio = Window::aspectRatio();
	const auto cursorPos = camera.screenSpaceToCameraSpace(Input::cursorPos());
	const auto dynamicTexturePos = Vec2{ 0.0f };
	static auto dynamicTextureSize = camera.height();
	const auto textureBox = Aabb::fromPosSize(dynamicTexturePos, Vec2{ dynamicTextureSize });

	if (textureBox.contains(cursorPos) && Input::isMouseButtonHeld(MouseButton::LEFT)) {
		const auto p = camera.posInGrid(cursorPos, dynamicTexturePos, dynamicTextureSize, texture.size());
		const auto size = texture.size();
		for (i64 y = std::clamp(p.y - brushSize, 0ll, size.y); y < std::clamp(p.y + brushSize + 1, 0ll, size.y); y++) {
			for (i64 x = std::clamp(p.x - brushSize, 0ll, size.x); x < std::clamp(p.x + brushSize + 1, 0ll, size.x); x++) {
				auto& val = fourierTransform[x][y];
				if (displayMagnitudeIfFalsePhaseIfTrue) {
					val = std::polar((brushColor / 255.0 * TAU<float>), std::abs(val));
				} else {
					val = std::arg(val) * brushColor;
				}

				texture.set({ x, y }, PixelRgba{ brushColor });
			}
		}

	}

	renderer.drawDynamicTexture(dynamicTexturePos, dynamicTextureSize, texture);
	renderer.update(gfx, camera, Window::size(), false);
}
