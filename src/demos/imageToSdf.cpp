#include <demos/imageToSdf.hpp>
#include <engine/frameAllocator.hpp>
#include <engine/debug.hpp>
#include <engine/window.hpp>
#include <engine/input.hpp>

#define PATH "C:/Users/user/Downloads/ffmpeg-5.1.2-essentials_build/ffmpeg-5.1.2-essentials_build/bin/"

ImageToSdfDemo::ImageToSdfDemo()
 : texture {
	[]() {
		const auto image = ImageRgba::fromFile(PATH "out1.png");
		DynamicTexture texture{ image->size() * 2 };
		if (image.has_value()) {
			//texture.copyAndResize(*image);
		}
		return texture;
	}() } {
	Window::setSize(Vec2{ 480, 360 } * 3.0f);
}

#include <functional>
#include <queue>
#include <utility>
using namespace std;// We use 64-bit integers to avoid some annoying integer math
// overflow corner cases.
using Metric = function<float(int64_t, int64_t)>; 
float euclidian(int64_t dx, int64_t dy) {
	return sqrt(dx * dx + dy * dy);
}
void sdf_partial(
	const vector<bool>& in_filled, int width,
	vector<pair<int, int>>* in_half_vector, Metric metric, bool negate) {
	//assert(width != 0);
	const auto height = in_filled.size() / width;
	//assert(height != 0);
	auto valid_pixel = [&](int x, int y) {
		return (x >= 0) && (x < width) && (y >= 0) && (y < height); };
	auto coord = [&](int x, int y) { return x + width * y; };
	auto filled = [&](int x, int y) -> bool {
		if (valid_pixel(x, y))
			return in_filled[coord(x, y)] ^ negate;
		return false ^ negate;
	};
	// Allows us to write loops over a neighborhood of a cell.
	auto do_neighbors = [&](int x, int y, function<void(int, int)> f) {
		for (int dy = -1; dy <= 1; dy++)
			for (int dx = -1; dx <= 1; dx++)
				if (valid_pixel(x + dx, y + dy))
					f(x + dx, y + dy);
	};  auto& half_vector = *in_half_vector;
	vector<bool> closed(in_filled.size());  struct QueueElement {
		int x, y, dx, dy;
		float dist;
	};
	struct QueueCompare {
		bool operator() (QueueElement& a, QueueElement& b) {
			return a.dist > b.dist;
		}
	};
	priority_queue<
		QueueElement, vector<QueueElement>, QueueCompare> pq;
	auto add_to_queue = [&](int x, int y, int dx, int dy) {
		pq.push({ x, y, dx, dy, metric(dx, dy) });
	};  // A. Seed phase: Find all filled (black) pixels that border an
   // empty pixel. Add half distances to every surrounding unfilled
   // (white) pixel.
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			if (filled(x, y)) {
				do_neighbors(x, y, [&](int x2, int y2) {
					if (!filled(x2, y2))
						add_to_queue(x2, y2, x2 - x, y2 - y);
					});
			}
		}
	}  // B. Propagation phase: Add surrounding pixels to queue and
	// discard the ones that are already closed.
	while (!pq.empty()) {
		auto current = pq.top();
		pq.pop();
		// If it's already been closed then the shortest vector has
		// already been found.
		if (closed[coord(current.x, current.y)])
			continue;
		// Close this one and store the half vector.
		closed[coord(current.x, current.y)] = true;
		half_vector[coord(current.x, current.y)] = {
		  current.dx, current.dy };
		// Add all open neighbors to the queue.
		do_neighbors(current.x, current.y, [&](int x2, int y2) {
			if (!filled(x2, y2) && !closed[coord(x2, y2)]) {
				int dx = 2 * (x2 - current.x);
				int dy = 2 * (y2 - current.y);
				auto [ddx, ddy] = half_vector[coord(current.x, current.y)];
				dx += ddx;
				dy += ddy;
				add_to_queue(x2, y2, dx, dy);
			}
			});
	}
}

vector<float> sdf(
	const vector<bool>& in_filled, int width) {
	const auto height = in_filled.size() / width;
	// Initialize vectors represented as half values.
	vector<pair<int, int>> half_vector(
		in_filled.size(), { 2 * width + 1, 2 * height + 1 });
	sdf_partial(in_filled, width, &half_vector, euclidian, false);
	sdf_partial(in_filled, width, &half_vector, euclidian, true);
	vector<float> out(in_filled.size());
	for (size_t i = 0; i < half_vector.size(); i++) {
		auto [dx, dy] = half_vector[i];
		out[i] = euclidian(dx, dy) / 2;
		if (in_filled[i])
			out[i] = -out[i];
	}
	return out;
}

auto ImageToSdfDemo::update() -> void {
	Camera camera;
	const auto gridSize = Vec2{ texture.size() };
	const auto gridCenter = gridSize / 2.0f;
	camera.pos = gridCenter;
	camera.setWidth(gridSize.x);
	if (gridSize.y > camera.height()) {
		camera.setWidth(gridSize.y * camera.aspectRatio);
	}

	auto screenshot = Renderer::screenshot();
	screenshot.saveToFile(frameAllocator.format("C:/Users/user/Downloads/ffmpeg-5.1.2-essentials_build/ffmpeg-5.1.2-essentials_build/bad2/out%d.png", frame).data());

	auto image = ImageRgba::fromFile(frameAllocator.format(PATH "out%d.png", frame).data());
	if (image.has_value()) {
		texture.copyAndResize(*image);
		//memcpy(texture.data(), image->data(), texture.dataSizeBytes());
	}
	/*ASSERT(texture.size().x == 480);
	ASSERT(texture.size().y == 360);*/
	/*for (int i = 0; i < texture.pixelCount(); i++) {
		bitset.set(i, val);
	}*/
	std::vector<bool> bitset;
	bitset.resize(texture.size().x * texture.size().y);
	for (int y = 0; y < texture.size().y; y++) {
		for (int x = 0; x < texture.size().x; x++) {
			const auto val = texture.get(Vec2T<i64>(x, y)).r > 227;
			/*bitset[y * texture.size().x + x] = val;*/
			bitset[y * texture.size().x + x] = val;
		}
	}
	auto result = sdf(bitset, texture.size().x);

	for (int y = 0; y < texture.size().y; y++) {
		for (int x = 0; x < texture.size().x; x++) {
			auto value = result[y * texture.size().x + x];
			////auto v = (float % 5) * 51;
			//float v = abs(value);
			//float b = 10.0f;
			//PixelRgba p{ 0 };
			//if (fmod(v, b) < b / 2.0f) {
			//	p = PixelRgba(fmod(v, b) / b * 255.0f);
			//}
			//if (v < 1.0f) {
			//	p = PixelRgba{ 127 };
			//}
			///*if (value < 0.0f) {
			//	v /= 2;
			//}*/
			*reinterpret_cast<float*>(&texture.data()[y * texture.size().x + x]) = value;
			//texture.set(Vec2T<i64>(x, y), PixelRgba(abs(value)));
			/*const auto p = texture.get(Vec2T<i64>(x, y));
			texture.set(Vec2T<i64>(x, y), PixelRgba(p.r, p.g, p.b, (value < 0.0f) ? 0 : 1 ));*/
		}
	}

	/*auto at = [this](int x, int y) {
		
		return texture.get(Vec2T<i64>(x, y)).r > 127;
	};*/
	//frame++;
	//ImageRgba result{ texture.size() };
	//for (int y = 0; y < texture.size().y; y++) {
	//	for (int x = 0; x < texture.size().x; x++) {
	//		const auto goal = !bitset[y * texture.size().x + x];
	//		int radius = 1;
	//		for (;;) {
	//			int minY = std::clamp(y - radius, 0, static_cast<int>(texture.size().y) - 1);
	//			int maxY = std::clamp(y + radius, 0, static_cast<int>(texture.size().y) - 1);
	//			int minX = std::clamp(x - radius, 0, static_cast<int>(texture.size().x) - 1);
	//			int maxX = std::clamp(x + radius, 0, static_cast<int>(texture.size().x) - 1);
	//			for (int ry = minY; ry <= maxY; ry++) {
	//				for (int rx = minX; rx <= maxX; rx++) {
	//					/*const auto distSqr = rx * rx + ry * ry;
	//					if (distSqr > radius) {
	//						continue;
	//					}*/
	//					if (bitset[ry * texture.size().x + rx] == goal) {
	//					//if (bitset.test(ry * texture.size().y + rx) == goal) {
	//						auto v = (radius % 5) * 51;
	//						if (goal == true) {
	//							v /= 2;
	//						}
	//						result.set(Vec2T<i64>(x, y), PixelRgba(v));
	//						goto nextLoop;
	//					}
	//				}
	//			}

	//			if (radius > 480) {
	//				break;
	//			}

	//			radius++;
	//		}
	//	nextLoop:
	//		int a = 5;
	//	}
	//}

	//texture.copyAndResize(result);
	//memcpy(texture.data(), result.data(), texture.dataSizeBytes());

	//frame++;
	//Debug::drawCircle(Vec2{ 0.0f }, 0.1f);

	if (Input::isKeyDown(Keycode::P)) {
		paused = !paused;
	}

	Renderer::drawDynamicTexture(gridCenter, gridSize.y, texture, true);
	if (!paused)
		frame++;
	Renderer::update(camera);
}
