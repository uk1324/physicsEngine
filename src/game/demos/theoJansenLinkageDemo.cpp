#include <game/demos/theoJansenLinkageDemo.hpp>
#include <game/ent.hpp>

#include <imgui/imgui.h>
using namespace ImGui;

auto TheoJansenLinkageDemo::name() -> const char* {
	return "theo jansen linkage";
}

auto TheoJansenLinkageDemo::loadSettingsGui() -> void {

}

auto TheoJansenLinkageDemo::load() -> void {
	auto makeCircle = [](Vec2 pos = Vec2{ 0.0f }) {
		return ent.body.create(Body{ pos, CircleCollider{ 0.2f }, false }).id;
	};
	const auto scale = 20.0f;
	auto connect = [&](const BodyId& a, const BodyId& b, float length) -> void {
		ent.distanceJoint.create(DistanceJoint{ a, b, length });
	};

	auto makeTriPoint = [&](const BodyId& a, const BodyId& b, float lengthAc, float lengthBc, bool flip = false) -> BodyId {
		const auto aPos = ent.body.get(a)->transform.pos;
		const auto bPos = ent.body.get(b)->transform.pos;
		const auto lengthAb = distance(aPos, bPos);
		const auto angleCos = (pow(lengthAb, 2.0f) + pow(lengthAc, 2.0f) - pow(lengthBc, 2.0f)) / (2.0f * lengthAb * lengthAc);
		const auto angle = acos(angleCos);
		ASSERT(!isnan(angle));
		const auto cPos = aPos + (bPos - aPos).normalized() * Rotation(angle * (flip ? -1.0f : 1.0f)) * lengthAc;
		const auto c = makeCircle(cPos);
		return c;
	};

	auto makeTri = [&](const BodyId& a, const BodyId& b, float lengthAc, float lengthBc) -> BodyId {
		lengthAc /= scale;
		lengthBc /= scale;
		const auto c = makeTriPoint(a, b, lengthAc, lengthBc);
		connect(a, c, lengthAc);
		connect(b, c, lengthBc);
		return c;
	};

	auto makeQuad = [&](const BodyId& a, const BodyId& b, float bc, float cd, float ad, bool flip = false) -> std::pair<BodyId, BodyId> {
		bc /= scale;
		cd /= scale;
		ad /= scale;
		const auto aPos = ent.body.get(a)->transform.pos;
		const auto bPos = ent.body.get(b)->transform.pos;
		const auto ab = distance(aPos, bPos);

		const auto ac = sqrt(((ab * cd) + (bc * ad)) * ((ab * ad) + (bc * cd)) / ((ab * bc) + (cd * ad)));

		const auto c = makeTriPoint(a, b, ac, bc, flip);
		const auto d = makeTriPoint(a, c, ad, cd, flip);

		connect(b, c, bc);
		connect(a, d, ad);
		connect(d, c, cd);
		//connect(c, d, cd);
		/*const auto h = ((ab * cd) + (bd * ac)) * ((ab * bd) + (cd * ac));
		const auto ad = sqrt(h / ((ab * ac)) + (bd * cd));
		const auto c = makeTriPoint(a, b, ac, ad);*/
		//const auto c = makeTriPoint(a, c, );
		//const auto d = makeTriPoint(c, b, cd, bd);
		//connect(a, c, ac);
		return { c, d };
	};
	// Bottom triangle
	{
		//const auto a = makeCircle(Vec2{ 0.0f, 1.0f });
		//const auto c = makeCircle(Vec2{ 0.0f, 1.0f + 65.7f / scale });
		//connect(a, c, 65.7f / scale);
		//const auto b = makeTri(c, a, 36.7f, 49.0f);


		////const auto b = makeCircle(Vec2{ 0.0f, 1.0f });
		////const auto e = makeCircle(Vec2{ 0.0f, 1.0f + 39.3f / scale });
		////connect(b, e, 39.3f / scale);
		////const auto [m, g] = makeQuad(b, e, em, 15.0f, 61.9f, true);

		////const auto [c, d] = makeQuad(b, e, 36.7f, 39.4f, 40.1f);

		////const auto f = makeTriPoint(d, e, 55.8f, 41.5f);

		//const auto [e, d] = makeQuad(c, b, 39.3f, 40.1f, 39.4f);
		//const auto f = makeTri(d, e, 55.8f, 41.5f);

		//const auto g = makeTriPoint(f, b, 50.0f / scale, 61.9f / scale);
		//connect(b, g, 61.9f / scale);
		//connect(f, g, 50.0f / scale);

		//const auto em = sqrt(pow(38.0f, 2.0f) + pow(7.8f, 2.0f));
		//const auto ePos = ent.body.get(e)->transform.pos;
		////const auto am = makeCircle(ePos + Vec2{ 38.0f / scale, 0.0f });
		//const auto m = makeCircle(ePos + Vec2{ 38.0f / scale, 7.8f / scale });
		///*connect(e, am, 38.0f / scale);
		//connect(am, m, 7.8f / scale);
		//connect(e, m, em / scale);*/
		//connect(m, g, 15.0f / scale);
	}

	const Vec2 points[] = { Vec2{ 1.44549f, 1.51909f }, Vec2{ 0.716788f, 4.72029f }, Vec2{ 2.31381f, 3.81405f }, Vec2{ 3.05905f, 5.64764f }, Vec2{ 1.28979f, 6.60129f }, Vec2{ 3.91004f, 7.54356f }, Vec2{ 5.02059f, 5.30811f }, Vec2{ 5.09323f, 6.05882f }, Vec2{ 5.09323f, 6.05882f } };

	//for (const auto p : points) {
	//	makeCircle(p);
	//}
	//const auto m = makeCircle( - Vec2{ 15.0f / scale, 0.0f });
	/*connect(g, m, 15.0f / scale);
	connect(m, e, em / scale);*/
	//const auto c = makeCircle();
	/*connect(a, b, 49.0f);
	connect(b, c, 36.7f);
	connect(c, a, 65.7f);*/
	//const auto d = makeCircle();
	//connect(c, d, 39.4f / scale);
	//const auto e = makeCircle();
	//connect(d, e, 40.1f / scale);
	//connect(b, e, 39.3f / scale);
	//const auto f = makeTri(d, e, 55.8f, 41.5f);
	////const auto f = makeCircle();
	///*connect(d, f, 55.8f / scale);
	//connect(e, f, 41.5f / scale);*/
	//const auto g = makeCircle();
	//connect(f, g, 50.0f / scale);
	//connect(b, g, 61.9f / scale);

	/*const auto main0 = makeCircle();
	const auto main1 = makeCircle();
	ent.revoluteJoint.create(RevoluteJoint{ main1.id, main0.id, Vec2{ 0.0f }, Vec2{ 0.0f }, 0.0f });
	ent.collisionsToIgnore.insert({ main0.id, main1.id });

	ent.revoluteJoint.create(RevoluteJoint{ main1.id, g.id, Vec2{ 0.0f }, Vec2{ 15.0f / scale, 0.0f }, 0.0f });*/

	ent.body.create(Body{ Vec2{ 0.0f, -50.0f }, BoxCollider{ Vec2{ 200.0f, 100.0f } }, true });
}
