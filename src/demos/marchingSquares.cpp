#include <demos/marchingSquares.hpp>
#include <engine/debug.hpp>
#include <engine/window.hpp>
#include <imgui/imgui.h>

MarchingSquares::MarchingSquares()
	: texture{ []() {
		const auto image = ImageRgba::fromFile("C:/Users/user/Desktop/bad.png");
		DynamicTexture texture{ image->size() / 4 };
		if (image.has_value()) {
			texture.copyAndResize(*image);
		}
		return texture;
	}() } {

	/*vertices.clear();
	marchingSquares();*/
}

int pnpoly(const std::vector<Vec2>& vert, Vec2 test)
{
	int i, j, c = 0;
	for (i = 0, j = vert.size() - 1; i < vert.size(); j = i++) {
		if (((vert[i].y > test.y) != (vert[j].y > test.y)) &&
			(test.x < (vert[j].x - vert[i].x) * (test.y - vert[i].y) / (vert[j].y - vert[i].y) + vert[i].x))
			c = !c;
	}
	return c;
}

#include <engine/frameAllocator.hpp>
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

	//Renderer::drawDynamicTexture(gridCenter, gridSize.y, texture);

	if (Input::isKeyDown(Keycode::A))
		paused = !paused;

	auto image = ImageRgba::fromFile(frameAllocator.format("C:/Users/user/Downloads/ffmpeg-5.1.2-essentials_build/ffmpeg-5.1.2-essentials_build/bin/out%d.png", i / 2).data());
	if (paused)
		i++;

	if (Input::isKeyDown(Keycode::LEFT)) {
		i--;
	}

	if (Input::isKeyDown(Keycode::RIGHT)) {
		i++;
	}
	if (image.has_value()) {
		visited.clear();
		vertices.clear();
		texture.copyAndResize(*image);
		marchingSquares();
	}

	for (int i = vertices.size() - 1; i >= 0; i--) {
		if (vertices[i].size() < 3) {
			vertices.erase(vertices.begin() + i);
		}
	}

	for (auto& v : vertices) {
		for (int i = v.size() - 1; i >= 0; i--) {
			for (int j = v.size() - 1; j >= 0; j--) {
				if (i == j)
					continue;

				if (distance(v[i], v[j]) < 0.05f) {
					v.erase(v.begin() + j);
				}
			}
		}
	}

	srand(0);
	for (auto& v : vertices) {
		for (auto& a : v) {
			a += Vec2(rand() / float(RAND_MAX), rand() / float(RAND_MAX));
		}
	}

	auto isClockWise = [](const std::vector<Vec2>& lines) -> bool {
		if (lines.size() <= 2) {
			return false;
		}
		auto twiceTheSignedArea = 0.0f;
		auto f = [](Vec2 a, Vec2 b) {
			return (b.x - a.x) * (b.y + a.y);
		};
		twiceTheSignedArea += f(lines[0], lines[1]);
		for (usize i = 1; i < lines.size() - 1; i++) {
			twiceTheSignedArea += f(lines[i], lines[i + 1]);
		}
		twiceTheSignedArea += f(lines[lines.size() - 1], lines[0]);
		return twiceTheSignedArea > 0.0f;
	};

	/*for (int i = 0; i < vertices.size(); i++) {*/
	for (int i = vertices.size() - 1; i >= 0; i--) {
		if (isClockWise(vertices[i]))
			continue;

		int outsideShape;
		for (int j = 0; j < vertices.size(); j++) {
			if (i == j)
				continue;

			if (!isClockWise(vertices[j]))
				continue;

			for (int a = 0; a < vertices[i].size(); a++) {
				if (!pnpoly(vertices[j], vertices[i][a])) {
					goto next;
				}
			}
			outsideShape = j;
			goto found;

			next:
			int x = 5;
		}
		goto end;
	found:
		//Debug::drawLine(vertices[outsideShape][0], vertices[i][0], Vec3::BLUE);
		auto& inside = vertices[i];
		auto& outside = vertices[outsideShape];

		auto intersectsShape = [](std::vector<Vec2> shape, const LineSegment& line) -> bool {
			for (int i = 0; i < shape.size(); i++) {
				auto next = i + 1;
				if (next >= shape.size()) {
					next = 0;
				}
				LineSegment l{ shape[i], shape[next] };
				auto endpoints = line.getCorners();
				auto lendpoints = l.getCorners();
				if (auto p = line.intersection(l); p.has_value()) {
					/*if (distance(*p, endpoints[0]) > 0.15f && distance(*p, endpoints[1]) > 0.15f
						&& distance(*p, lendpoints[0]) > 0.15f && distance(*p, lendpoints[1]) > 0.15f) {
						return true;
					}*/
					/*if (distance(*p, endpoints[0]) > 0.15f && distance(*p, endpoints[1]) > 0.15f) {
						return true;
					}*/
					return true;
				}
			}
			return false;
		};

		int foundInside, foundOutside;
		bool foundLine = false;
		for (int i = 0; i < inside.size(); i++) {
			for (int j = 0; j < outside.size(); j++) {
				auto dir = (outside[j] - inside[i]).normalized();
				LineSegment segment{ inside[i], outside[j]};
				/*LineSegment segment{ inside[i] - dir * 0.05f, outside[j] + dir * 0.05f };*/
				if (!intersectsShape(inside, segment) && !intersectsShape(outside, segment)) {
					foundInside = i;
					foundOutside = j;
					foundLine = true;
					goto foundLineLabel;
				}
				//Debug::drawLine(outside[j], inside[i], Vec3::RED);
			}
		}
		goto end;
		foundLineLabel:
		if (foundLine) {
			Debug::drawLine(inside[foundInside], outside[foundOutside], Vec3::BLUE);
			//ImGui::Checkbox("asd", &asd);
			asd = true;
			if (asd) {
				const auto cutPoint = inside[foundInside];
				const auto cutPointOutside = outside[foundOutside];
				std::rotate(inside.begin(), inside.begin() + foundInside, inside.end());
				//std::reverse(inside.begin(), inside.end());
				/*auto next = foundOutside + 1;
				if (next >= outside.size()) {
					next = 0;
				}*/
				auto k = foundOutside + 1;
				if (k >= outside.size()) {
					k = 0;
				}
				outside.insert(outside.begin() + k, inside.begin(), inside.end());
				auto h = foundOutside + inside.size();
				if (h >= outside.size()) {
					h = 0;
				}
				//Debug::drawPoint(outside[h]);
				outside.insert(outside.begin() + h + 1, cutPoint);
				outside.insert(outside.begin() + h + 2, cutPointOutside);
				/*outside.insert(outside.begin() + foundOutside + inside.size() + 2, outside[foundOutside]);*/
				/*outside.insert(outside.begin() + foundOutside + inside.size() + 2, inside.back());*/
				/*outside.insert(outside.begin() + foundOutside + inside.size() + 1, cutPoint);
				outside.insert(outside.begin() + foundOutside + inside.size() + 2, cutPointOutside);*/
				//Debug::drawPoint(cutPoint);
				vertices.erase(vertices.begin() + i);
			}
		}
	}
	end:
	int x = 5;

	static int index = 0;
		//ImGui::SliderInt("index", &index, 0, lines.size() - 1);
		if (Input::isKeyDown(Keycode::A)) {
			index -= 1;
		}
		if (Input::isKeyDown(Keycode::D)) {
			index += 1;
		}

	static bool testa = false;
	ImGui::Checkbox("testa", &testa);
	for (const auto& lines : vertices) {
		//ImGui::TextWrapped("%zu", lines.size());
		

		Vec3 color = Vec3::RED;
		if (isClockWise(lines)) {
			color = Vec3::GREEN;
		}

		static int taa = 0;
		ImGui::SliderInt("test", &taa, 0, lines.size() - 1);
		auto vv = lines;
		std::rotate(vv.begin(), vv.begin() + taa, vv.end());

		if (testa) {
			Debug::drawLines(lines, color);
			Debug::drawPoint(lines[index]);
		} else {
			/*const auto& triangles = triangulate(lines);*/
			//static int tb = 0;
			const auto& triangles = triangulate(vv);
			/*ImGui::SliderInt("tb", &tb, 0, triangles.size() - 1);
			for (int i = 0; i < tb; i++) {
				Debug::drawLines(triangles[i].v, Vec3::WHITE);
			}*/
			for (const auto& tri : triangles) {
				Debug::drawLines(tri.v, Vec3::WHITE);
			}
		}

		
		//Debug::drawLines(lines, Vec3::WHITE);

		//if (twiceTheSignedArea > 0.0f) {
		//	const auto& triangles = triangulate(lines);
		//	for (const auto& tri : triangles) {
		//		Debug::drawLines(tri.v, Vec3::WHITE);
		//	}
		//} else {
		//	Debug::drawLines(lines, Vec3::WHITE);
		//}
		//Debug::drawLines(lines, Vec3::WHITE);
	}

	//for (const auto& start : starts) {
	//	Debug::drawPoint(start + Vec2{ 1.0f }, Vec3::RED);
	//}

	/*for (i64 y = 0; y < texture.size().y; y++) {
		for (i64 x = 0; x < texture.size().x; x++) {
			if (visited[y * texture.size().x + y]) {
				Debug::drawCircle(Vec2{ Vec2T{ x, y } } + Vec2{ 0.5f }, 0.3f, Vec3::RED);
			}
		}
	}*/

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
	//if (whiteCount / static_cast<float>((texture.size().x * texture.size().y)) > 0.5f) {
	//	importantValue = true;
	//} else {
	//	importantValue = false;
	//}
	importantValue = false;

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
	visited.clear();
	visited.resize(texture.size().x * texture.size().y, false);
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
	starts.clear();
	Vec2T<i64> startPos{ 0, 0 };
	for (;;) {
		for (i64 y = startPos.y; y < texture.size().y; y++) {
			for (i64 x = startPos.x; x < texture.size().x; x++) {
				if (getVisited(x, y))
					continue;
				setVisited(x, y);
				if (at(x, y) && !(at(x + 1, y) && at(x, y + 1) && at(x + 1, y + 1))) {
					startPos = Vec2T{ x, y };
					starts.push_back(Vec2{ startPos });
					goto foundStaringPoint;
				}
			}
			if (y == texture.size().y - 1) {
				goto endend;
			}
			startPos.x = 0;
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
				/*if (previousMove == Vec2T<i64>{ -1, 0 }) {
					move = { 0, -1 };
				} else {
					move = { 0, 1 };
				}*/
				if (previousMove == Vec2T<i64>{ -1, 0 }) {
					move = { 0, 1 };
				} else {
					move = { 0, -1 };
				}
				/*setVisited(current.x + 1, current.y);
				setVisited(current.x, current.y + 1);*/
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
				/*setVisited(current.x, current.y);
				setVisited(current.x + 1, current.y + 1);*/
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

			
			
			/*verts.push_back(Vec2{ current } + Vec2{ 1.0f });
			if (previousMove == move) {
				verts.pop_back();
			}*/
			if (previousMove == move) {
				verts.pop_back();
			}
			verts.push_back(Vec2{ current } + Vec2{ 1.0f });
			
			current += move;
			setVisited(current.x, current.y);
			/*setVisited(current.x + 1, current.y);
			setVisited(current.x, current.y + 1);
			setVisited(current.x + 1, current.y + 1);*/

			if (current == startPos)
				break;

			previousMove = move;
		}
	end:
		vertices.push_back(std::move(verts));
	}
endend:
	int x = 5;
	for (int i = 0; i < 30; i++) {
		for (auto& v : vertices) {
			if (v.size() >= 3) {
				std::vector<usize> toRem;
				for (int i = 0; i < v.size(); i++) {
					auto next = i + 1;
					if (next >= v.size()) {
						next = 0;
					}
					auto previous = i - 1;
					if (previous < 0) {
						previous = v.size() - 1;
					}
					/*if (dot(v[previous] - v[i], v[next] - v[i]) == 0.0f && det(v[previous] - v[i], v[next] - v[i]) < 0) {
						toRem.push_back(i);
					}*/
					if (dot(v[previous] - v[i], v[next] - v[i]) == 0.0f) {
						toRem.push_back(i);
					}
				}
				std::sort(toRem.begin(), toRem.end());
				for (int i = toRem.size() - 1; i > 0; i--) {
					v.erase(v.begin() + toRem[i]);
				}
			}
		}
	}
	//static bool t = false;
	//ImGui::Begin("tt");
	//ImGui::Checkbox("t", &t);
	//ImGui::End();
	//t = false;
	//if (t) {
	//	for (int i = 0; i < 2; i++) {
	//		for (auto& v : vertices) {
	//			if (v.size() >= 3) {
	//				std::vector<usize> toRem;
	//				for (int i = 0; i < v.size(); i++) {
	//					auto next = i + 1;
	//					if (next >= v.size()) {
	//						next = 0;
	//					}
	//					auto previous = i - 1;
	//					if (previous < 0) {
	//						previous = v.size() - 1;
	//					}
	//					/*if (dot(v[previous] - v[i], v[next] - v[i]) == 0.0f && det(v[previous] - v[i], v[next] - v[i]) < 0) {
	//						toRem.push_back(i);
	//					}*/
	//					auto x = abs(dot((v[previous] - v[i]).normalized(), (v[i] - v[next]).normalized()));
	//					//Debug::drawText(v[i], frameAllocator.format("%g", x).data(), Vec3::GREEN, 3.0f);
	//					/*if (abs(dot(v[previous] - v[i], v[i] - v[next])) < 0.4f) {
	//						toRem.push_back(i);
	//					}*/
	//					if (abs(x - 1.0f) < 0.2f) {
	//						toRem.push_back(i);
	//					}
	//				}
	//				std::sort(toRem.begin(), toRem.end());
	//				for (int i = toRem.size() - 1; i > 0; i--) {
	//					v.erase(v.begin() + toRem[i]);
	//				}
	//			}
	//		}
	//	}
	//}
	//int x = 5;
}
