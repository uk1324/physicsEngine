#include <demos/integrationDemo.hpp>
#include <engine/renderer.hpp>
#include <utils/dbg.hpp>
#include <engine/time.hpp>

auto circleRadius = 0.5f;
const auto startHeight = 6.0f;
const auto gravity = -5.0f;

std::vector<float> maxPos;
std::vector<float> maxVelT;

IntegrationDemo::IntegrationDemo() {
	const auto gapSize = 0.5f;
	
	const auto r = circleRadius + 0.2f;
	for (i32 i = 0; i < std::size(boxes); i++) {
		auto& box = boxes[i];
		box.pos = Vec2{ (i) * r * 2.0f - (std::size(boxes) / 2.f - 0.5f) * r * 2.0f, startHeight };
	}
}

std::vector<Vec2> pos0;
std::vector<Vec2> pos1;

struct Particle {
	float v = 0.0f;
	float x = 1.0f;
};

Particle p0;
Particle p1;


auto IntegrationDemo::update() -> void {
	
	Camera camera;
	camera.zoom /= 8.0f;
	camera.pos += Vec2{ 0.0f, 4.0f };
	static float t = 0.0f;
	const auto dt = Time::deltaTime();

	//static auto freqencySquared = 20.0f;
	////freqencySquared += 1.0f;
	//for (int i = 0; i < 1; i++) {
	//	p0.v -= dt * freqencySquared * p0.x;
	//	p0.x += dt * p0.v;

	//	const auto v = p1.v;
	//	p1.v -= dt * freqencySquared * p1.x;
	//	p1.x += dt * v;
	//}

	//auto update = [&](std::vector<Vec2>& v, Particle p, Vec3 c) {
	//	if (v.size() < 5000)
	//		v.push_back({ p.x, p.v });
	//	for (int i = 1; i < v.size(); i++) {
	//		auto a = v[i - 1];
	//		a.y /= sqrt(freqencySquared);
	//		auto b = v[i];
	//		b.y /= sqrt(freqencySquared);
	//		Debug::drawLine(a, b, c);
	//	}
	//};
	//update(pos0, p0, Vec3::GREEN);
	//update(pos1, p1, Vec3::RED);


	ImGui::Text("even though implicit euler with position correction gained a bit of height it seems to still be stable");
	ImGui::PlotLines("test", maxPos.data(), maxPos.size(), 0, nullptr, FLT_MAX, FLT_MAX, Vec2{ 400.f });
	if (maxVelT.size() != 0)
		ImGui::Text("last max pos time %g", maxVelT.back());
	ImGui::Text("start height %g", startHeight);
	ImGui::Text("elapsed time %gs", t);
	
	const auto updateValue = 100000;
	const auto elapsedPerFrame = updateValue * dt;
	ImGui::Text("elapsed per frame %gs", elapsedPerFrame);
	static int updates = 1;
	chk(updateFast) {
		updates = updateValue;
	} else {
		updates = 1;
	}
	for (int i = 0; i < updates; i++) {
		/* 
		One thing that makes some balls gain energy is doing this
		b.velY += gravity * dt;
		if (b.pos.y < circleRadius && b.velY < 0.0f) {
			b.velY = -b.velY;
		}
		instead of this
		if (b.pos.y < circleRadius && b.velY < 0.0f) {
			b.velY = -b.velY;
		} else {
			b.velY += gravity * dt;
		}
		It seems that the latter is correct, because most places use it, but I don't thing doing something like this is possible in a more general solver. The gravity forces have to be integrated at the start of the frame no matter what. 
		When using explicit euler the first version makes it diverge while in the second version it looses evergy.
		Other methods also seems to diverge when using the first version.

		There are actually 2 verions of implicit euler described which is described in the wikipedia article.
		*/

		// Some balls look like the have the same height, but they are slightly different.

		

		// Explicit euler
		{
			auto& b = boxes[0];
			const auto v = b.velY;
			b.velY += gravity * dt;
			if (b.pos.y < circleRadius && b.velY < 0.0f) {
				b.velY = -b.velY;
			}
			b.pos.y += v * dt;
		}

		// Implicit euler
		{
			auto& b = boxes[1];
			if (b.pos.y < circleRadius && b.velY < 0.0f) {
				const auto error = circleRadius - b.pos.y;
				b.velY = -b.velY;
			} else {
				b.velY += gravity * dt;
			}
			b.pos.y += b.velY * dt;
		}
		
		// Implicit euler with position correction.
		// When the time step is decreased the balls gains more height, but it always seems to converge to some value after not too long.
		{
			auto& b = boxes[2];
			if (b.pos.y < circleRadius && b.velY < 0.0f) {
				const auto error = circleRadius - b.pos.y;
				b.velY = -b.velY;
				b.pos.y += error;
			} else {
				b.velY += gravity * dt;
			}
			b.pos.y += b.velY * dt;
			if (maxPos.size() == 0 || b.pos.y > maxPos.back()) {
				maxPos.push_back(b.pos.y);
				maxVelT.push_back(t + dt * i);
			}
		}

		// Implicit euler with velocity based position correction.
		{
			auto& b = boxes[3];
			if (b.pos.y < circleRadius && b.velY < 0.0f) {
				const auto error = circleRadius - b.pos.y;
				b.velY = -b.velY;
				b.velY += error; // Is this line correct?
			} else {
				b.velY += gravity * dt;
			}
			b.pos.y += b.velY * dt;
		}

		{
			auto& b = boxes[4];
			b.pos.y += b.velY * dt;
			if (b.pos.y < circleRadius) {
				b.velY = -b.velY;
			} else {
				b.velY += gravity * dt;
			}
		}

		{
			auto& b = boxes[5];
			if (b.pos.y < circleRadius) {
				b.velY = -b.velY;
			} else {
				b.velY += gravity * dt;
			}
			b.pos.y += b.velY * dt;
		}
	}
	// To prevent the small dts from getting rounded down.
	t += dt * updates;

	//{
	//	const auto dt = Time::deltaTime();
	//	{
	//		auto& b = boxes[0];
	//		b.pos.y += b.velY * dt;
	//		b.velY += gravity * dt;
	//		if (b.pos.y < circleRadius) {
	//			b.velY = -b.velY;
	//		}
	//	}

	//	{
	//		auto& b = boxes[1];
	//		if (b.pos.y < circleRadius) {
	//			b.velY = -b.velY;
	//		}
	//		b.velY += gravity * dt;
	//		b.pos.y += b.velY * dt;
	//	}

	//	{
	//		auto& b = boxes[2];
	//		if (b.pos.y < circleRadius) {
	//			b.velY = -b.velY;
	//		}
	//		b.velY += gravity * dt;
	//		const auto error = circleRadius - b.pos.y;
	//		if (error > 0.0f) {
	//			b.pos.y += error;
	//		}
	//		b.pos.y += b.velY * dt;
	//	}

	//	{
	//		auto& b = boxes[3];
	//		const auto error = circleRadius - b.pos.y;
	//		if (b.pos.y < circleRadius) {
	//			b.velY = -b.velY + error;
	//		}
	//		b.velY += gravity * dt;
	//		b.pos.y += b.velY * dt;
	//	}

	//	{
	//		auto& b = boxes[4];
	//		b.velY += gravity * dt;
	//		const auto vel = b.velY;
	//		b.pos.y += vel * dt;
	//		const auto error = circleRadius - b.pos.y;
	//		if (b.pos.y < circleRadius && b.velY < 0.0f) {
	//			b.velY = -b.velY;
	//			b.pos.y += error;
	//		}
	//	}

	//	{
	//		auto& b = boxes[5];
	//		const auto error = circleRadius - b.pos.y;
	//		const auto collision = b.pos.y < circleRadius&& b.velY < 0.0f;
	//		b.velY += gravity * dt;
	//		const auto vel = b.velY;
	//		if (collision) {
	//			b.velY = -b.velY;
	//			b.pos.y += error;
	//		}
	//		b.pos.y += vel * dt;
	//		/*const auto error = circleRadius - b.pos.y;
	//		if (b.pos.y < circleRadius && b.velY < 0.0f) {
	//			b.velY = -b.velY;
	//			b.pos.y += error;
	//		}*/
	//	}
	//}

	//{
	//	auto& b = boxes[4];
	//	b.velY += gravity * dt;
	//	const auto vel = b.velY;
	//	if (b.pos.y < circleRadius && b.velY < 0.0f) {
	//		b.velY = -b.velY;
	//	}
	//	b.pos.y += vel * dt;
	//}

	//{
	//	auto& b = boxes[5];
	//	b.velY += gravity * dt;
	//	const auto vel = b.velY;
	//	const auto error = circleRadius - b.pos.y;
	//	if (b.pos.y < circleRadius && b.velY < 0.0f) {
	//		b.velY = -b.velY;
	//		b.pos.y += error;
	//	}
	//	b.pos.y += vel * dt;
	//}

	//{
	//	auto& b = boxes[4];
	//	b.velY += gravity * dt;
	//	const auto vel = b.velY;
	//	const auto error = circleRadius - b.pos.y;
	//	if (b.pos.y < circleRadius && b.velY < 0.0f) {
	//		b.velY = -b.velY + error;
	//	}
	//	b.pos.y += vel * dt;
	//}

	//{
	//	auto& b = boxes[5];
	//	b.velY += gravity * dt;
	//	const auto vel = b.velY;
	//	const auto error = circleRadius - b.pos.y;
	//	if (b.pos.y < circleRadius && b.velY < 0.0f) {
	//		b.velY = -b.velY;
	//		b.pos.y += error;
	//	}
	//	b.pos.y += vel * dt;
	//}

	//{
	//	auto& b = boxes[4];
	//	b.velY += gravity * dt;
	//	const auto vel = b.velY;
	//	const auto error = circleRadius - b.pos.y;
	//	if (b.pos.y < circleRadius && b.velY < 0.0f) {
	//		b.velY = -b.velY + error;
	//	}
	//	b.pos.y += vel * dt;
	//}

	//{
	//	auto& b = boxes[5];
	//	b.velY += gravity * dt;
	//	const auto vel = b.velY;
	//	const auto error = circleRadius - b.pos.y;
	//	if (b.pos.y < circleRadius && b.velY < 0.0f) {
	//		b.velY = -b.velY;
	//		b.pos.y += error;
	//	}
	//	b.pos.y += vel * dt;
	//}


	//{
	//	auto& b = boxes[4];
	//	b.velY += gravity * dt;
	//	if (b.pos.y < circleRadius) {
	//		b.velY = -b.velY;
	//	}
	//	const auto error = circleRadius - b.pos.y;
	//	if (error > 0.0f) {
	//		b.pos.y += error;
	//	}
	//	b.pos.y += b.velY * dt;
	//}

	//{
	//	auto& b = boxes[5];
	//	const auto error = circleRadius - b.pos.y;
	//	b.velY += gravity * dt;
	//	if (b.pos.y < circleRadius) {
	//		b.velY = -b.velY + error;
	//		//b.velY +=  / dt;
	//	}
	//	b.pos.y += b.velY * dt;
	//}

	//{
	//	auto& b = boxes[2];
	//	if (b.pos.y < circleRadius) {
	//		b.velY = -b.velY;
	//	}
	//	b.velY += gravity * dt;
	//	const auto error = circleRadius - b.pos.y;
	//	if (error > 0.0f) {
	//		b.pos.y += error;
	//	}
	//	b.pos.y += b.velY * dt;
	//}

	//{
	//	auto& b = boxes[3];
	//	const auto error = circleRadius - b.pos.y;
	//	if (b.pos.y < circleRadius) {
	//		b.velY = -b.velY + error;
	//		//b.velY +=  / dt;
	//	}
	//	b.velY += gravity * dt;
	//	b.pos.y += b.velY * dt;
	//}

	for (const auto& box : boxes) {
		Debug::drawHollowCircle(box.pos, circleRadius);
	}


	Debug::drawLine(Vec2{ -camera.width() / 2.0f, 0.0f }, Vec2{ camera.width() / 2.0f, 0.0f });
	Debug::drawLine(Vec2{ -camera.width() / 2.0f, startHeight }, Vec2{ camera.width() / 2.0f, startHeight }, Vec3::RED);

	Renderer::update(camera, 0.5f);
}
