#include <demos/marchingSquares.hpp>
#include <engine/debug.hpp>
#include <engine/window.hpp>

MarchingSquares::MarchingSquares()
	: texture{ []() {
		const auto image = ImageRgba::fromFile("assets/apple.png");
		Window::setSize(Vec2{ image->size() * 3.0f });
		/*DynamicTexture texture{ image->size() / 7 };*/
		DynamicTexture texture{ image->size() / 7 };
		/*if (image.has_value()) {
			texture.copyAndResize(*image);
		} else {
			ASSERT_NOT_REACHED();
		}
		return texture;*/
		return texture;
	}() } {
}

#include <engine/frameAllocator.hpp>

#define PATH ""
#include <engine/input.hpp>

auto MarchingSquares::update() -> void {
	Camera camera;
	const auto gridSize = Vec2{ texture.size() };
	const auto gridCenter = gridSize / 2.0f;
	camera.pos = gridCenter;
	camera.setWidth(gridSize.x);
	if (gridSize.y > camera.height()) {
		camera.setWidth(gridSize.y * camera.aspectRatio);
	}

	if (Input::isKeyDown(Keycode::A))
		paused = !paused;

	if (paused)
		return;

	if (i % 2 == 0) {
		auto image = ImageRgba::fromFile(frameAllocator.format(PATH "/out%d.png", i / 2).data());
		if (image.has_value()) {
			texture.copyAndResize(*image);
		} else {
			ASSERT_NOT_REACHED();
		}
		/*vertices.clear();
		marchingSquares();*/
	}
	i++; 

	//const auto yScale = 4;
	Vec2 pos{ 0.0f };
	int y = 0;
	while (pos.y < texture.size().y + 1) {
		pos.x = 0.0f;
		while (pos.x < texture.size().x + 1) {
			auto center = pos;
			if (y % 2 == 0) {
				center.x += 0.75f;
			}
			auto p = Vec2T<i64>{ center.applied(floor) };
			if (p.x < texture.size().x && p.y < texture.size().y && texture.get(Vec2T<i64>{ p.x, texture.size().y - 1 - p.y }).r > 60) {
				auto hexagon = ConvexPolygon::regular(6, 0.5f);
				for (auto& v : hexagon.verts) {
					v *= Rotation{ TAU<float> / 12.0f };
					v += center;
				}
				Debug::drawLines(hexagon.verts, Vec3::WHITE);
			}
			pos.x += 1.5f;
		}
		pos.y += 0.5f * sin(TAU<float> / 6.0f);
		y++;
	}


	/*for (const auto& verts : vertices) {
		Debug::drawLines(verts);
	}*/
	for (const auto& verts : vertices) {
		auto v = verts;
		for (auto& a : v)
			a += Vec2{ 1.0f };
		const auto& r = triangulate(v);
		for (const auto& tri : r) {
			Debug::drawLines(tri.v, Vec3::WHITE);
		}
	}

	//Renderer::drawDynamicTexture(gridCenter, gridSize.y, texture);


	Renderer::update(camera);
}

auto MarchingSquares::marchingSquares() -> void {
	int whiteCount = 0;
	for (auto& p : texture) {
		const auto color = p.grayscaled();
		if (color.r > 127) {
			whiteCount++;
			p = PixelRgba{ 255 };
		} else {
			p = PixelRgba{ 0 };
		}
	}

	bool importantValue;
	if (whiteCount / static_cast<float>((texture.size().x * texture.size().y)) > 0.5f) {
		importantValue = true;
	} else {
		importantValue = false;
	}

	auto at = [&](i64 x, i64 y) -> bool {
		if (x < 0 || y < 0 || x >= texture.size().x || y >= texture.size().y) {
			return false;
		}
		return (texture.get(Vec2T{ x, texture.size().y - 1 - y }).r == 0) == importantValue;
	};

	auto value = [&](i64 x, i64 y) -> i32 {
		i32 value = at(x, y) << 0;
		value += at(x + 1, y) << 1;
		value += at(x, y + 1) << 2;
		value += at(x + 1, y + 1) << 3;
		return value;
	};
	vertices.clear();

	std::vector<bool> visited;
	visited.resize(texture.size().x * texture.size().y);
	auto setVisited = [&](i64 x, i64 y) -> void {
		if (x < 0 || y < 0 || x >= texture.size().x || y >= texture.size().y) {
			return;
		}
		visited[y * texture.size().x + x] = true;
	};

	auto getVisited = [&](i64 x, i64 y) -> bool {
		if (x < 0 || y < 0 || x >= texture.size().x || y >= texture.size().y) {
			return true;
		}
		return visited[y * texture.size().x + x];
	};

	Vec2T<i64> startPos{ 0, 0 };
	for (;;) {
		for (i64 y = startPos.y; y < texture.size().y; y++) {
			for (i64 x = startPos.x; x < texture.size().x; x++) {
				if (getVisited(x, y))
					continue;
				if (at(x, y) && !(at(x + 1, y) && at(x, y + 1) && at(x + 1, y + 1))) {
					startPos = Vec2T{ x, y };
					goto foundStaringPoint;
				}
			}
			if (y == texture.size().y - 1) {
				goto endend;
			}
		}
	foundStaringPoint:
		Vec2T<i64> current = startPos;

		Vec2T<i64> previousMove{ 0, 0 };
		std::vector<Vec2> verts;
		int i = 0;
		for (;;) {
			/*i++;
			if (i == 308)
				break;*/
			Vec2 pos{ current };
			Vec2T<i64> move;
			//Vec2 vertex;
			/*
			4 8
			1 2
			*/
			const auto v = value(current.x, current.y);
			std::optional<Vec2> vert;
			switch (v) {
				/*
				0 0 | 4 0 | 4 8
				1 0 | 1 0 | 1 0
				*/
			case 1:
			case 1 | 4:
			case 1 | 4 | 8:
				//if (v == 1 || v == (1 | 4 | 8)) {
				//	vert = Vec2{ 0.0f, -1.0f };
				//}
				move = { 0, -1 };
				break;

				/*
				0 0 | 0 0 | 4 0
				0 2 | 1 2 | 1 2
				*/
			case 2:
			case 1 | 2:
			case 1 | 2 | 4:
				//if (v == 2 || v == (1 | 2 | 4)) {
				//	vert = Vec2{ 1.0f, 0.0f };
				//}
				move = { 1, 0 };
				break;

				/*
				4 0 | 4 8 | 4 8
				0 0 | 0 0 | 0 2
				*/
			case 4:
			case 4 | 8:
			case 2 | 4 | 8:
				//if (v == 4 || v == (2 | 4 | 8)) {
				//	vert = Vec2{ -1.0f, 0.0f };
				//}
				move = { -1, 0 };
				break;

				/*
				0 8 | 0 8 | 0 8
				0 0 | 0 2 | 1 2
				*/
			case 8:
			case 2 | 8:
			case 1 | 2 | 8:
				/*if (v == 8 || v == (1 | 2 | 8)) {
					vert = Vec2{ 0.0f, 1.0f };
				}*/
				move = { 0, 1 };
				break;

				/*
				0 8
				1 0
				*/
			case 1 | 8:
				if (previousMove == Vec2T<i64>{ -1, 0 }) {
					move = { 0, -1 };
				} else {
					move = { 0, 1 };
				}
				break;

				/*
				4 0
				0 2
				*/
			case 2 | 4:
				if (previousMove == Vec2T<i64>{ 0, -1 }) {
					move = { -1, 0 };
				} else {
					move = { 1, 0 };
				}
				break;

				//	/*
				//	0 8
				//	1 0
				//	*/
				//case 1 | 8:
				//	if (previousMove == Vec2T<i64>{ -1, 0 }) {
				//		move = { 0, 1 };
				//	} else {
				//		move = { 0, -1 };
				//	}
				//	break;

				//	/*
				//	4 0
				//	0 2
				//	*/
				//case 2 | 4:
				//	if (previousMove == Vec2T<i64>{ 0, -1 }) {
				//		move = { -1, 0 };
				//	} else {
				//		move = { 1, 0 };
				//	}
				//	break;

			case 0:
				ASSERT_NOT_REACHED();
			default:
				move = { 0, 0 };
				//ASSERT_NOT_REACHED();
				goto end;
				break;
			}


			//if (at(current.x, current.y) && at(current.x + 1, current.y) && at(current.x, current.y + 1))
			//if (at(current.x, current.y) && at(current.x + 1, current.y)) {
			//	move = Vec2T<i64>{ 1, 0 };
			//	//vertex = Vec2{ pos.x, pos.y + 0.5f };
			//} else if (at(current.x, current.y) && at(current.x - 1, current.y)) {
			//	move = Vec2T<i64>{ -1, 0 };
			//	//vertex = Vec2{ pos.x + 1.0f, pos.y + 0.5f };
			//}
			if (previousMove == move) {
				verts.pop_back();
			}
			if (vert.has_value()) {
				verts.push_back(Vec2{ current } + *vert);
			} else {
				verts.push_back(Vec2{ current });
			}
			current += move;
			setVisited(current.x, current.y);
			setVisited(current.x + 1, current.y);
			setVisited(current.x, current.y + 1);
			setVisited(current.x + 1, current.y + 1);

			if (current == startPos)
				break;

			previousMove = move;
		}
	end:
		vertices.push_back(std::move(verts));
	}
	endend:

	for (auto& v : vertices) {
		if (v.size() >= 3) {
			std::vector<usize> toRem;
			for (int i = 0; i < vertices.size(); i++) {
				auto next = i + 1;
				if (next >= vertices.size()) {
					next = 0;
				}
				auto previous = i - 1;
				if (previous < 0) {
					previous = vertices.size() - 1;
				}

				if (dot(v[previous] - v[i], v[next] - v[i]) == 0.0f) {
					toRem.push_back(i);
				}
			}

			std::sort(toRem.begin(), toRem.end());
			for (int i = toRem.size() - 1; i > 0; i--) {
				vertices.erase(vertices.begin() + toRem[i]);
			}
		}
	}


}
