#include <game/game.hpp>
#include <engine/debug.hpp>
#include <game/input.hpp>
#include <game/collision.hpp>
#include <game/distanceJoint.hpp>
#include <engine/time.hpp>
#include <engine/window.hpp>
#include <math/utils.hpp>
#include <engine/renderer.hpp>
#include <imgui/imgui.h>
#include <game/ent.hpp>
#include <game/levelFormat/levelData.hpp>
#include <utils/overloaded.hpp>
#include <engine/frameAllocator.hpp>
#include <customImguiWidgets.hpp>

// spatial hashing / bucketing
// inactive flag on objects

// The exact number of collisions pairs checked in the O(n^2) broadphase is choose(n, 2).
// choose(n, 2) = n * (n - 1) / 2 = n^2 - n / 2. Taking the limit as n goes to infinity you just get n^2 / 2


#include <sstream>
#include <utils/io.hpp>
#include <json/Json.hpp>
#include <fstream>

std::vector<BodyId> bodies;
Game::Game() {
	//int height = 10;
	//float boxSize = 1.0f;
	//float gapSize = 0.1f;
	//for (int i = 1; i < height + 1; i++) {
	//	std::vector<BodyId> bodies;
	//	for (int j = 0; j < i; j++) {
	//		float y = (height + 1 - i) * (boxSize + gapSize);
	//		float x = -i * (boxSize / 2.0f + boxSize / 8.0f) + j * (boxSize + boxSize / 4.0f);

	//		ent.body.create(Body{ Vec2{ x, y }, BoxCollider{ Vec2{ boxSize } }, false });
	//		//ent.body.create(Body{ Vec2{ x, y }, CircleCollider{ 0.5f }, false });	

	//		//ent.body.create(Body{ Vec2{ x, y }, ConvexPolygon::regular(3, 0.5f), false });

	//		/*float radius = 0.7f;
	//		if (i == height) {
	//			auto gon = ConvexPolygon::regular(6, radius);
	//			gon.verts.pop_back();
	//			gon.calculateNormals();
	//			const auto& [id, b] = ent.body.create(Body{ Vec2{ x, y }, gon, false });
	//			bodies.push_back(id);
	//			b.transform.rot *= Rotation{ -TAU<float> / 6.0f };
	//		} else {
	//			const auto& [id, _] = ent.body.create(Body{ Vec2{ x, y }, ConvexPolygon::regular(6, radius), false });
	//			bodies.push_back(id);
	//		}*/
	//	}

	///*	if (i == height) {
	//		for (int j = 0; j < bodies.size() - 1; j++) {
	//			ent.distanceJoint.create(DistanceJoint{ bodies[j], bodies[j + 1], boxSize + boxSize / 4.0f });
	//		}
	//	}*/
	//}

	//{
	//	float height = 2.0f;
	//	float width = 0.5f;
	//	for (int i = 0; i < 25; i++) {
	//		const auto& [_, body] = ent.body.create(Body{ Vec2{ i * (height + 0.02f), height / 2.0f }, BoxCollider{ Vec2{ width, height } } });
	//		body.coefficientOfFriction = 0.7f;
	//	}
	//}

	{
		float width = 0.5f;
		float height = 4.0f;
		auto addJoint = [this](BodyId a, BodyId b, Vec2 anchorA, Vec2 anchorB) {
			auto [jointId, _] = ent.distanceJoint.create(DistanceJoint{ a, b, 0.0f, anchorA, anchorB });
			const BodyPair bodyPair{ a, b };
			revoluteJointsWithIgnoredCollisions.push_back({ jointId, bodyPair });
		};

		for (int i = 0; i < 4; i++) {
			const auto pos = Vec2{ i * (height - width), 0.0f };
			const auto collider = BoxCollider{ Vec2{ height, width } };
			for (int _ = 0; _ < 2; _++) {
				const auto& [id, body] = ent.body.create(Body{ pos + Vec2{ 0.0f, 2.0f }, collider });
				bodies.push_back(id);
			}
			
			addJoint(bodies.back(), *(bodies.end() - 2), Vec2{ 0.0f }, Vec2{ 0.0f });
		}

		for (size_t i = 0; i < bodies.size() - 2; i += 2) {
			for (int j = 0; j < 2; j++) {
				const auto anchor = Vec2{ height / 2.0f - width / 2.0f, 0.0f };
				addJoint(bodies[i + j], bodies[i + j + 2], anchor, -anchor);
			}
		}

		for (const auto& a : bodies) {
			for (const auto& b : bodies) {
				if (&a != &b) {
					collisionSystem.collisionsToIgnore.insert({ a, b });
				}
			}
		}

		for (size_t i = 0; i < bodies.size() - 3; i += 2) {
			collisionSystem.collisionsToIgnore.erase({ bodies[i], bodies[i + 3] });
		}

		// Attach the endpoints to a prismatic joint?
		for (int i = 0; i < bodies.size(); i += 2) {
			auto b0 = ent.body.get(bodies[i + 1]);
			const auto a = PI<float> / 10.0f;
			const auto b = PI<float> - a;
			b0->transform.rot = Rotation{ a };
			auto b1 = ent.body.get(bodies[i]);
			b1->transform.rot = Rotation{ b };
		}

	}

	//ent.body.create(Body{ Vec2{ 0.0f, -50.0f }, BoxCollider{ Vec2{ 100.0f } }, true });
	ent.body.create(Body{ Vec2{ 0.0f, -50.0f }, BoxCollider{ Vec2{ 200.0f, 100.0f } }, true });
	/*const auto& [a, a_] = ent.body.create(Body{ Vec2{ 1.0f, 3.0f }, BoxCollider{ Vec2{ 1.0f } }, false });
	const auto& [b, b_] = ent.body.create(Body{ Vec2{ 0.0f, 2.0f }, BoxCollider{ Vec2{ 1.0f } }, false });
	ent.distanceJoint.create(DistanceJoint{ a, b, 3.0f, Vec2{ 0.2f }, Vec2{ 0.2f } });*/

	camera.zoom = 0.125f / 2.0f;
	camera.pos = Vec2{ 0.0f, 6.0f };
	Input::registerKeyButton(Keycode::G, GameButton::SELECT_GRAB_TOOL);
	Input::registerKeyButton(Keycode::Q, GameButton::SELECT_SELECT_TOOL);
	Input::registerKeyButton(Keycode::J, GameButton::START_SELECT_JOINT_TOOL);
	Input::registerKeyButton(Keycode::D, GameButton::SELECT_DISTANCE_JOINT_TOOL);
	Window::maximize();
	//gravity = Vec2{ 0.0f, 0.0f };
	gravity = Vec2{ 0.0f, -10.0f };
	
}

auto Game::saveLevel() const -> Json::Value {
	auto level = Json::Value::emptyObject();

	std::unordered_map<i32, i32> oldBodyIndexToNewIndex;

	auto toJson = [](Vec2 v) -> Json::Value {
		return { { "x", v.x }, { "y", v.y } };
	};

	level["gravity"] = toJson(gravity);

	{
		level["bodies"] = Json::Value::emptyArray();
		auto& bodies = level["bodies"].array();
		for (const auto& [id, body] : ent.body) {
			const auto newIndex = static_cast<i32>(bodies.size());
			oldBodyIndexToNewIndex[id.index()] = newIndex;

			auto colliderToLevelCollider = [](const Collider& collider) -> LevelCollider {
				return std::visit(overloaded{
					[](const CircleCollider& c) -> LevelCollider { return LevelCircle{ .radius = c.radius }; },
					[](const BoxCollider& c) -> LevelCollider { return LevelBox{ .size = c.size }; },
					[](const ConvexPolygon& c) -> LevelCollider { return LevelConvexPolygon{ .verts = c.verts }; },
				}, collider);
			};

			bodies.push_back(LevelBody{
				.pos = body.transform.pos,
				.orientation = body.transform.angle(),
				.vel = body.vel,
				.angularVel = body.angularVel,
				.mass = body.mass,
				.rotationalInertia = body.rotationalInertia,
				.coefficientOfFriction = body.coefficientOfFriction,
				.collider = colliderToLevelCollider(body.collider)
			}.toJson());
		}
	}

	{
		level["distanceJoints"] = Json::Value::emptyArray();
		auto& joints = level["distanceJoints"].array();
		for (const auto& [id, joint] : ent.distanceJoint) {
			joints.push_back(LevelDistanceJoint{
				.bodyAIndex = oldBodyIndexToNewIndex[joint.bodyA.index()],
				.bodyBIndex = oldBodyIndexToNewIndex[joint.bodyB.index()],
				.distance = joint.requiredDistance,
				.anchorA = joint.anchorOnA,
				.anchorB = joint.anchorOnB
			}.toJson());
		}
	}
	return level;
}

auto Game::saveLevelToFile(std::string_view path) -> void {
	std::ofstream file{ path.data() };
	const auto levelJson = saveLevel();
	Json::prettyPrint(file, levelJson);
}

auto Game::loadLevel(const Json::Value& level) -> bool {
	auto vec2FromJson = [](const Json::Value& json) -> Vec2 {
		return Vec2{ json.at("x").number(), json.at("y").number() };
	};

	// First loading into intermediate vectors so if the loading fails midway the old level is still loaded.
	Vec2 levelGravity;
	std::vector<LevelBody> levelBodies;
	std::vector<LevelDistanceJoint> levelDistanceJoints;
	try {
		levelGravity = vec2FromJson(level.at("gravity"));;
		{
			const auto& bodies = level.at("bodies").array();
			for (const auto& bodyJson : bodies) {
				levelBodies.push_back(LevelBody::fromJson(bodyJson));
			}
		}
		{
			const auto& distanceJoints = level.at("distanceJoints").array();
			for (const auto& jointJson : distanceJoints) {
				levelDistanceJoints.push_back(LevelDistanceJoint::fromJson(jointJson));
			}
		}
	} catch (const Json::JsonError&) {
		ASSERT_NOT_REACHED();
		goto error;
	}

	collisionSystem.reset();
	ent.reset();
	contacts.clear();

	gravity = levelGravity;

	for (const auto& levelBody : levelBodies) {
		const auto collider = std::visit(overloaded{
			[](const LevelCircle& c) -> Collider { return CircleCollider{.radius = c.radius }; },
			[](const LevelBox& c) -> Collider { return BoxCollider{.size = c.size }; },
			[](const LevelConvexPolygon& c) -> Collider { return ConvexPolygon{.verts = c.verts }; },
		}, levelBody.collider);
		const auto& [_, body] = ent.body.create(Body{ levelBody.pos, collider, false });
		body.transform.rot = Rotation{ levelBody.orientation };
		body.vel = levelBody.vel;
		body.angularVel = levelBody.angularVel;
		body.mass = levelBody.mass;
		body.rotationalInertia = levelBody.rotationalInertia;
		body.coefficientOfFriction = levelBody.coefficientOfFriction;
		body.updateInvMassAndInertia();
	}

	for (const auto& levelJoint : levelDistanceJoints) {
		const auto bodyA = ent.body.validate(levelJoint.bodyAIndex);
		const auto bodyB = ent.body.validate(levelJoint.bodyBIndex);
		if (!bodyA.has_value() || !bodyB.has_value()) {
			goto error;
		}
		ent.distanceJoint.create(DistanceJoint{
			.bodyA = *bodyA,
			.bodyB = *bodyB,
			.requiredDistance = levelJoint.distance,
			.anchorOnA = levelJoint.anchorA,
			.anchorOnB = levelJoint.anchorB,
		});
	}

	return true;

	error:
	dbg("failed to load level");
	return false;
}

auto Game::drawUi() -> void {
	using namespace ImGui;

	Begin("physics engine");
	Checkbox("update physics", &updatePhysics);
	if (!updatePhysics && Button("single step")) {
		doASingleStep = true;
	}
	Checkbox("show trajectory", &drawTrajectory);
	if (drawTrajectory) {
		SliderFloat2("initial velocity", initialVelocity.data(), -10.0f, 10.0f);
	}
	Checkbox("draw contacts", &drawContacts);
	if (drawContacts) {
		Checkbox("scale contact normals", &scaleContactNormals);
	}

	auto disableGravity = gravity == Vec2{ 0.0f };
	Checkbox("disableGravity", &disableGravity);
	if (disableGravity) {
		gravity = Vec2{ 0.0f };
	} else {
		gravity = Vec2{ 0.0f, -10.0f };
	}

	Checkbox("reuse previous contact accumulators", &reusePreviousFrameContactAccumulators);
	End();

	Begin("savestates");
	if (Button("make savesate")) {
		levelSavestates.push_back(saveLevel());
	}

	for (int i = static_cast<int>(levelSavestates.size()) - 1; i >= 0; i--) {
		if (Button(frameAllocator.format("load %d", i).data())) {
			[[maybe_unused]] const auto _ = loadLevel(levelSavestates[i]);
		}
		SameLine();
		if (Button("delete")) {
			levelSavestates.erase(levelSavestates.begin() + i);
		}
	}

	std::optional<const char*> errorModalMessage;
	if (BeginMainMenuBar()) {
		if (BeginMenu("file")) {
			if (MenuItem("open", "ctrl+o")) {
				const auto error = openLoadLevelDialog();
				if (error.has_value()) {
					errorModalMessage = *error;
				}
			}
			if (MenuItem("save", "ctrl+s")) {
				if (lastSavedLevelPath.has_value()) {
					saveLevelToFile(*lastSavedLevelPath);
				} else {
					openSaveLevelDialog();
				}
			}
			if (MenuItem("save as")) {
				openSaveLevelDialog();
			}
			EndMenu();
		}
		EndMainMenuBar();
	}

	openErrorPopupModal(errorModalMessage);

	End();

	
	if (Input::isButtonDown(GameButton::SELECT_GRAB_TOOL)) selectedTool = Tool::GRAB;
	if (Input::isButtonDown(GameButton::SELECT_SELECT_TOOL)) selectedTool = Tool::SELECT;

	if (Input::isButtonDown(GameButton::START_SELECT_JOINT_TOOL)) {
		selectingJointTool = true;
	} else if (selectingJointTool) {
		if (Input::isButtonDown(GameButton::SELECT_DISTANCE_JOINT_TOOL)) {
			selectedTool = Tool::DISTANCE_JOINT;
		} else if (Input::anyKeyPressed()) {
			selectingJointTool = false;
		}
	}

	
	

	Begin("tool");
	Combo("selected tool", reinterpret_cast<int*>(&selectedTool), "grab\0select\0distance joint\0revolute joint\0create body\0\0");

	switch (selectedTool) {
	case Game::Tool::GRAB: break;
	case Game::Tool::SELECT: selectToolGui(); break;
	case Game::Tool::DISTANCE_JOINT: break;
	case Game::Tool::REVOLUTE_JOINT: break;
	case Game::Tool::CREATE_BODY:
		Combo("shape", reinterpret_cast<int*>(&selectedShape), "circle\0rectangle\0\0");
		break;
	}

	End();


}

auto Game::openLoadLevelDialog() -> std::optional<const char*> {
	const auto path = ImGui::openFileSelect();
	if (!path.has_value()) {
		return std::nullopt;
	}

	const std::ifstream file{ path->data(), std::ios_base::binary };
	if (file.fail()) {
		return "failed to open file";
	} 

	std::stringstream buffer;
	buffer << file.rdbuf();

	const auto levelStr = buffer.str();
	Json::Value levelJson;
	try {
		levelJson = Json::parse(levelStr);
	} catch (const Json::ParsingError&) {
		return "failed to parse level";
	}
	if (!loadLevel(levelJson)) {
		return "failed to load level";
	}

	return std::nullopt;
}

auto Game::openSaveLevelDialog() -> void {
	if (const auto path = ImGui::openFileSave(); path.has_value()) {
		lastSavedLevelPath = path;
		saveLevelToFile(*path);
	}
}

auto Game::openErrorPopupModal(std::optional<const char*> message) -> void {
	using namespace ImGui;
	if (message.has_value()) {
		errorPopupModalMessage = *message;
		OpenPopup("error");
	}

	const auto center = GetMainViewport()->GetCenter();
	SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	if (BeginPopupModal("error", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		TextWrapped(errorPopupModalMessage);
		if (Button("ok", ImVec2(120, 0))) {
			CloseCurrentPopup();
		}
		SetItemDefaultFocus();
		EndPopup();
	}
}

auto Game::update() -> void {
	camera.aspectRatio = Window::aspectRatio();
	Vec2 dir{ 0.0f };
	if (Input::isKeyHeld(Keycode::W)) dir.y += 1.0f;
	if (Input::isKeyHeld(Keycode::S)) dir.y -= 1.0f;
	if (Input::isKeyHeld(Keycode::D)) dir.x += 1.0f;
	if (Input::isKeyHeld(Keycode::A)) dir.x -= 1.0f;
	camera.pos += dir.normalized() * Time::deltaTime() / camera.zoom;
	camera.scrollOnCursorPos();

	// For positions not not lag behind the camera has to be updated first.
	const auto mousePos = camera.screenSpaceToCameraSpace(Input::cursorPos());
	Debug::drawPoint(mousePos);

	ent.update();

	if (drawTrajectory) {
		Vec2 previous = mousePos;
		for (int i = 0; i < 50; i++) {
			float x = i / 20.0f / 16.0f / camera.zoom;
			Vec2 v = mousePos + Vec2{ x * initialVelocity.x, x * x * gravity.y / 2.0f + x * initialVelocity.y };
			if ((previous - v).lengthSq() < 0.001f)
				continue;

			const auto collision = collisionSystem.raycast(previous, v);
			if (collision.has_value()) {
				Debug::drawLine(previous, previous + ((v - previous) * collision->t));
				Debug::drawPoint(previous + ((v - previous) * collision->t));
				break;
			}
			Debug::drawLine(previous, v);
			previous = v;
		}
	}
	if (Input::isKeyDown(Keycode::U)) {
		const auto& [_, body] = ent.body.create(Body{ mousePos, BoxCollider{ Vec2{ 1.0f } }, false });
		body.vel = initialVelocity;
		body.angularVel = 1.5f;
		body.transform.rot = Rotation{ 0.0f };
	}

	if (Input::isKeyDown(Keycode::Z)) __debugbreak();

	drawUi();
	if (Input::isKeyDown(Keycode::X)) updatePhysics = !updatePhysics;
	if (Input::isKeyHeld(Keycode::CTRL) && Input::isKeyDown(Keycode::S)) {
		if (lastSavedLevelPath.has_value()) {
			saveLevelToFile(*lastSavedLevelPath);
		} else {
			openSaveLevelDialog();
		}
	}

	std::optional<const char*> errorMessage;
	if (Input::isKeyHeld(Keycode::CTRL) && Input::isKeyDown(Keycode::O)) {
		const auto error = openLoadLevelDialog();
		if (error.has_value()) {
			errorMessage = *error;
		}
	}
	openErrorPopupModal(errorMessage);

	if (Input::isMouseButtonDown(MouseButton::RIGHT)) {
		grabStart = mousePos;
	}

	if (grabStart.has_value() && Input::isMouseButtonUp(MouseButton::RIGHT)) {
		const auto size = (mousePos - *grabStart).applied(abs);
		const auto pos = (mousePos + *grabStart) / 2.0f;
		ent.body.create(Body{ pos, BoxCollider{ size }, 0.0f });
	}

	std::optional<BodyId> bodyUnderCursor;
	Vec2 bodyUnderCursorPosInSelectedObjectSpace;
	for (const auto [id, body] : ent.body) {
		if (contains(mousePos, body.transform.pos, body.transform.angle(), body.collider)) {
			bodyUnderCursor = id;
			bodyUnderCursorPosInSelectedObjectSpace = (mousePos - body.transform.pos) * body.transform.rot.inversed();
			break;
		}
	}

	if (selectedTool == Tool::GRAB) {
		if (bodyUnderCursor.has_value() && Input::isMouseButtonDown(MouseButton::LEFT)) {
			grabbed = bodyUnderCursor;
			grabPointInGrabbedObjectSpace = bodyUnderCursorPosInSelectedObjectSpace;
		}
		if (Input::isMouseButtonUp(MouseButton::LEFT)) {
			grabbed = std::nullopt;
		}

		if (grabbed.has_value()) {
			auto body = ent.body.get(*grabbed);
			if (body.has_value()) {
				const auto offsetUprightSpace = grabPointInGrabbedObjectSpace * body->transform.rot;
				const auto fromMouseToObject = mousePos - (body->transform.pos + offsetUprightSpace);
				body->force = -body->vel / (Time::deltaTime() * 5.0f) * body->mass;
				body->force += fromMouseToObject / pow(Time::deltaTime(), 2.0f) * body->mass / 10.0f;
				body->torque = det(offsetUprightSpace, body->force);
			} else {
				grabbed = std::nullopt;
			}
			
		}
	} else {
		grabbed = std::nullopt;
	}

	selectToolUpdate(bodyUnderCursor);

	if (selectedTool == Tool::DISTANCE_JOINT || selectedTool == Tool::REVOLUTE_JOINT) {
		if (bodyUnderCursor.has_value() && Input::isMouseButtonDown(MouseButton::LEFT)) {
			if (!distanceJointBodyA.has_value()) {
				distanceJointBodyA = bodyUnderCursor;
				distanceJointBodyAAnchor = bodyUnderCursorPosInSelectedObjectSpace;
			} else {
				const auto aId = *distanceJointBodyA;
				const auto bId = *bodyUnderCursor;
				const auto a = ent.body.get(aId);
				const auto b = ent.body.get(bId);
				const auto aAnchor = distanceJointBodyAAnchor;
				const auto bAnchor = bodyUnderCursorPosInSelectedObjectSpace;
				float distance = 0.0f;
				if (selectedTool == Tool::DISTANCE_JOINT) {
					distance = ::distance(a->transform.pos + aAnchor * a->transform.rot, b->transform.pos + bAnchor * b->transform.rot);
				}

				if (aId != bId && a.has_value() && b.has_value()) {
					const auto& [joint, _] = ent.distanceJoint.create(DistanceJoint{
						aId, bId,
						distance,
						aAnchor, bAnchor
					});
					distanceJointBodyA = std::nullopt;

					if (selectedTool == Tool::REVOLUTE_JOINT) {
						BodyPair bodyPair{ aId, bId };
						collisionSystem.collisionsToIgnore.insert(bodyPair);
						revoluteJointsWithIgnoredCollisions.push_back(std::pair{ joint, bodyPair });
					}
				}
			}
		}

		if (distanceJointBodyA.has_value()) {
			auto body = ent.body.get(*distanceJointBodyA);
			if (body.has_value()) {
				Debug::drawPoint(body->transform.pos + distanceJointBodyAAnchor * body->transform.rot, Vec3::RED);
			} else {
				distanceJointBodyA = std::nullopt;
			}
		}
	} else {
		distanceJointBodyA = std::nullopt;
	}

	std::erase_if(revoluteJointsWithIgnoredCollisions, [this](const auto& it) {
		const auto& [joint, bodyPair] = it;
		if (!ent.distanceJoint.isAlive(joint)) {
			collisionSystem.collisionsToIgnore.erase(bodyPair);
			return true;
		}
		return false;
	});

	if (selectedTool == Tool::CREATE_BODY && Input::isMouseButtonDown(MouseButton::LEFT)) {
		switch (selectedShape) {
		case Game::BodyShape::CIRCLE:
			ent.body.create(Body{ mousePos, CircleCollider{ 0.5f } });
			break;
		case Game::BodyShape::RECTANGLE:
			ent.body.create(Body{ mousePos, BoxCollider{ Vec2{ 1.0f } } });
			break;
		}
	}

	// The collisions system has to be updated because even if the physics isn't updated, because it registres new entities.
	collisionSystem.update();
	if (doASingleStep) {
		physicsStep();
		doASingleStep = false;
	} else if (updatePhysics) {
		physicsStep();
	}

	for (const auto& [_, body] : ent.body) {
		Debug::drawCollider(body.collider, body.transform.pos, body.transform.angle());
	}

	if (drawContacts) {
		for (const auto& [_, collision] : contacts) {
			for (i32 i = 0; i < collision.contactCount; i++) {
				const auto& contact = collision.contacts[i];
				const auto scale = scaleContactNormals ? contact.separation : 0.1f;
				Debug::drawRay(contact.pos, -collision.normal * scale, Vec3::RED);
			}
		}
	}

	switch (selectedTool) {
	case Game::Tool::GRAB:
		break;
	case Game::Tool::SELECT:
		selectToolDraw();
		break;
	case Game::Tool::DISTANCE_JOINT:
		break;
	case Game::Tool::CREATE_BODY:
		break;
	}

	for (const auto& [_, joint] : ent.distanceJoint) {
		const auto endpoints = joint.getEndpoints();
		Debug::drawRay(endpoints[0], (endpoints[1] - endpoints[0]).normalized() * joint.requiredDistance);
	}

	Renderer::update(camera);
}
auto Game::physicsStep() -> void {
	for (const auto [_, body] : ent.body) {
		if (body.isStatic())
			continue;

		// It might be better to use impulses instead of forces so they are independent of the time step.
		body.vel += (body.force * body.invMass + gravity) * Time::deltaTime();
		body.force = Vec2{ 0.0f };

		body.angularVel += body.torque * body.invRotationalInertia * Time::deltaTime();
		body.angularVel *= pow(angularDamping, Time::deltaTime());
		body.torque = 0.0f;
	}

	collisionSystem.detectCollisions(contacts);

	const auto invDeltaTime = 1.0f / Time::deltaTime();

	for (auto& [key, contact] : contacts) {
		auto a = ent.body.get(key.a);
		auto b = ent.body.get(key.b);
		// @Performance: this and the other loop.
		if (!a.has_value() || !b.has_value())
			continue;
		contact.preStep(*a, *b, invDeltaTime);
	}

	for (const auto& [_, joint] : ent.distanceJoint) {
		joint.preStep(invDeltaTime);
	}

	// TODO: Should the constraints be solved separately from collisions?
	for (int i = 0; i < 10; i++) {
		for (auto& [key, contact] : contacts) {
			auto a = ent.body.get(key.a);
			auto b = ent.body.get(key.b);
			if (!a.has_value() || !b.has_value())
				continue;
			contact.applyImpulse(*a, *b);
		}
		for (const auto& [_, joint] : ent.distanceJoint) {
			joint.applyImpluse();
		}
	}

	for (const auto [_, body] : ent.body) {
		if (body.isStatic())
			continue;
		body.transform.pos += body.vel * Time::deltaTime();
		body.transform.rot *= Rotation{ body.angularVel * Time::deltaTime() };
	}
}

auto Game::selectToolGui() -> void {
	using namespace ImGui;
	std::visit(overloaded{
		[&](const BodyId& bodyId) {
			auto body = ent.body.get(bodyId);
			if (!body.has_value())
				return;

			auto isStatic = body->isStatic();
			if (Checkbox("is static", &isStatic)) {
				if (isStatic) {
					body->makeStatic();
				} else {
					body->updateMass();
				}
			}
		},
		[&](const DistanceJointId& jointId) {
			auto joint = ent.distanceJoint.get(jointId);
			if (!joint.has_value())
				return;

			InputFloat("required distance", &joint->requiredDistance);
		}
	}, *selected);
}

auto Game::selectToolUpdate(const std::optional<BodyId>& bodyUnderCursor) -> void {
	if (selectedTool != Tool::SELECT) {
		selected = std::nullopt;
		return;
	} 

	if (Input::isKeyDown(Keycode::DEL)) {
		std::visit(overloaded{
			[&](const BodyId& body) { ent.body.destroy(body); },
			[&](const DistanceJointId& joint) { ent.distanceJoint.destroy(joint); }
		}, *selected);
		selected = std::nullopt;
	}

	if (Input::isMouseButtonDown(MouseButton::LEFT)) {
		selected = std::nullopt;
		for (const auto& [id, joint] : ent.distanceJoint) {
			const auto endpoints = joint.getEndpoints();;
			const auto jointCollider = LineSegment{ endpoints[0], endpoints[1] };
			const auto colliderThickness = 0.02f / camera.zoom;
			Debug::drawCircle(endpoints[0], colliderThickness);
			if (jointCollider.asCapsuleContains(colliderThickness, camera.cursorPos())) {
				selected = id;
				break;
			}
		}

		if (!selected.has_value() && bodyUnderCursor.has_value()) {
			selected = *bodyUnderCursor;
		}
	}
}

auto Game::selectToolDraw() -> void {
	if (!selected.has_value())
		return;

	std::visit(overloaded{
		[&](const BodyId& bodyId) {
			const auto body = ent.body.get(bodyId);
			if (!body.has_value())
				return;
			Debug::drawPoint(body->transform.pos, Vec3::RED);
		},
		[&](const DistanceJointId& jointId) {
			const auto joint = ent.distanceJoint.get(jointId);
			if (!joint.has_value())
				return;
			const auto endpoints = joint->getEndpoints();
			for (const auto& endpoint : endpoints) {
				Debug::drawPoint(endpoint, Vec3::RED);
			}
		}
	}, *selected);
}

bool Game::reusePreviousFrameContactAccumulators = true;
bool Game::positionCorrection = true;
bool Game::accumulateImpulses = true;