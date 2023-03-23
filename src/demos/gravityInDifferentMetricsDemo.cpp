#include <demos/gravityInDifferentMetricsDemo.hpp>
#include <engine/input.hpp>
#include <engine/time.hpp>
#include <utils/dbg.hpp>
#include <numeric>

using namespace ImGui;
#include <string>
using namespace std;

struct Ratio {
	int numerator;
	int denominator;

	auto operator==(const Ratio&) const -> bool = default;
};

static auto isPrime(int x) -> bool {
	if (x <= 1)
		return false;

	for (int i = 2; i <= sqrt(x); i++) {
		if (x % i == 0)
			return false;
	}
	return true;
}

static auto floatToRatio(float x) -> Ratio {
	float integralPart;
	auto fractionalPart = modf(x, &integralPart);

	if (fractionalPart < 0.0f) {
		// Store the sign inside the integral part.
		fractionalPart = -fractionalPart;
		integralPart = -integralPart;
	}

	int denominatorPower = 0;
	float y = fractionalPart;
	float _;
	while (const auto isNotInteger = modf(y, &_) > 0.0f) {
		// @Performance: This convert it into a decimal fraction, but should work with any integer. Using 2 would make the multiplication faster, but it would take more steps. Don't know how it impacts the gdc algorithm.
		y *= 10.0f;
		denominatorPower++;
	}
	const auto decimalNumerator = static_cast<int>(y);
	const auto decimalDenominator = static_cast<int>(pow(10.0f, denominatorPower));
	const auto gdc = std::gcd(decimalNumerator, decimalDenominator);
	const auto denominator = decimalDenominator / gdc;
	return { decimalNumerator / gdc + denominator * static_cast<int>(integralPart), denominator };
}

// https://codegolf.stackexchange.com/questions/63629/calculate-the-p-adic-norm-of-a-rational-number
static auto padic(Ratio ratio, int p) -> Ratio {
	auto f = [&p](int m) -> int {
		// Convert m to k * p^n. Return just the p^n part.
		int x = 1;
		while (m % p == 0) {
			x *= p;
			m /= p;
		}
		return x;
		//return (m % p) == 0 ? f(m / p) * p : 1;
	};

	if (ratio.numerator == 0 || ratio.denominator == 0) {
		return Ratio{ 0, 1 };
	}

	auto gcd = std::gcd(ratio.numerator, ratio.denominator);
	// The p-adic norm for a / b = p^n * (m / n) is equal to p^(-n). 
	// f() extracts the p^n part and then the put the denominator on top and the numerator on bottom to calculate the reciprocal.
	return Ratio{ f(ratio.denominator / gcd), f(ratio.numerator / gcd) };
}

static auto padic(float m, int p) -> float {
	if (isnan(m))
		return 0;
	if (m == 0)
		return 0;

	const auto ratio = floatToRatio(m);
	const auto result = padic(ratio, p);
	return static_cast<float>(result.numerator) / static_cast<float>(result.denominator);
}

static auto testPadic() -> void {
	auto test = [](const Ratio& result, const Ratio& expected) -> void { 
		ASSERT(result == expected);
		put("%d/%d\n", result.numerator, result.denominator);
	};
	const Ratio r{ 63, 550 };
	test(padic(r, 2), { 2, 1 });
	test(padic(r, 3), { 1, 9 });
	test(padic(r, 5), { 25, 1 });
	test(padic(r, 7), { 1, 7 });
	test(padic(r, 11), { 11, 1 });
	test(padic(r, 13), { 1, 1 });
}

GravityInDifferentMetricsDemo::GravityInDifferentMetricsDemo() 
	: selectedColor{ nextRandomColor() } {
	camera.zoom /= 8.0f;
	testPadic();
}

auto GravityInDifferentMetricsDemo::update() -> void {
	camera.moveOnWasd();
	camera.scrollOnCursorPos();
	Begin("gravity");
	TextWrapped("Click once to select position and a second time to selected the velocity. Click on a body to delete it.");
	InputFloat("radius", &selectedRadius);
	InputFloat("mass", &selectedMass);
	Checkbox("is static", &selectedIsStatic);
	Checkbox("paused", &isPaused);
	InputInt("speed", &speed);
	ColorEdit3("color", selectedColor.data());
	Checkbox("cool", &cool);
	if (cool) {
		InputFloat("cool value", &coolValue);
	}
	Combo("metric", reinterpret_cast<int*>(&selectedMetric), "lp-norm\0chebyshev\0discrete\0\p-adic\0triangle\0sin\0\0");
	switch (selectedMetric) {
	case Metric::LP_NORM:
		InputFloat("p value", &pNormP);
		break;

	case Metric::P_ADIC:
		Text("p value %d", pAdicP);
		SameLine();
		if (Button("-")) {
			do {
				pAdicP--;
			} while (pAdicP > 2 && !isPrime(pAdicP));
		}
		SameLine();
		if (Button("+")) {
			do {
				pAdicP++;
			} while (!isPrime(pAdicP));
		}
		break;

	case Metric::SIN:
		InputFloat("pow", &sinPow);
		break;
	default:
		break;
	}
	if (Button("reset")) {
		bodies.clear();
	}
	End();

	/*
	Cool patterns

	Place one body with zero velocity and for the second one select a velocity to make the preview orbit the other planet. The planets will trochoid like orbits which are perodic functions. Not sure if they are exacly trochoid, because there are can get offset a bit.
	This is best visible when the mass difference isn't too big. And the object with bigger mass is created first. Masses can also be equal.
	It also looks cool when the eccentricity of the start orbit is high.

	The simplest way to get a working moon I found is to first create a static massive planet. Then make a planet with the same mass orbit it. Then create a planet with a small mass near the orbiting planet and make it have a lot of velocity. The mass difference shouldn't be too big. 5555 and 10 fine for me.
	*/

	// const auto force = distance * pow(metric(distance);
	//const auto force = distance * metric(distance);

	int substeps = 5;
	const auto dt = Time::deltaTime() / substeps;
	auto updateVel = [this, &dt](Body& a) -> void {
		for (auto& b : bodies) {
			if (&a == &b)
				continue;
			const auto distance = a.pos - b.pos;
			auto d = distance;
			if (cool) {
				// Without normalizing the orbits kinda look like roulettes for example hypotrochoids.
				// This is probably because it exaggerates the effects of https://en.wikipedia.org/wiki/Apsidal_precession.
				// If the focues point is close to the orbit you can see the apsidal_precession well, but this might be a side effect of the integration method.
				// When a pNormP is chosen after a while when the trails look like circles in different lp spaces. For example 0.5 looks like some circle with value > 2.
				const auto force = distance * (gravitationalConstant * a.mass * b.mass) / pow(metric(distance), coolValue);
				//const auto force = distance * (abs(distance.x) + abs(distance.y)) * (gravitationalConstant * a.mass * b.mass);
				a.vel -= force * dt / a.mass;
			} else {
				// Raised to 3 to normalize the distance vector.
				const auto force = distance * (gravitationalConstant * a.mass * b.mass) / pow(metric(distance), 3.0f);
				a.vel -= force * dt / a.mass;
			}
		}
	};

	auto updatePos = [&dt](Body& body) -> void {
		body.pos += body.vel * dt;
	};

	auto updateTrail = [](Body& body) -> void {
		const auto lastPos = body.trial.back();
		if (distance(body.pos, lastPos) > 0.1f) {
			body.trial.push_back(body.pos);
		}
	};

	auto drawBody = [](const Body& body) -> void {
		Debug::drawHollowCircle(body.pos, body.radius, body.color);
		Debug::drawText(body.pos, body.mass);
	};

	auto drawTrail = [](const Body& body) -> void {
		// TODO: LOD
		// Store previous if distance to small go to next.
		for (usize i = 0; i < body.trial.size() - 1; i++) {
			Debug::drawLine(body.trial[i], body.trial[i + 1], body.color);
		}
		Debug::drawLine(body.trial.back(), body.pos, body.color);
	};

	auto makeSelected = [*this]() -> Body {
		Body b{ *selectedPos, selectedRadius, selectedMass, camera.cursorPos() - *selectedPos, selectedColor, selectedIsStatic };
		b.trial.push_back(b.pos);
		return b;
	};

	if (selectedPos.has_value()) {
		Body preview = makeSelected();
		drawBody(preview);
		for (int i = 0; i < 200; i++) {
			for (int j = 0; j < substeps; j++) {
				updateVel(preview);
				updatePos(preview);
				updateTrail(preview);
			}
		}
		drawTrail(preview);
	}

	if (Input::isMouseButtonDown(MouseButton::RIGHT)) {
		selectedPos = std::nullopt;
	}
	if (Input::isMouseButtonDown(MouseButton::LEFT)) {
		bool deleted = false;
		for (auto it = bodies.begin(); it != bodies.end(); ++it) {
			if (distance(camera.cursorPos(), it->pos) <= it->radius) {
				bodies.erase(it);
				deleted = true;
				break;
			}
		}
		if (!deleted) {
			if (!selectedPos.has_value()) {
				selectedPos = camera.cursorPos();
			} else {
				bodies.push_back(makeSelected());
				selectedPos = std::nullopt;
				selectedColor = nextRandomColor();
			}
		}
	}

	for (const auto& body : bodies) {
		drawBody(body);
		if (body.trial.size() >= 1) {
			drawTrail(body);
		}
	}

	if (!isPaused) {
		for (int i = 0; i < speed; i++) {
			for (int j = 0; j < substeps; j++) {
				for (auto& a : bodies) {
					updateVel(a);
				}

				for (auto& body : bodies) {
					if (body.isStatic)
						continue;
					updatePos(body);
				}
			}
		}
	}

	for (auto& body : bodies) {
		updateTrail(body);
	}

	Renderer::update(camera, powf(2.0, round(log2f(1.0f / camera.zoom / 25.0f))));
}

auto GravityInDifferentMetricsDemo::metric(Vec2 v) -> float {
	// Derivation of equations of planetary motion. http://www.wiu.edu/cas/mathematics_and_philosophy/graduate/equations-planetary-motion.pdf
	// When a single body is statinary then the differential equation describing it's motion on a single axis is.
	// Eq(x(t).diff(t).diff(t), -x(t) / (sqrt(x(t) ** 2 + y(t) ** 2) ** 3))
	// It is raised to the power of 3 to notmalize the vector.
	// So the general equation would be 
	// Eq(x(t).diff(t).diff(t), -x(t) / (norm(x(t), y(t)) ** 3))

	// Making x = cos(t) and y = sin(t)
	// -cos(t) = -cos(t) / (sqrt(cos(t)^2 + sin(t)^2) ** 3))
	// -cos(t) = -cos(t) / 1

	// Not sure if this is useful for anything. https://math.stackexchange.com/questions/2176371/how-do-trig-functions-change-in-an-l-p-norm

	switch (selectedMetric) {
	case Metric::LP_NORM: return pow(pow(abs(v.x), pNormP) + pow(abs(v.y), pNormP), 1.0f / pNormP);
	case Metric::CHEBYSHEV: return std::max(abs(v.x), abs(v.y));
	case Metric::DISCRETE: return v == Vec2{ 0.0f, 0.0f } ? 0.0f : 1.0f;
	case Metric::P_ADIC: return padic(abs(v.x), pAdicP) + padic(abs(v.y), pAdicP);
	case Metric::TRIANGLE: return max(abs(v.x) * 0.866f + (v.y) * 0.5f, -v.y);
	case Metric::SIN: return sin(pow(abs(v.x), sinPow)) + sin(pow(abs(v.y), sinPow));
	}
}

auto GravityInDifferentMetricsDemo::nextRandomColor() -> Vec3 {
	return PixelRgba::fromHsv(static_cast<float>(rand()) / RAND_MAX, 1.0f, 1.0f).toVec3();
}
