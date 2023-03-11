#include <demos/marchingSquaresDemo.hpp>
#include <engine/debug.hpp>
#include <engine/window.hpp>
#include <imgui/imgui.h>

using namespace ImGui;

#define PATH "C:/Users/user/Downloads/ffmpeg-5.1.2-essentials_build/ffmpeg-5.1.2-essentials_build/bin/"

MarchingSquaresDemo::MarchingSquaresDemo()
	: texture{ []() {
		const auto image = ImageRgba::fromFile(PATH "out1.png");
		//const auto image = ImageRgba::fromFile("C:/Users/user/Desktop/bez.png");
		DynamicTexture texture{ image->size() };
		//DynamicTexture texture{ image->size() / 10 };
		if (image.has_value()) {
			texture.copyAndResize(*image);
		}
		return texture;
	}() } {

	const auto gridSize = Vec2{ texture.size() };
	const auto gridCenter = gridSize / 2.0f;
	camera.pos = gridCenter;
	camera.setWidth(gridSize.x);
	if (gridSize.y > camera.height()) {
		camera.setWidth(gridSize.y * camera.aspectRatio);
	}

	Window::setSize(Vec2{ texture.size() * 3 });
}

#include <math/polygon.hpp>

#include <engine/frameAllocator.hpp>
#include <set>
#include <engine/input.hpp>
#include <math/marchingSquares.hpp>
#include <utils/dbg.hpp>

template<typename T>
static auto get(const T& array, i64 index) -> auto {
	// TODO: Wrap around points like -2 or size + 2 correctly.
	if (index < 0) {
		index = array.size() - 1;
	} else if (index >= static_cast<i64>(array.size())) {
		index = 0;
	}
	return array[index];
}

auto MarchingSquaresDemo::update() -> void {
	const auto gridSize = Vec2{ texture.size() };
	const auto gridCenter = gridSize / 2.0f;
	camera.pos = gridCenter;
	camera.setWidth(gridSize.x);
	camera.scrollOnCursorPos();

	Begin("marching squares");
	Checkbox("draw image", &drawImage);
	Checkbox("pixel perfect", &pixelPerfect);
	Checkbox("connect diagonals", &connectDiagonals);
	End();

	if (drawImage) {
		Renderer::drawDynamicTexture(gridCenter, gridSize.y, texture);
	}

	if (Input::isKeyDown(Keycode::A))
		paused = !paused;

	if (Input::isKeyDown(Keycode::LEFT)) {
		frame--;
	}

	if (Input::isKeyDown(Keycode::RIGHT)) {
		frame++;
	}
	auto image = ImageRgba::fromFile(frameAllocator.format(PATH "out%d.png", frame).data());
	if (!paused)
		frame++;

	if (image.has_value()) {
		visited.clear();
		vertices.clear();
		texture.copyAndResize(*image);
		for (auto& p : texture) {
			p = p.grayscaled().r > 127 ? PixelRgba::WHITE : PixelRgba::BLACK;
		}
		vertices = ::marchingSquares(texture, pixelPerfect, connectDiagonals);
	}

	for (int i = static_cast<int>(vertices.size() - 1); i >= 0; i--) {
		if (vertices[i].size() < 3) {
			vertices.erase(vertices.begin() + i);
		}
	}

	chk(removeHoles) {
		::removeHoles(vertices, false);
	}


	static float max = 1.5f;
	chkbox(doDp);
	if (doDp) {
		for (auto& lines : vertices) {
			lines = polygonDouglassPeckerSimplify(lines, max);
		}
	}
	chk(removenear) {
		for (auto& v : vertices) {
			std::vector<Vec2> r;
			for (int i = 0; i < v.size(); i++) {
				auto a = get(v, i);
				auto b = get(v, i + 1);
				if (distance(a, b) > 2.5f) {
					r.push_back(a);
				}
			}
			v = r;
		}
	}

	chk(removeNear2) {
		for (auto& v : vertices) {
			const auto sizeBefore = vertices.size();
			std::vector<Vec2> r;
			for (int i = 0; i < v.size(); i++) {
				auto a = get(v, i);
				auto b = get(v, i + 1);
				if (distance(a, b) > 2.5f) {
					r.push_back(a);
				}
			}
			//if (v.size() != 0 && v.size() != sizeBefore) {
			//	Debug::drawPoint(v[0]);
			//}

			v = r;
		}
	}

	static bool testa = true;
	ImGui::Checkbox("testa", &testa);
	for (const auto& lines : vertices) {
		Vec3 color = Vec3::RED;
		if (simplePolygonIsClockwise(lines)) {
			color = Vec3::GREEN;
		}
		if (simplePolygonArea(lines) < 0.01f) {
			color = Vec3{ 255, 255, 0 } / 255.0f;
		}

		if (testa) {
			Debug::drawLines(lines, color);
		} else {
			if (simplePolygonIsClockwise(lines)) {
				const auto& triangles = triangulate(lines);
				for (const auto& tri : triangles) {
					Debug::drawLines(tri.v, Vec3::WHITE);
				}
			}
		}

		const auto dotSize = 15.0f / texture.size().x;
		for (const auto& vert : lines) {
			Debug::drawCircle(vert, dotSize, Vec3::RED);
		}
	}

	Renderer::update(camera);
}

