#include <demos/maxwell.hpp>
#include <engine/renderer.hpp>
#include <engine/input.hpp>
#include <utils/dbg.hpp>
#include <math/polygon.hpp>

struct Charge {
	Vec2 pos;
	float strength;
};

//std::vector<Charge> charges{ Charge{ Vec2{ 0.0f }, 1.0f } };
std::vector<Charge> charges;
std::vector<Vec2> electricField;
std::vector<float> electricFieldUpZ;
std::vector<float> electricFieldDownZ;
std::vector<float> electricCharge;

std::vector<Vec2> selectedArea;

#include <utils/array2d.hpp>
#include <utils/print.hpp>


#include <utils/stringStream.hpp>

static thread_local StringStream stream;

template<typename ...Args>
auto putText(const char* message, const Args&... args) -> void {
	stream.string().clear();
	printToStream(stream, message, args...);
	ImGui::TextWrapped("%s", stream.string().c_str());
}


auto Maxwell::update() -> void {
	putText("% test %", 5.0f, Vec2{ 1.0f });
	//std::ostream test(&buf, true);
	//std::streambuf();
	Camera camera;
	camera.pos += camera.aabb().size() / 2.0f;
	//Debug::drawCircle(Vec2{ 0.0f });

	if (Input::isMouseButtonDown(MouseButton::LEFT)) {
		charges.push_back(Charge{ camera.cursorPos(), 1.0f });
	} else if (Input::isMouseButtonDown(MouseButton::RIGHT)) {
		charges.push_back(Charge{ camera.cursorPos(), -1.0f });
	}

	if (Input::isMouseButtonDown(MouseButton::MIDDLE)) {
		selectedArea.push_back(camera.cursorPos());
	}
	Debug::drawLines(selectedArea);

	for (const auto& charge : charges) {
		Debug::drawCircle(charge.pos, 0.02f, charge.strength < 0.0f ? Vec3::BLUE : Vec3::RED);
		Debug::drawText(charge.pos, charge.strength, Vec3::WHITE, 0.02f);
	}

	const auto aabb = camera.aabb();
	const auto step = aabb.size().x / 100.0f;

	auto convert = [&](Vec2 v) -> Vec2T<i64> {
		return Vec2T<i64>{ (v / step).applied(floor) };
	};

	const auto electricFieldSize = convert(aabb.size());
	const auto cellCount = electricFieldSize.x * electricFieldSize.y;
	if (electricField.size() != cellCount) {
		electricField.resize(cellCount, Vec2{ 0.0f });
		electricFieldUpZ.resize(cellCount, 0.0f);
		electricFieldDownZ.resize(cellCount, 0.0f);
		electricCharge.resize(cellCount, 0.0f);
	}
	auto at = [&](const Vec2T<i64>& pos) -> Vec2& {
		const auto p = pos.clamped(Vec2T<i64>{ 0 }, electricFieldSize - Vec2T<i64>{ 1 });
		return electricField[electricFieldSize.x * p.y + p.x];
	};

	auto atUp = [&](const Vec2T<i64>& pos) -> float& {
		const auto p = pos.clamped(Vec2T<i64>{ 0 }, electricFieldSize - Vec2T<i64>{ 1 });
		return electricFieldUpZ[electricFieldSize.x * p.y + p.x];
	};

	auto atDown = [&](const Vec2T<i64>& pos) -> float& {
		const auto p = pos.clamped(Vec2T<i64>{ 0 }, electricFieldSize - Vec2T<i64>{ 1 });
		return electricFieldDownZ[electricFieldSize.x * p.y + p.x];
	};

	auto chargeAt = [&](const Vec2T<i64>& pos) -> float& {
		const auto p = pos.clamped(Vec2T<i64>{ 0 }, electricFieldSize - Vec2T<i64>{ 1 });
		return electricCharge[electricFieldSize.x * p.y + p.x];
	};


	std::fill(electricCharge.begin(), electricCharge.end(), 0);
	for (i64 y = 0; y < electricFieldSize.y; y++) {
		for (i64 x = 0; x < electricFieldSize.x; x++) {
			const auto aabb = Aabb::fromCorners(Vec2(x, y) * step, Vec2(x + 1, y + 1) * step);
			for (const auto& charge : charges) {
				if (aabb.contains(charge.pos)) {
					chargeAt({ x, y }) += charge.strength;
				}
			}
		}
	}


	//for (float y = aabb.min.y; y <= aabb.max.y + step; y += step) {
	//	for (float x = aabb.min.x; x <= aabb.max.x + step; x += step) {
	//		const Vec2 pos{ x, y };
	//		Vec2 electricity{ 0.0f };
	//		for (const auto& charge : charges) {
	//			auto direction = (pos - charge.pos);
	//			const auto distance = direction.length();
	//			direction /= distance;
	//			electricity += charge.strength * direction / pow(distance, 2.0f);
	//		}
	//		at(convert(pos)) = electricity;
	//	}
	//}

	static auto lastFrame = charges.size();
	/*if (charges.size() != lastFrame)*/ {
		lastFrame = charges.size();

		for (i64 y = 0; y < electricFieldSize.y; y++) {
			for (i64 x = 0; x < electricFieldSize.x; x++) {
				const auto pos = Vec2{ Vec2T<i64>{ x, y } } * step;
				Vec2 electricity{ 0.0f };
				for (const auto& charge : charges) {
					auto direction = (pos - charge.pos);
					const auto distance = direction.length();
					electricity += charge.strength * direction.normalized() / pow(distance, 2.0f);
					/*electricity.x += direction.x / pow(distance, 3.0f);
					electricity.y += direction.y / pow(distance, 3.0f);*/
				}
				at({ x, y }) = electricity;
			}
		}

		for (i64 y = 0; y < electricFieldSize.y; y++) {
			for (i64 x = 0; x < electricFieldSize.x; x++) {
				const auto pos = Vec3(x, y, -1.0f) * step;
				Vec3 electricity{ 0.0f };
				for (const auto& charge : charges) {
					auto direction = (pos - Vec3(charge.pos.x, charge.pos.y, 0.0f));
					const auto distance = direction.length();
					electricity += direction.normalized() * charge.strength / pow(distance, 2.0f);
				}
				atDown({ x, y }) = electricity.z;
			}
		}

		for (i64 y = 0; y < electricFieldSize.y; y++) {
			for (i64 x = 0; x < electricFieldSize.x; x++) {
				const auto pos = Vec3(x, y, 1.0f) * step;
				Vec3 electricity{ 0.0f };
				for (const auto& charge : charges) {
					auto direction = (pos - Vec3(charge.pos.x, charge.pos.y, 0.0f));
					const auto distance = direction.length();
					direction /= distance;
					electricity += direction.normalized() * charge.strength / pow(distance, 2.0f);
				}
				atUp({ x, y }) = electricity.z;
			}
		}
	}

	//for (int _ = 0; _ < 10; _++) {
	//	for (i64 y = 0; y < electricFieldSize.y; y++) {
	//		for (i64 x = 0; x < electricFieldSize.x; x++) {
	//			/*const auto divergence = at({ x + 1, y }).x - at({ x - 1, y }).x - at({ x, y - 1 }).y + at({ x, y + 1 }).y;*/
	//			/*const auto divergence = (at({ x + 1, y }).x - at({ x - 1, y }).x) / 2.0f 
	//				+ (-at({ x, y - 1 }).y + at({ x, y + 1 }).y) / 2.0f;*/
	//			const auto divergence = (at({ x + 1, y }).x - at({ x - 1, y }).x) / (2.0f)
	//				+ (-at({ x, y - 1 }).y + at({ x, y + 1 }).y) / (2.0f);

	//			/*const auto o = -1.0f * divergence / 4.0f - d / 4.0f; */
	//			const auto o = -1.0f * divergence / 4.0f - chargeAt({ x, y }) / 8.0f;
	//			at({ x + 1, y }).x += o;
	//			at({ x - 1, y }).x -= o;
	//			at({ x, y + 1 }).y += o;
	//			at({ x, y - 1 }).y -= o;

	//			/*const auto divergence = at({ x + 1, y }).x - at({ x - 1, y }).x - at({ x, y - 1 }).y + at({ x, y + 1 }).y;
	//			const auto o = -1.0f * divergence / 4.0f;*/
	//			//const auto divergence = 
	//			//	(at({ x + 1, y }).x - at({ x - 1, y }).x) / 2.0f
	//			//	+ (-at({ x, y - 1 }).y + at({ x, y + 1 }).y) / 2.0f;
	//			//const auto divergence =
	//			//	(at({ x + 1, y }).x - at({ x - 1, y }).x)
	//			//	+ (-at({ x, y - 1 }).y + at({ x, y + 1 }).y);
	//			//const auto o = -1.0f * divergence / 4.0f;
	//			////const auto o = -1.0f * divergence / 4.0f;
	//			//at({ x + 1, y }).x += o;
	//			//at({ x - 1, y }).x -= o;
	//			//at({ x, y + 1 }).y += o;
	//			//at({ x, y - 1 }).y -= o;

	//			/*const auto a =
	//				(at({ x + 1, y }).x - at({ x - 1, y }).x) / 2.0f
	//				+ (-at({ x, y - 1 }).y + at({ x, y + 1 }).y) / 2.0f;*/
	//			/*const auto a =
	//				(at({ x + 1, y }).x - at({ x - 1, y }).x)
	//				+ (-at({ x, y - 1 }).y + at({ x, y + 1 }).y);
	//			if (abs(a - 0.0f) > 0.15f) {
	//				t = true;
	//			}*/
	//		}
	//	}
	//}
	// Divergence is a linear operator, because it is a sum of derivatives and derivatives are linear.
	const auto p = camera.cursorPos();
	const auto gp = convert(p);

	chk(abc) {
		const auto value = (-pow(p.x, 2.0f) - pow(p.y, 2.0f)) / pow((pow(p.x, 2.0f) + pow(p.y, 2.0f)), 5.0f / 2.0f);
		ImGui::Text("analytical value %g", value);
		const auto numericalValue = (-atDown(gp) + atUp(gp)) / (step * 2.0f);
		ImGui::Text("numerical value %g", numericalValue);
	} else {
		const auto value = (pow(p.x, 2.0f) + pow(p.y, 2.0f)) / pow((pow(p.x, 2.0f) + pow(p.y, 2.0f)), 5.0f / 2.0f);
		ImGui::Text("analytical value %g", value);
		const auto numericalValue = 
			(at(gp + Vec2T<i64>{ 1, 0 }).x - at(gp - Vec2T<i64>{ 1, 0 }).x) / (step * 2.0f)
			+ (-at(gp - Vec2T<i64>{ 0, 1 }).y + at(gp + Vec2T<i64>{ 0, 1 }).y) / (step * 2.0f);
		ImGui::Text("numerical value %g", numericalValue);
	}
	


	chk(showDivergence) {
		for (i64 y = 0; y < electricFieldSize.y; y++) {
			for (i64 x = 0; x < electricFieldSize.x; x++) {
			/*	const auto divergence = 
					at({ x + 1, y }).x - at({ x - 1, y }).x 
					- at({ x, y - 1 }).y + at({ x, y + 1 }).y
					- atDown({ x, y }) + atUp({ x, y });*/
				//chk(test) {
				/*const auto divergence =
					(at({ x + 1, y }).x - at({ x - 1, y }).x) / (step * 2.0f)
					+ (-at({ x, y - 1 }).y + at({ x, y + 1 }).y) / (step * 2.0f)
					+ atDown({ x, y }) - atUp({ x, y });*/
				/*const auto divergence =
					+ (atDown({ x, y }) - atUp({ x, y })) / (step * 2.0f);*/
				const auto gp = Vec2T{ x, y };
				const auto divergence = (at(gp + Vec2T<i64>{ 1, 0 }).x - at(gp - Vec2T<i64>{ 1, 0 }).x) / (step * 2.0f)
					+ (-at(gp - Vec2T<i64>{ 0, 1 }).y + at(gp + Vec2T<i64>{ 0, 1 }).y) / (step * 2.0f)
					+ (-atDown(gp) + atUp(gp)) / (step * 2.0f);
				/*auto value = round(divergence * 10.0f) / 10.0f;*/
				auto value = round(divergence);
				if (value == 0.0f) {
					value = abs(value);
				}
				Debug::drawText(Vec2{ Vec2T<i64>{ x, y } } *step, value, Vec3::WHITE, 0.02f);
			}
		}
	}

	for (float y = aabb.min.y; y <= aabb.max.y + step; y += step) {
		for (float x = aabb.min.x; x <= aabb.max.x + step; x += step) {
			const Vec2 pos{ x, y };
			Vec2 electricity = at(convert(pos));
			const auto max = 30.0f;
			const auto strength = electricity.length();
			electricity /= strength;
			const auto color = PixelRgba::scientificColoring(strength, 0.0f, max).toVec3();
			const auto ray = electricity * step;
			Debug::drawRay(pos, ray, color);
			const auto rayEnd = pos + ray;
			/*Debug::drawRay(rayEnd, ray * 0.2f * Rotation{ degToRad(160.0f) }, color);
			Debug::drawRay(rayEnd, ray * 0.2f * Rotation{ -degToRad(160.0f) }, color);*/
			Debug::drawRay(rayEnd, ray * 0.4f * Rotation{ degToRad(160.0f) }, color);
			Debug::drawRay(rayEnd, ray * 0.4f * Rotation{ -degToRad(160.0f) }, color);
		}
	}

	auto current = camera.cursorPos();
	for (int i = 0; i < 200; i++) {
		Vec2 velocity{ 0.0f };
		for (const auto& charge : charges) {
			const auto direction = (current - charge.pos);
			const auto length = direction.length();
			if (charge.strength < 0.0f && length < 0.2f) {
				Debug::drawLine(current, charge.pos);
				goto stop;
			}
			velocity += direction * charge.strength / pow(length, 3.0f);
		}
		const auto next = current + velocity * 0.01f;
		Debug::drawLine(current, next);
		current = next;
	}
	stop:

	const auto areaAabb = Aabb::fromPoints(selectedArea);
	const auto min = convert(areaAabb.min);
	const auto max = convert(areaAabb.max);
	float sum = 0.0f;
	for (i64 y = min.y; y < max.y; y++) {
		for (i64 x = min.x; x < max.x; x++) {
			const auto pos = Vec2{ Vec2T<i64>{ x, y } } * step;
			if (!isPointInPolygon(selectedArea, pos)) {
				continue;
			}
			Debug::drawPoint(pos, Vec3::WHITE);

			/*const auto diffX = at({ x + 1, y }).x - at({ x - 1, y }).x;
			const auto diffY = at({ x, y + 1 }).y - at({ x, y - 1 }).y;*/
			//sum += diffX + diffY;
			const auto divergence = at({ x + 1, y }).x - at({ x - 1, y }).x - at({ x, y - 1 }).y + at({ x, y + 1 }).y;
			sum += divergence;
		}
	}
	ImGui::Text("%g", -sum);

	//if (selectedArea.size() >= 3) {
	//	float s = 0.0f;
	//	int previous = selectedArea.size() - 1;
	//	for (int i = 0; i < selectedArea.size(); previous = i, i++) {
	//		const auto begin = selectedArea[previous];
	//		const auto end = selectedArea[i];
	//		for (int n = 0; n < 10; n++) {
	//			const auto pos = begin + (end - begin).normalized() * (end - begin).length() * (n / 10.0f);
	//			Debug::drawPoint(pos, Vec3::WHITE);
	//			const auto [x, y] = convert(pos);
	//			const Vec2 derivative{ 
	//				(at({ x + 1, y }).x - at({ x - 1, y }).x) / 2.0f, 
	//				(at({ x, y - 1 }).y + at({ x, y + 1 }).y) / 2.0f};
	//			/*s += dot((end - begin).normalized(), derivative);*/
	//			s += dot((end - begin).normalized() * (end - begin).length() / 10.0f, derivative);
	//		}
	//	}
	//	ImGui::Text("%g", s);

	//}

	Renderer::update(camera);
}