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
#include <utils/timer.hpp>
#include <game/levelFormat/levelData.hpp>
#include <utils/overloaded.hpp>
#include <engine/frameAllocator.hpp>
#include <customImguiWidgets.hpp>

#include <sstream>
#include <utils/io.hpp>
#include <json/Json.hpp>
#include <fstream>

#include <game/demos/pyramidDemo.hpp>
#include <game/demos/doubleDominoDemo.hpp>
#include <game/demos/hexagonalPyramidDemo.hpp>
#include <game/demos/scissorsMechanismDemo.hpp>

#include <filesystem>

Game::Game() {
#define DEMO(type) demos.push_back(std::make_unique<type>());
	DEMO(PyramidDemo)
	DEMO(DoubleDominoDemo)
	DEMO(HexagonalPyramid)
	DEMO(ScissorsMechanism)
	std::sort(demos.begin(), demos.end(), [](const auto& a, const auto& b) { return strcmp(a->name(), b->name()) < 0; });
#undef DEMO

	camera.zoom = 0.125f / 2.0f;
	camera.pos = Vec2{ 0.0f, 6.0f };
	Input::registerKeyButton(Keycode::G, GameButton::SELECT_GRAB_TOOL);
	Input::registerKeyButton(Keycode::Q, GameButton::SELECT_SELECT_TOOL);
	Input::registerKeyButton(Keycode::J, GameButton::START_SELECT_JOINT_TOOL);
	Input::registerKeyButton(Keycode::D, GameButton::SELECT_DISTANCE_JOINT_TOOL);
	Input::registerKeyButton(Keycode::SHIFT, GameButton::SNAP_TO_IMPORTANT_FEATURES);
	Window::maximize();

	std::ifstream lastLoadedInfo{ lastLoadedLevelInfoPath };
	std::string type;
	std::getline(lastLoadedInfo, type);
	bool loadedOldLevel = false;
	if (type == "demo") {
		std::string name;
		std::getline(lastLoadedInfo, name);
		for (auto& demo : demos) {
			if (demo->name() == name) {
				loadDemo(*demo.get());
				loadedOldLevel = true;
			}
		}
	} else if (type == "level_path") {
		std::string path;
		std::getline(lastLoadedInfo, path);
		if (!loadLevelFromFile(path.data()).has_value()) {
			loadedOldLevel = true;
		}
	} 

	if (!loadedOldLevel) {
		ent.body.create(Body{ Vec2{ 0.0f, -50.0f }, BoxCollider{ Vec2{ 200.0f, 100.0f } }, true });
	}
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

	{
		auto& trails = (level["trails"] = Json::Value::emptyArray()).array();
		for (const auto& [id, trail] : ent.trail) {
			trails.push_back(LevelTrail{
				.bodyIndex = oldBodyIndexToNewIndex[trail.body.index()],
				.anchor = trail.anchor,
				.color = trail.color,
				.maxHistorySize = trail.maxHistorySize,
			}.toJson());
		}
	}

	{
		auto& ignoredCollisions = (level["ignoredCollisions"] = Json::Value::emptyArray()).array();
		for (const auto& ignoredCollision : ent.collisionsToIgnore) {
			ignoredCollisions.push_back(LevelIgnoredCollision{
				.bodyAIndex = oldBodyIndexToNewIndex[ignoredCollision.a.index()],
				.bodyBIndex = oldBodyIndexToNewIndex[ignoredCollision.b.index()],
			}.toJson());
		}
	}
	return level;
}

auto Game::saveLevelToFile(std::string_view path) -> void {
	std::ofstream file{ path.data() };
	const auto levelJson = saveLevel();
	Json::prettyPrint(file, levelJson);
	std::ofstream info{ lastLoadedLevelInfoPath };
	info << "level_path\n";
	info << path;
}

auto Game::loadLevel(const Json::Value& level) -> bool {
	auto vec2FromJson = [](const Json::Value& json) -> Vec2 {
		return Vec2{ json.at("x").number(), json.at("y").number() };
	};

	// First loading into intermediate vectors so if the loading fails midway the old level is still loaded.
	Vec2 levelGravity;
	std::vector<LevelBody> levelBodies;
	std::vector<LevelDistanceJoint> levelDistanceJoints;
	std::vector<LevelIgnoredCollision> levelIgnoredCollisions;
	std::vector<LevelTrail> levelTrails;
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
		{
			const auto& ignoredCollisions = level.at("ignoredCollisions").array();
			for (const auto& ignoredCollision : ignoredCollisions) {
				levelIgnoredCollisions.push_back(LevelIgnoredCollision::fromJson(ignoredCollision));
			}
		}

		if (level.contains("trails")) {
			const auto& trails = level.at("trails").array();
			for (const auto& trail : trails) {
				levelTrails.push_back(LevelTrail::fromJson(trail));
			}
		}
	} catch (const Json::JsonError&) {
		goto error;
	}

	resetLevel();

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

	for (const auto& levelTrail : levelTrails) {
		const auto body = ent.body.validate(levelTrail.bodyIndex);
		if (!body.has_value()) {
			goto error;
		}
		ent.trail.create(Trail{
			.body = *body,
			.anchor = levelTrail.anchor,
			.color = levelTrail.color,
			.maxHistorySize = levelTrail.maxHistorySize,
		});
	}

	for (const auto& ignoredCollision : levelIgnoredCollisions) {
		const auto bodyA = ent.body.validate(ignoredCollision.bodyAIndex);
		const auto bodyB = ent.body.validate(ignoredCollision.bodyBIndex);
		if (!bodyA.has_value() || !bodyB.has_value()) {
			goto error;
		}
		ent.collisionsToIgnore.insert({ *bodyA, *bodyB });
	}

	return true;

	error:
	dbg("failed to load level");
	return false;
}

auto Game::loadLevelFromFile(const char* path) -> std::optional<const char*> {
	const std::ifstream file{ path, std::ios_base::binary };
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

	std::ofstream info{ lastLoadedLevelInfoPath };
	info << "level_path\n";
	info << path;
	return std::nullopt;
}

auto Game::loadDemo(Demo& demo) -> void {
	resetLevel();
	demo.load();
	loadedDemo = demo;
	std::ofstream info{ lastLoadedLevelInfoPath };
	info << "demo\n";
	info << loadedDemo->name();
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
	InputInt("solver iterations", &physicsSolverIterations);
	InputInt("physics substeps", &physicsSubsteps);
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
	End();


	if (BeginMainMenuBar()) {
		if (BeginMenu("file")) {
			if (MenuItem("new")) {
				resetLevel();
				ent.body.create(Body{ Vec2{ 0.0f, -50.0f }, BoxCollider{ Vec2{ 200.0f, 100.0f } }, true });
			}
			if (MenuItem("open", "ctrl+o")) {
				const auto error = openLoadLevelDialog();
				if (error.has_value()) {
					openErrorPopupModal(*error);
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
		if (BeginMenu("demos")) {
			for (auto& demo : demos) {
				if (MenuItem(demo->name())) {
					loadDemo(*demo.get());
				}
			}

			EndMenu();
		}
		EndMainMenuBar();
	}


	if (loadedDemo.has_value()) {
		Begin("demo");
		if (TreeNode("load settings")) {
			loadedDemo->loadSettingsGui();
			if (Button("reload")) {
				loadDemo(*loadedDemo);
			}
			TreePop();
		}
		if (TreeNode("runtime settings")) {
			loadedDemo->settingsGui();
			TreePop();
		}
		End();
	}
	
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

	{
		Begin("physics profile");
		const auto updateRate = round(1.0f / Time::deltaTime());
		const auto hz = static_cast<int>(updateRate * physicsSubsteps);
		Text("physics running at %dhz", hz);
		Combo("units", &profileUnitsIndex, "ms\0delta time %\0physics total time %\0\0");
		if (BeginTable("profile table", 2, ImGuiTableFlags_Borders)) {
			auto rowIndex = 0;
			auto row = [&](const char* name, float time) {
				TableNextRow();
				TableSetColumnIndex(0);
				Text(name);
				TableSetColumnIndex(1);
				if (profileUnitsIndex == 0) {
					Text("%.2fms", time);
				} else if (profileUnitsIndex == 1) {
					const auto toMs = 1000.0f;
					Text("%.2f", time / (Time::deltaTime() * toMs));
				} else {
					Text("%.2f", time / physicsProfile.total);
				}
				rowIndex++;
			};
			row("collideTotal", physicsProfile.collideTotal);
			row("colliderUpdateBvh", physicsProfile.collideUpdateBvh);
			row("collideDetectCollisions", physicsProfile.collideDetectCollisions);
			row("solveTotal", physicsProfile.solveTotal);
			row("solvePrestep", physicsProfile.solvePrestep);
			row("solveVelocities", physicsProfile.solveVelocities);
			row("total", physicsProfile.total);

			EndTable();
		}
		End();
	}

	Begin("tool");
	Combo("selected tool", reinterpret_cast<int*>(&selectedTool), "grab\0select\0distance joint\0revolute joint\0create body\0trail\0\0");

	switch (selectedTool) {
	case Game::Tool::GRAB: break;
	case Game::Tool::SELECT: selectToolGui(); break;
	case Game::Tool::DISTANCE_JOINT: break;
	case Game::Tool::REVOLUTE_JOINT: break;
	case Game::Tool::CREATE_BODY:
		Combo("shape", reinterpret_cast<int*>(&selectedShape), "circle\0rectangle\0\0");
		break;
	case Game::Tool::TRAIL: break;
	}

	End();

	displayErrorPopupModal();
}

#include <filesystem>

auto Game::openLoadLevelDialog() -> std::optional<const char*> {
	const auto path = ImGui::openFileSelect();
	if (!path.has_value()) {
		return std::nullopt;
	}
	return loadLevelFromFile(path->data());
}

auto Game::openSaveLevelDialog() -> void {
	if (const auto path = ImGui::openFileSave(); path.has_value()) {
		lastSavedLevelPath = path;
		saveLevelToFile(*path);
	}
}

auto Game::openErrorPopupModal(const char* message) -> void {
	errorPopupModalMessage = message;
	openErrorPopup = true;
}

auto Game::displayErrorPopupModal() -> void {
	using namespace ImGui;
	if (openErrorPopup) {
		OpenPopup("error");
		openErrorPopup = false;
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
	auto cursorPos = camera.screenSpaceToCameraSpace(Input::cursorPos());
	Debug::drawPoint(cursorPos);

	ent.update();

	if (drawTrajectory) {
		Vec2 previous = cursorPos;
		for (int i = 0; i < 50; i++) {
			float x = i / 20.0f / 16.0f / camera.zoom;
			Vec2 v = cursorPos + Vec2{ x * initialVelocity.x, x * x * gravity.y / 2.0f + x * initialVelocity.y };
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
		const auto& [_, body] = ent.body.create(Body{ cursorPos, BoxCollider{ Vec2{ 1.0f } }, false });
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

	if (Input::isKeyHeld(Keycode::CTRL) && Input::isKeyDown(Keycode::O)) {
		const auto error = openLoadLevelDialog();
		if (error.has_value()) {
			openErrorPopupModal(*error);
		}
	}

	if (Input::isMouseButtonDown(MouseButton::RIGHT)) {
		grabStart = cursorPos;
	}

	if (grabStart.has_value() && Input::isMouseButtonUp(MouseButton::RIGHT)) {
		const auto size = (cursorPos - *grabStart).applied(abs);
		const auto pos = (cursorPos + *grabStart) / 2.0f;
		ent.body.create(Body{ pos, BoxCollider{ size }, 0.0f });
	}

	std::optional<BodyId> bodyUnderCursor;
	Vec2 bodyUnderCursorPosInSelectedObjectSpace;
	for (const auto [id, body] : ent.body) {
		if (contains(cursorPos, body.transform.pos, body.transform.angle(), body.collider)) {
			bodyUnderCursor = id;
			bodyUnderCursorPosInSelectedObjectSpace = (cursorPos - body.transform.pos) * body.transform.rot.inversed();
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
				const auto fromMouseToObject = cursorPos - (body->transform.pos + offsetUprightSpace);
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

	selectToolUpdate(cursorPos, bodyUnderCursor);

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
						ent.collisionsToIgnore.insert(bodyPair);
						ent.revoluteJointsWithIgnoredCollisions.push_back(std::pair{ joint, bodyPair });
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

	if (selectedTool == Tool::CREATE_BODY && Input::isMouseButtonDown(MouseButton::LEFT)) {
		switch (selectedShape) {
		case Game::BodyShape::CIRCLE:
			ent.body.create(Body{ cursorPos, CircleCollider{ 0.5f } });
			break;
		case Game::BodyShape::RECTANGLE:
			ent.body.create(Body{ cursorPos, BoxCollider{ Vec2{ 1.0f } } });
			break;
		}
	}

	if (selectedTool == Tool::TRAIL && Input::isMouseButtonDown(MouseButton::LEFT) && bodyUnderCursor.has_value()) {
		ent.trail.create(Trail{ .body = *bodyUnderCursor, .anchor = bodyUnderCursorPosInSelectedObjectSpace });
	}

	auto doPhysicsUpdate = updatePhysics || doASingleStep;
	if (doASingleStep) {
		doASingleStep = false;
	}

	// The collisions system has to be updated because even if the physics isn't updated, because it registres new entities.
	collisionSystem.update();
	if (doPhysicsUpdate) {
		physicsProfile = PhysicsProfile{};
		Timer timer;
		const auto substepLength = Time::deltaTime() / physicsSubsteps;
		for (i32 i = 0; i < physicsSubsteps; i++) {
			physicsStep(substepLength, physicsSolverIterations, physicsProfile);

			if (selectedTool == Tool::GRAB && grabbed.has_value()) {
				auto body = ent.body.get(*grabbed);
				if (body.has_value()) {
					const auto offsetUprightSpace = grabPointInGrabbedObjectSpace * body->transform.rot;
					const auto fromMouseToObject = cursorPos - (body->transform.pos + offsetUprightSpace);
					body->force = -body->vel / (Time::deltaTime() * 2.0f) * body->mass;
					body->force += fromMouseToObject / pow(Time::deltaTime(), 2.0f) * body->mass / 10.0f;
					body->torque = det(offsetUprightSpace, body->force);
				} else {
					grabbed = std::nullopt;
				}
			}
		}
		physicsProfile.total = timer.elapsedMilliseconds();

		// Update trail after physics to get the most up to date position.
		for (const auto& [_, trail] : ent.trail) {
			trail.update();
		}
	}

	draw();

	Renderer::update(camera);
}
auto Game::physicsStep(float dt, i32 solverIterations, PhysicsProfile& profile) -> void {
	for (const auto [_, body] : ent.body) {
		if (body.isStatic())
			continue;

		// It might be better to use impulses instead of forces so they are independent of the time step.
		body.vel += (body.force * body.invMass + gravity) * dt;
		body.force = Vec2{ 0.0f };

		body.angularVel += body.torque * body.invRotationalInertia * dt;
		body.angularVel *= pow(angularDamping, dt);
		body.torque = 0.0f;
	}

	{
		Timer timerCollision;
		{
			Timer timer;
			collisionSystem.updateBvh();
			profile.collideUpdateBvh = timer.elapsedMilliseconds();
		}
		{
			Timer timer;
			collisionSystem.detectCollisions(contacts, ent.collisionsToIgnore);
			profile.collideDetectCollisions = timer.elapsedMilliseconds();
		}
		profile.collideTotal += timerCollision.elapsedMilliseconds();
	}

	const auto invDt = 1.0f / dt;

	Timer solveTimer;
	{
		Timer timer;
		for (auto& [key, contact] : contacts) {
			auto a = ent.body.get(key.a);
			auto b = ent.body.get(key.b);
			// @Performance: this and the other loop.
			if (!a.has_value() || !b.has_value())
				continue;
			contact.preStep(*a, *b, invDt);
		}

		for (const auto& [_, joint] : ent.distanceJoint) {
			joint.preStep(invDt);
		}
		profile.solvePrestep += timer.elapsedMilliseconds();
	}

	{
		Timer timer;
		// TODO: Should the constraints be solved separately from collisions?
		for (int i = 0; i < solverIterations; i++) {
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
		profile.solveVelocities = timer.elapsedMilliseconds();
	}
	profile.solveTotal += solveTimer.elapsedMilliseconds();

	for (const auto [_, body] : ent.body) {
		if (body.isStatic())
			continue;
		body.transform.pos += body.vel * dt;
		body.transform.rot *= Rotation{ body.angularVel * dt };
	}
}

auto Game::draw() -> void {
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
	case Game::Tool::REVOLUTE_JOINT:
		break;
	case Game::Tool::CREATE_BODY:
		break;
	case Game::Tool::TRAIL:
		break;
	}

	for (const auto& [_, joint] : ent.distanceJoint) {
		const auto endpoints = joint.getEndpoints();
		Debug::drawRay(endpoints[0], (endpoints[1] - endpoints[0]).normalized() * joint.requiredDistance);
	}

	for (const auto& [_, trail] : ent.trail) {
		trail.draw();
	}
}

auto Game::resetLevel() -> void {
	collisionSystem.reset();
	ent.reset();
	contacts.clear();
	gravity = Vec2{ 0.0f, -10.0f };
	loadedDemo = std::nullopt;
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
		},
		[&](const TrailId& trailId) {
			auto trail = ent.trail.get(trailId);
			if (!trail.has_value())
				return;

			ColorEdit3("color", trail->color.data());
			InputInt("max history size", &trail->maxHistorySize);
			InputFloat2("anchor", trail->anchor.data());
			if (Button("clear") && trail->history.size() != 0) {
				const auto last = trail->history.back();
				trail->history.clear();
				trail->history.push_back(last);
			}
		}
	}, *selected);
}

auto Game::selectToolUpdate(Vec2 cursorPos, const std::optional<BodyId>& bodyUnderCursor) -> void {
	if (selectedTool != Tool::SELECT) {
		selected = std::nullopt;
		return;
	} 

	if (Input::isKeyDown(Keycode::DEL)) {
		std::visit(overloaded{
			[&](const BodyId& body) { ent.body.destroy(body); },
			[&](const DistanceJointId& joint) { ent.distanceJoint.destroy(joint); },
			[&](const TrailId& trail) { ent.trail.destroy(trail); }
		}, *selected);
		selected = std::nullopt;
	}

	if (Input::isMouseButtonDown(MouseButton::LEFT)) {
		const auto colliderThickness = 0.02f / camera.zoom;
		selected = std::nullopt;
		auto select = [&]() -> std::optional<Entity> {
			for (const auto& [id, joint] : ent.distanceJoint) {
				const auto endpoints = joint.getEndpoints();
				const auto jointCollider = LineSegment{ endpoints[0], endpoints[1] };
				if (jointCollider.asCapsuleContains(colliderThickness, cursorPos)) {
					return id;
				}
			}

			for (const auto& [id, trail] : ent.trail) {
				if (trail.history.size() > 0 && distance(trail.history.back(), cursorPos) < colliderThickness) {
					return id;
				}
			}

			if (bodyUnderCursor.has_value()) {
				return bodyUnderCursor;
			}

			return std::nullopt;
		};
		selected = select();
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
		},
		[&](const TrailId& traildId) {
			const auto trail = ent.trail.get(traildId);
			if (!trail.has_value())
				return;
			return;
		}

	}, *selected);
}

bool Game::reusePreviousFrameContactAccumulators = true;
bool Game::positionCorrection = true;
bool Game::accumulateImpulses = true;