#include <demos/marchingSquares.hpp>
#include <engine/debug.hpp>

MarchingSquares::MarchingSquares()
	: texture{ []() {
		const auto image = ImageRgba::fromFile("assets/bad.png");
		DynamicTexture texture{ image->size() / 10 };
		if (image.has_value()) {
			texture.copyAndResize(*image);
		} else {
			ASSERT_NOT_REACHED();
		}
		return texture;
	}() } {
	for (auto& p : texture) {
		const auto color = p.grayscaled();
		p = (color.r > 127) ? PixelRgba{ 255 } : PixelRgba{ 0 };
	}

	auto at = [&](i64 x, i64 y) -> bool {
		if (x < 0 || y < 0 || x >= texture.size().x || y >= texture.size().y) {
			return false;
		}
		return texture.get(Vec2T{ x, texture.size().y - 1 - y }).r == 0;
	};

	auto value = [&](i64 x, i64 y) -> i32 {
		i32 value = at(x, y) << 0;
		value += at(x + 1, y) << 1;
		value += at(x, y + 1) << 2;
		value += at(x + 1, y + 1) << 3;
		return value;
	};

	Vec2T<i64> current;
	for (i64 y = 1; y < texture.size().y - 1; y ++) {
		for (i64 x = 1; x < texture.size().x - 1; x ++) {
			if (at(x, y) && !(at(x + 1, y) && at(x, y + 1) && at(x + 1, y + 1))) {
				current = Vec2T{ x, y };
				goto foundStaringPoint;
			}
		}
	}
	foundStaringPoint:

	const auto startPos = current;
	Vec2T<i64> previousMove{ 0, 0 };
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
			if (v == 1) {
				vert = pos - Vec2{ 0.0f, 1.0f };
			}
			move = { 0, -1 };
			break;

			/*
			0 0 | 0 0 | 4 0
			0 2 | 1 2 | 1 2
			*/
		case 2:
		case 1 | 2:
		case 1 | 2 | 4:
			move = { 1, 0 };
			break;

			/*
			4 0 | 4 8 | 4 8
			0 0 | 0 0 | 0 2
			*/
		case 4:
		case 4 | 8:
		case 2 | 4 | 8:
			move = { -1, 0 };
			break;

			/*
			0 8 | 0 8 | 0 8
			0 0 | 0 2 | 1 2
			*/
		case 8:
		case 2 | 8:
		case 1 | 2 | 8:
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
		current += move;
		if (previousMove == move) {
			vertices.pop_back();
		}
		vertices.push_back(Vec2{ current });

		if (current == startPos)
			break;

		previousMove = move;
	}
end:
	int x = 5;

	//// Just do something like greedy meshing.
	//const auto step = 8;
	//for (i64 y = step; y < texture.size().y - step; y += step) {
	//	for (i64 x = step; x < texture.size().x - step; x += step) {
	//		auto at = [&](i64 x, i64 y) -> bool {
	//			return texture.get(Vec2T{ x, texture.size().y - 1 - y }).r == 0;
	//		};

	//		if (at(x, y) && at(x + step, y) && at(x, y + step) && at(x + step, y + step)) {
	//			/*triangles.push_back(Triangle{ Vec2(x, y), Vec2(x + step, y), Vec2(x + step, y + step) });
	//			triangles.push_back(Triangle{ Vec2(x + step, y + step), Vec2(x, y + step), Vec2(x, y) });*/
	//		} else if (at(x, y) && at(x + step, y + step) && at(x, y + step)) {
	//			/*triangles.push_back(Triangle{ Vec2(x, y), Vec2(x + step / 2.0f, y), Vec2(x, y + step) });
	//			triangles.push_back(Triangle{ Vec2(x + step / 2.0f, y), Vec2(x + step, y + step / 2.0f), Vec2(x, y + step) });
	//			triangles.push_back(Triangle{ Vec2(x + step, y + step / 2.0f), Vec2(x + step, y + step), Vec2(x, y + step) });*/
	//		} else if (at(x, y) && at(x + step, y) && at(x, y + step)) {
	//			/*triangles.push_back(Triangle{ Vec2(x, y), Vec2(x + step, y), Vec2(x + step, y + step / 2.0f) });
	//			triangles.push_back(Triangle{ Vec2(x, y), Vec2(x + step, y + step / 2.0f), Vec2(x + step / 2.0f, y + step) });
	//			triangles.push_back(Triangle{ Vec2(x, y), Vec2(x + step / 2.0f, y + step), Vec2(x, y + step) });*/
	//		} else if (at(x, y) && at(x + step, y + step) && at(x + step, y)) {
	//			/*triangles.push_back(Triangle{ Vec2(x + step, y + step), Vec2(x + step / 2.0f, y + step), Vec2(x + step, y) });
	//			triangles.push_back(Triangle{ Vec2(x + step / 2.0f, y + step), Vec2(x, y + step / 2.0f), Vec2(x + step, y) });
	//			triangles.push_back(Triangle{ Vec2(x, y + step / 2.0f), Vec2(x, y), Vec2(x + step, y) });*/
	//		} else if (at(x, y + step) && at(x + step, y + step) && at(x + step, y)) {
	//			/*triangles.push_back(Triangle{ Vec2(x + step, y + step), Vec2(x, y + step), Vec2(x, y + step / 2.0f) });
	//			triangles.push_back(Triangle{ Vec2(x + step, y + step), Vec2(x, y + step / 2.0f), Vec2(x + step / 2.0f, y) });
	//			triangles.push_back(Triangle{ Vec2(x + step, y + step), Vec2(x + step / 2.0f, y), Vec2(x + step, y) });*/
	//		} /*else if (at(x, y) && at(x + 1, y)) {

	//		} else if (at(x, y) && at(x, y + 1)) {

	//		} else if (at(x, y) && at(x + 1, y + 1)) {

	//		} else if (at(x + 1, y)) {

	//		} else if (at(x + 1, y) && at(x, y + 1)) {

	//		} else if (at(x + 1, y) && at(x + 1, y + 1)) {

	//		} else if (at(x, y + 1) && at(x + 1, y)) {

	//		} else if (at(x, y + 1)) {

	//		} else if (at(x, y + 1) && at(x + 1, y + 1)) {

	//		} else if (at(x + 1, y + 1) && at(x + 1, y)) {

	//		} else if (at(x + 1, y + 1) && at(x, y + 1)) {

	//		}*/ 
	//		//else {
	//		//	if (at(x, y)) {
	//		//		triangles.push_back(Triangle{ Vec2(x, y), Vec2(x + step / 2.0f, y), Vec2(x, y + step / 2.0f) });
	//		//	} 
	//		//	if (at(x + step, y)) {
	//		//		triangles.push_back(Triangle{ Vec2(x + step, y), Vec2(x + step, y + step / 2.0f), Vec2(x + step / 2.0f, y) });
	//		//	} 
	//		//	if (at(x, y + step)) {
	//		//		triangles.push_back(Triangle{ Vec2(x + step, y), Vec2(x + step, y + step / 2.0f), Vec2(x + step / 2.0f, y) });
	//		//	} 
	//		//	if (at(x + step, y + step)) {
	//		//		triangles.push_back(Triangle{ Vec2(x + step, y + step), Vec2(x + step / 2.0f, y + step), Vec2(x + step, y + step / 2.0f) });
	//		//	}
	//		//}

	//			
	//			/*(at(x, y)) {
	//			triangles.push_back(Triangle{ Vec2(x, y), Vec2(x + step / 2.0f, y), Vec2(x, y + 2.0f) });
	//		}*/
	//		
	//		/*else if (at(x, y)) {
	//			triangles.push_back(Triangle{ Vec2(x, y), Vec2(x + step / 2.0f, y), Vec2(x, y + step / 2.0f) });
	//		}*/
	//	}
	//}

}

#include <engine/frameAllocator.hpp>

auto MarchingSquares::update() -> void {
	Camera camera;
	const auto gridSize = Vec2{ texture.size() };
	const auto gridCenter = gridSize / 2.0f;
	camera.pos = gridCenter;
	camera.setWidth(gridSize.x);
	if (gridSize.y > camera.height()) {
		camera.setWidth(gridSize.y * camera.aspectRatio);
	}

	//Debug::drawLines(vertices);
	int i = 0;
	//for (const auto& p : vertices) {
	//	Debug::drawPoint(p + Vec2{ 1.0f }, Vec3::RED);
	//	//Debug::drawText(p + Vec2{ 1.0f }, frameAllocator.format("%d", values[i]).data(), Vec3::WHITE, 0.6f);
	//	i++;
	//}
	//Debug::drawPoint(vertices[0] + Vec2{ 1.0f }, Vec3::GREEN);
	auto v = vertices;
	for (auto& a : v)
		a += Vec2{ 1.0f };
	Debug::drawLines(v, Vec3::RED);
	SimplePolygonTriangulator triangulate;
	auto r = triangulate(v);

	/*for (const auto& tri : r) {
		Debug::drawLines(tri.v, Vec3::RED);
	}*/
	Renderer::drawDynamicTexture(gridCenter, gridSize.y, texture);


	Renderer::update(camera);
}
