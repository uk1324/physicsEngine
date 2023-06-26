#include <demos/refractionDemo.hpp>
#include <vector>
#include <engine/renderer.hpp>
#include <engine/debug.hpp>
#include <engine/time.hpp>
#include <engine/input.hpp>
#include <imgui/imgui.h>

std::vector<Vec2> particles;
int frame = 0;

// Is it faster to draw a single quad that draws an array of points or to draw multiple quads. Probably the latter, because less ifs?

auto RefractionDemo::update() -> void {
	Camera camera;
	frame++;

	static float speed = 0.75018754688f;
	float speedA = speed;
	ImGui::SliderFloat("speed", &speed, 0.0f, 1.0f);
	float speedOfLight = 0.5f;
	float speedOfLightInMaterial = speedOfLight * speed;
	Vec2 direction = Vec2{ 0.2f, 0.1f }.normalized();

	if (frame % 6 == 0) {
		auto randFloat = []() {
			return (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 2.0f;
		};
		//particles.push_back(Vec2{ Vec2{ -1.0f, -0.6f } + Vec2{ randFloat(), randFloat() } * 0.05f  });
		for (int i = 0; i < 10; i++) {
			const auto startPos = Vec2{ -1.0f, -0.6f };
			const auto pos = startPos + direction.rotBy90deg() * 0.01 * i;
			particles.push_back(pos);
		}
	}

	Debug::drawLine(Vec2{ 0.0f, 10.0f }, Vec2{ 0.0f, -10.0f });

	for (auto& particle : particles) {
		

		Vec2 speed = direction * speedOfLight;
		if (particle.x > 0.0f) {
			Vec2 normal = Vec2{ 1.0f, 0.0f };
			speed -= normal * dot(normal, direction) * 0.5f;
			/*const auto indexOfRefraction = speedOfLight / speedOfLightInMaterial;
			Vec2 normal = Vec2{ 1.0f, 0.0f };
			float n0 = cross(normal, direction);
			const auto n1 = n0 / indexOfRefraction;
			const auto angle = asin(n1);
			speed = speedOfLightInMaterial * normal * Rotation{ angle };*/
			//// n1 = n0 / i
			//float cosBetween = cross(normal, direction);

			/*speed *= Rotation{ angle };*/
			/*const auto indexOfRefraction = speedOfLight.length() / speedOfLightInMaterial.length();*/
			//const auto indexOfRefraction = speedOfLight.length() / speedOfLightInMaterial.length();
			//Vec2 normal = Vec2{ 1.0f, 0.0f };
			//const auto n0 = cross(normal, speedOfLight);
			////const auto n0 = speedOfLight.angle() - PI<float>;
			//// i = n0 / n1
			//// n1 = n0 / i
			////speed = speedOfLightInMaterial;
			//const auto sinAngle = n0 / indexOfRefraction;
			//const auto angle = asin(sinAngle);
			////speed *= Rotation{ angle };
			//const auto angleBefore = speedOfLight.angle();
			//speed = speedOfLightInMaterial.length() * normal * Rotation{ angle };
		}
		particle += speed * Time::deltaTime();
		Debug::drawPoint(particle);

		particles.erase(std::remove_if(particles.begin(), particles.end(), [](Vec2 pos) { return pos.x > 1.0f; }), particles.end());
	}

	
	if (Input::isKeyDown(Keycode::R)) {
		particles.clear();
	}

	Renderer::update(camera);
}
