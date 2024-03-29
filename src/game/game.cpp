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
#include <utils/fileIo.hpp>

#include <sstream>
#include <utils/io.hpp>
#include <json/Json.hpp>
#include <fstream>

#include <game/demos/pyramidDemo.hpp>
#include <game/demos/doubleDominoDemo.hpp>
#include <game/demos/hexagonalPyramidDemo.hpp>
#include <game/demos/scissorsMechanismDemo.hpp>
#include <game/demos/cycloidDemo.hpp>
#include <game/demos/leaningTowerOfLireDemo.hpp>
#include <game/demos/theoJansenLinkageDemo.hpp>
#include <game/demos/testingDemo.hpp>
#include <game/gameSettingsManager.hpp>

#include <filesystem>

Game::Game() {
#define DEMO(type) demos.push_back(std::make_unique<type>());
	DEMO(PyramidDemo)
	DEMO(DoubleDominoDemo)
	DEMO(HexagonalPyramid)
	DEMO(ScissorsMechanism)
	DEMO(CycloidDemo)
	DEMO(LeaningTowerOfLireDemo)
	DEMO(TheoJansenLinkageDemo)
	DEMO(TestingDemo)
	std::sort(demos.begin(), demos.end(), [](const auto& a, const auto& b) { return strcmp(a->name(), b->name()) < 0; });
#undef DEMO
	Input::registerKeyButton(Keycode::G, GameButton::SELECT_GRAB_TOOL);
	Input::registerKeyButton(Keycode::Q, GameButton::SELECT_SELECT_TOOL);
	Input::registerKeyButton(Keycode::J, GameButton::START_SELECT_JOINT_TOOL);
	Input::registerKeyButton(Keycode::D, GameButton::SELECT_DISTANCE_JOINT_TOOL);
	Input::registerKeyButton(Keycode::SHIFT, GameButton::SNAP_TO_IMPORTANT_FEATURES);
	Input::registerKeyButton(Keycode::F, GameButton::FOCUS_SELECTED_ON_OBJECT);
	Window::maximize();
	
	bool loadedOldLevel = false;
	const auto& type = gameSettings.levelLoadedBeforeClosingType;
	if (type == "demo") {
		const auto& name = gameSettings.levelLoadedBeforeClosingInfo;
		for (auto& demo : demos) {
			if (demo->name() == name) {
				loadDemo(*demo.get());
				loadedOldLevel = true;
			}
		}
	} else if (type == "level_path") {
		const auto& path = gameSettings.levelLoadedBeforeClosingInfo;
		if (!loadLevelFromFile(path.data()).has_value()) {
			loadedOldLevel = true;
		}
	} 
	
	if (!loadedOldLevel) {
		ent.body.create(Body{ Vec2{ 0.0f, -50.0f }, BoxCollider{ Vec2{ 200.0f, 100.0f } }, true });
	}
	//physicsSubsteps = 10;
}

auto Game::saveLevel() const -> Json::Value {
	std::unordered_map<i32, i32> oldBodyIndexToNewIndex;

	Level level;
	level.gravity = gravity;

	for (const auto& [id, body] : ent.body) {
		// @Hack:
		if (abs(body.transform.pos.x) > 2000.0f || abs(body.transform.pos.y) > 2000.0f) {
			continue;
		}
		const auto newIndex = static_cast<i32>(level.bodies.size());
		oldBodyIndexToNewIndex[id.index()] = newIndex;

		auto colliderToLevelCollider = [](const Collider& collider) -> LevelCollider {
			return std::visit(overloaded{
				[](const CircleCollider& c) -> LevelCollider { return LevelCircle{ .radius = c.radius }; },
				[](const BoxCollider& c) -> LevelCollider { return LevelBox{ .size = c.size }; },
				[](const ConvexPolygon& c) -> LevelCollider { return LevelConvexPolygon{ .verts = c.verts }; },
			}, collider);
		};

		// TODO: Maybe check if the convex polygon collider has zero verts.

		level.bodies.push_back(LevelBody{
			.pos = body.transform.pos,
			.orientation = body.transform.angle(),
			.vel = body.vel,
			.angularVel = body.angularVel,
			.mass = body.mass,
			.rotationalInertia = body.rotationalInertia,
			.coefficientOfFriction = body.coefficientOfFriction,
			.collider = colliderToLevelCollider(body.collider)
		});
	}
		
	for (const auto& [id, joint] : ent.distanceJoint) {
		level.distanceJoints.push_back(LevelDistanceJoint{
			.bodyAIndex = oldBodyIndexToNewIndex[joint.bodyA.index()],
			.bodyBIndex = oldBodyIndexToNewIndex[joint.bodyB.index()],
			.distance = joint.requiredDistance,
			.anchorA = joint.anchorOnA,
			.anchorB = joint.anchorOnB
		});
	}

	for (const auto& joint : ent.revoluteJoint) {
		level.revoluteJoints.push_back(LevelRevoluteJoint{
			.bodyAIndex = oldBodyIndexToNewIndex[joint->bodyA.index()],
			.bodyBIndex = oldBodyIndexToNewIndex[joint->bodyB.index()],
			.anchorA = joint->localAnchorA,
			.anchorB = joint->localAnchorB,
			.motorSpeed = joint->motorSpeedInRadiansPerSecond,
		});
	}

	for (const auto& [id, trail] : ent.trail) {
		level.trails.push_back(LevelTrail{
			.bodyIndex = oldBodyIndexToNewIndex[trail.body.index()],
			.anchor = trail.anchor,
			.color = trail.color,
			.maxHistorySize = trail.maxHistorySize,
		});
	}

	for (const auto& ignoredCollision : ent.collisionsToIgnore) {
		level.ignoredCollisions.push_back(LevelIgnoredCollision{
			.bodyAIndex = oldBodyIndexToNewIndex[ignoredCollision.a.index()],
			.bodyBIndex = oldBodyIndexToNewIndex[ignoredCollision.b.index()],
		});
	}

	return level.toJson();
}

auto Game::saveLevelToFile(std::string_view path) -> void {
	std::ofstream file{ path.data() };
	const auto levelJson = saveLevel();
	Json::prettyPrint(file, levelJson);
	auto settings = gameSettings.saveAtScopeEnd();
	settings->levelLoadedBeforeClosingType = "level_path";
	settings->levelLoadedBeforeClosingInfo = path;
}

auto Game::loadLevel(const Json::Value& levelJson) -> bool {
	// First loading into intermediate type so if the loading fails midway the old level is still loaded.
	Level level;
	try {
		level = Level::fromJson(levelJson);
	} catch (const Json::JsonError&) {
		goto error;
	}

	resetLevel();

	gravity = level.gravity;

	for (const auto& levelBody : level.bodies) {
		const auto collider = std::visit(overloaded{
			[](const LevelCircle& c) -> Collider { return CircleCollider{ .radius = c.radius }; },
			[](const LevelBox& c) -> Collider { return BoxCollider{ .size = c.size }; },
			[](const LevelConvexPolygon& c) -> Collider { 
				ConvexPolygon polygon{ .verts = c.verts };
				polygon.calculateNormals();
				return polygon;  
			},
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

	for (const auto& levelJoint : level.distanceJoints) {
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

	for (const auto& levelJoint : level.revoluteJoints) {
		const auto bodyA = ent.body.validate(levelJoint.bodyAIndex);
		const auto bodyB = ent.body.validate(levelJoint.bodyBIndex);
		if (!bodyA.has_value() || !bodyB.has_value()) {
			goto error;
		}
		ent.revoluteJoint.create(RevoluteJoint{
			.bodyA = *bodyA,
			.bodyB = *bodyB,
			.localAnchorA = levelJoint.anchorA,
			.localAnchorB = levelJoint.anchorB,
			.motorSpeedInRadiansPerSecond = levelJoint.motorSpeed,
		});
	}

	for (const auto& levelTrail : level.trails) {
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

	for (const auto& ignoredCollision : level.ignoredCollisions) {
		const auto bodyA = ent.body.validate(ignoredCollision.bodyAIndex);
		const auto bodyB = ent.body.validate(ignoredCollision.bodyBIndex);
		if (!bodyA.has_value() || !bodyB.has_value()) {
			goto error;
		}
		ent.collisionsToIgnore.insert({ *bodyA, *bodyB });
	}
	afterLoad();

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

	const auto levelStr = readFileToString(file);
	Json::Value levelJson;
	try {
		levelJson = Json::parse(levelStr);
	} catch (const Json::ParsingError&) {
		return "failed to parse level";
	}
	if (!loadLevel(levelJson)) {
		return "failed to load level";
	}

	auto settings = gameSettings.saveAtScopeEnd();
	settings->levelLoadedBeforeClosingType = "level_path";
	settings->levelLoadedBeforeClosingInfo = path;
	return std::nullopt;
}

auto Game::loadDemo(Demo& demo) -> void {
	resetLevel();
	demo.load();
	afterLoad();
	loadedDemo = demo;
	auto settings = gameSettings.saveAtScopeEnd();
	settings->levelLoadedBeforeClosingType = "demo";
	settings->levelLoadedBeforeClosingInfo = loadedDemo->name();
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
	Checkbox("disable gravity", &disableGravity);
	if (disableGravity) {
		gravity = Vec2{ 0.0f };
	} else {
		gravity = Vec2{ 0.0f, -10.0f };
	}

	Checkbox("warm starting", &warmStarting);
	Checkbox("accumulate impulses", &accumulateImpulses);
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

	// https://github.com/ocornut/imgui/issues/331
	bool openGridModal = false;
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
		if (BeginMenu("grid")) {
			MenuItem("enabled", nullptr, &isGridEnabled);

			if (MenuItem("configure", nullptr, false, isGridEnabled)) {
				openGridModal = true;
			}
			EndMenu();
		}
		if (BeginMenu("snapping")) {
			MenuItem("to grid", nullptr, &snapToGrid);
			MenuItem("to objects", nullptr, &snapToObjects);
			EndMenu();
		}
		EndMainMenuBar();
	}

	if (openGridModal) {
		OpenPopup("grid");
	}
	const auto center = GetMainViewport()->GetCenter();
	SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	{
		// https://github.com/ocornut/imgui/issues/528
		bool isOpen = true;
		if (BeginPopupModal("grid", &isOpen)) {
			Checkbox("automatically scale", &automaticallyScaleGrid);
			if (automaticallyScaleGrid) BeginDisabled();
			InputFloat("cell size", &gridCellSize, 0.0f, 0.0f, "%.3f");
			if (automaticallyScaleGrid) EndDisabled();
			if (gridCellSize <= 0.0f) {
				gridCellSize = 0.01f;
			}
			EndPopup();
		}
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
			selectedJointType = JointType::DISTANCE;
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
	Combo("selected tool", reinterpret_cast<int*>(&selectedTool), "grab\0select\0create joint\0create body\0create line\0trail\0disable collision\0\0");

	switch (selectedTool) {
	case Game::Tool::GRAB: break;
	case Game::Tool::SELECT: selectToolGui(); break;
	case Game::Tool::CREATE_JOINT: {
		#include <game/jointTypeMacro.hpp>
		#define STRING(TYPE, typeString) typeString,
		static constexpr const char* jointNames[]{
			JOINT_TYPE_LIST(STRING)
		};
		#undef STRING
		#include <game/jointTypeMacroUndef.hpp>
		Combo("type", reinterpret_cast<int*>(&selectedJointType), jointNames, static_cast<int>(std::size(jointNames)));
		break;
	}
		
	case Game::Tool::CREATE_BODY:
		Combo("shape", reinterpret_cast<int*>(&selectedShape), "circle\0rectangle\0\0");
		switch (selectedShape)
		{
		case Game::BodyShape::CIRCLE:
			InputFloat("radius", &circleRadius);
			if (circleRadius < 0.05f) {
				circleRadius = 0.05f;
			}
			break;
		case Game::BodyShape::RECTANGLE:
			InputFloat2("size", boxSize.data());
			boxSize = boxSize.max(Vec2{ 0.05f });
			inputAngle("orientation", &boxOrientation);
			break;
		}
		break;
	case Game::Tool::CREATE_LINE:
		InputFloat("width", &lineWidth);
		if (lineWidth < 0.05f) {
			lineWidth = 0.05f;
		}
		Checkbox("static", &isLineStatic);
		Checkbox("endpoints inside", &endpointsInside);
		Checkbox("chain", &chainLine);
		// TODO: if chainLine { join with revolute joints }
		break;
	case Game::Tool::TRAIL: break;
	case Game::Tool::DISABLE_COLLISON: break;
	}

	End();

	displayErrorPopupModal();
}

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
	Debug::clearDebuggerScreen();
	camera.aspectRatio = Window::aspectRatio();
	camera.moveOnWasd();

	if (Input::isKeyDown(Keycode::F3)) {
		Renderer::drawImGui = !Renderer::drawImGui;
	}

	if (Input::isKeyDown(Keycode::R)) {
		loadDemo(*loadedDemo);
	}

	std::optional<float> gridSize;
	if (isGridEnabled) {
		if (automaticallyScaleGrid) {
			gridSize = powf(2.0, round(log2f(1.0f / camera.zoom / 25.0f)));
		} else {
			gridSize = gridCellSize;
		}
	}

	// For positions not not lag behind the camera has to be updated first.
	auto cursorPos = camera.screenSpaceToCameraSpace(Input::cursorPos());
	if (Input::isButtonHeld(GameButton::SNAP_TO_IMPORTANT_FEATURES)) {
		bool snappedToObject = false;
		if (snapToObjects) {
			float minDistance = std::numeric_limits<float>::infinity();
			Vec2 closestFeaturePos;
			for (const auto& [_, body] : ent.body) {
				const auto dist = distance(body.transform.pos, cursorPos);
				if (dist < minDistance) {
					minDistance = dist;
					closestFeaturePos = body.transform.pos;
				}

				auto convexPolygonCase = [&](Span<const Vec2> verts) {
					if (verts.size() <= 1)
						return;

					usize previous = verts.size() - 1;
					for (usize i = 0; i < verts.size(); previous = i, i++) {
						const auto cursorPosInColliderSpace = cursorPos * body.transform.inversed();
						const auto pointOnLine = LineSegment{ verts[previous], verts[i] }.closestPointTo(cursorPosInColliderSpace);
						const auto d = distance(pointOnLine, cursorPosInColliderSpace);
						if (d < minDistance) {
							minDistance = d;
							closestFeaturePos = pointOnLine * body.transform;
						}
					}
				};

				std::visit(overloaded{
					[&](const BoxCollider& box) {
						const auto& corners = box.getCorners(Transform::identity);
						convexPolygonCase(Span{ corners.data(), corners.size() });
					},
					[&](const CircleCollider& circle) {
						const auto centerToPos = cursorPos - body.transform.pos;
						const auto d = abs(centerToPos.length() - circle.radius);
						if (d < minDistance) {
							minDistance = d;
							closestFeaturePos = body.transform.pos + circle.radius * centerToPos.normalized();
						}
					},
					[&](const ConvexPolygon& polygon) {
						convexPolygonCase(polygon.verts);
					},
					}, body.collider);
			}
			//Debug::drawPoint(closestFeaturePos, Vec3::RED);
			//Debug::drawHollowCircle(cursorPos, 0.03f / camera.zoom);
			// TODO: Snapping to joints?
			if (minDistance < (0.03f / camera.zoom)) {
				cursorPos = closestFeaturePos;
				snappedToObject = true;
			}
		}

		if (!snappedToObject && snapToGrid && gridSize.has_value()) {
			cursorPos = (cursorPos / *gridSize).applied(round) * *gridSize;
		}
	}
	Debug::drawPoint(cursorPos);
	/*const auto s = 0.01f / camera.zoom;
	Debug::drawLine(cursorPos - Vec2{ s, 0.0f }, cursorPos + Vec2{ s, 0.0f });
	Debug::drawLine(cursorPos - Vec2{ 0.0f, s }, cursorPos + Vec2{ 0.0f, s });*/

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

	if (loadedDemo.has_value()) {
		DemoData demoData{
			.camera = camera,
			.cursorPos = cursorPos
		};
		loadedDemo->update(demoData);
	}

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
		// @Hack
		if (jointBodyA.has_value() && id == *jointBodyA) {
			continue;
		}

		if (selectedTool == Tool::GRAB && body.isStatic()) {
			continue;
		}

		if (contains(cursorPos, body.transform.pos, body.transform.angle(), body.collider)) {
			bodyUnderCursor = id;
			bodyUnderCursorPosInSelectedObjectSpace = (cursorPos - body.transform.pos) * body.transform.rot.inversed();
			break;
		}
	}

	bool scrollOnCursorPosEnabled = true;

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

	if (selectedTool == Tool::CREATE_JOINT) {
		if (bodyUnderCursor.has_value() && Input::isMouseButtonDown(MouseButton::LEFT)) {
			if (!jointBodyA.has_value()) {
				jointBodyA = bodyUnderCursor;
				jointBodyALocalAnchor = bodyUnderCursorPosInSelectedObjectSpace;
			} else {
				const auto aId = *jointBodyA;
				const auto bId = *bodyUnderCursor;
				const auto a = ent.body.get(aId);
				const auto b = ent.body.get(bId);
				const auto aAnchor = jointBodyALocalAnchor;
				const auto bAnchor = bodyUnderCursorPosInSelectedObjectSpace;
				float distance = ::distance(a->transform.pos + aAnchor * a->transform.rot, b->transform.pos + bAnchor * b->transform.rot);

				if (aId != bId && a.has_value() && b.has_value()) {
					switch (selectedJointType) {
					case Game::JointType::DISTANCE:
						ent.distanceJoint.create(DistanceJoint{
							aId, bId,
							distance,
							aAnchor, bAnchor
						});
						break;
					case Game::JointType::REVOLUTE:
						ent.revoluteJoint.create(RevoluteJoint{
							aId, bId,
							aAnchor, bAnchor
							});
						ent.collisionsToIgnore.insert(BodyPair{ aId, bId });
						break;
					case Game::JointType::SPRING:
						ent.springJoint.create(SpringJoint{
							aId, bId,
							distance,
							aAnchor, bAnchor
						});
						break;
					}
					jointBodyA = std::nullopt;
				}
			}
		}

		if (jointBodyA.has_value()) {
			auto body = ent.body.get(*jointBodyA);
			if (body.has_value()) {
				Debug::drawPoint(body->transform.pos + jointBodyALocalAnchor * body->transform.rot, Vec3::RED);
			} else {
				jointBodyA = std::nullopt;
			}
		}
	} else {
		jointBodyA = std::nullopt;
	}

	if (selectedTool == Tool::CREATE_BODY) {
		if (Input::isKeyHeld(Keycode::CTRL)) {
			scrollOnCursorPosEnabled = false;
			const auto scaling = pow(2.0f, 5.0f * Input::scrollDelta() * Time::deltaTime());
			switch (selectedShape) {
			case Game::BodyShape::CIRCLE:
				circleRadius *= scaling;
				break;
			case Game::BodyShape::RECTANGLE:
				boxSize *= scaling;
				break;
			}
		}

		if (Input::isMouseButtonDown(MouseButton::LEFT)) {
			switch (selectedShape) {
			case Game::BodyShape::CIRCLE:
				ent.body.create(Body{ cursorPos, CircleCollider{ circleRadius } });
				break;
			case Game::BodyShape::RECTANGLE:
				const auto& [_, body] = ent.body.create(Body{ cursorPos, BoxCollider{ boxSize } });
				body.transform.rot = Rotation{ boxOrientation };
				break;
			}
		}
	}

	if (selectedTool == Tool::CREATE_LINE) {
		if (Input::isMouseButtonDown(MouseButton::LEFT)) {
			if (!lineStart.has_value()) {
				lineStart = cursorPos;
			} else {
				const auto lineEnd = cursorPos;
				const auto line = lineEnd - *lineStart;
				const auto middle = (lineEnd + *lineStart) / 2.0f;
				auto length = line.length();
				if (endpointsInside) {
					length += lineWidth;
				}
				const auto& [_, body] = ent.body.create(Body{ middle, BoxCollider{ Vec2{ length, lineWidth } }, isLineStatic });
				body.transform.rot = Rotation{ line.angle() };
				if (chainLine) {
					lineStart = lineEnd;
				} else {
					lineStart = std::nullopt;
				}
			}
		}

		if (lineStart.has_value() && Input::isMouseButtonDown(MouseButton::RIGHT)) {
			lineStart = std::nullopt;
		}
	} else {
		lineStart = std::nullopt;
	}

	if (selectedTool == Tool::TRAIL && Input::isMouseButtonDown(MouseButton::LEFT) && bodyUnderCursor.has_value()) {
		ent.trail.create(Trail{ .body = *bodyUnderCursor, .anchor = bodyUnderCursorPosInSelectedObjectSpace });
	}

	if (selectedTool == Tool::DISABLE_COLLISON) {
		if (Input::isMouseButtonDown(MouseButton::LEFT)) {
			if (!disableCollisionBodyA.has_value()) {
				disableCollisionBodyA = bodyUnderCursor;
			} else if (bodyUnderCursor.has_value()) {
				ent.collisionsToIgnore.insert(BodyPair{ *disableCollisionBodyA, *bodyUnderCursor });
				disableCollisionBodyA = std::nullopt;
			}
		}
	} else {
		disableCollisionBodyA = std::nullopt;
	}

	ent.update();

	auto doPhysicsUpdate = updatePhysics || doASingleStep;
	if (doASingleStep) {
		doASingleStep = false;
	}

	// TODO: Find out what is setting the vel of static bodies.
	for (auto body : ent.body) {
		if (body->isStatic()) {
			body->angularVel = 0.0f;
			body->vel = Vec2{ 0.0f };
		}
	}

	// The collisions system has to be updated because even if the physics isn't updated, because it registres new entities.
	collisionSystem.update();
	if (doPhysicsUpdate) {
		physicsProfile = PhysicsProfile{};
		Timer timer;
		const auto substepLength = Time::deltaTime() / physicsSubsteps;
		for (i32 i = 0; i < physicsSubsteps; i++) {
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

			if (loadedDemo.has_value()) {
				loadedDemo->physicsStep();
			}

			physicsStep(substepLength, physicsSolverIterations, physicsProfile);
		}
		physicsProfile.total = timer.elapsedMilliseconds();
	}

	// Update trail after physics to get the most up to date position.
	for (const auto& [_, trail] : ent.trail) {
		trail.update();
	}

	if (scrollOnCursorPosEnabled) {
		camera.scrollOnCursorPos();
	}
	draw(cursorPos);

	Renderer::update(camera, gridSize);
}

#include <utils/array2d.hpp>

//auto scaleMatrix(Array2d<float>& m, const std::vector<float>& scale /* scaling matrix diagonal */) -> void {
//	if (m.size().x)
//}

//auto constructMatrix(const CollisionMap& contacts) -> Array2d<float> {
//	const auto rigidBodyStateVectorSize = 3;
//	const auto stateVectorSize = ent.body.aliveCount() * rigidBodyStateVectorSize;
//	std::vector<float> invMassMatrixDiagonal;
//	for (const auto& body : ent.body) {
//		invMassMatrixDiagonal.push_back(body->invMass);
//		invMassMatrixDiagonal.push_back(body->invMass);
//		invMassMatrixDiagonal.push_back(body->invRotationalInertia);
//	}
//	Array2d<float> jacobian{ stateVectorSize, stateVectorSize };
//	std::unordered_map<BodyId, int> bodyIdToIndex;
//	{
//		int index = 0;
//		for (const auto& body : ent.body) {
//			bodyIdToIndex[body.id] = index;
//			index++;
//		}
//	}
//	auto computeJacobian = [](const Body& a, const Body& b, const Collision& collision, int contactIndex) -> Array2d<float> {
//		Array2d<float> jacobian{ rigidBodyStateVectorSize * 2, 1 };
//		const auto& contact = collision.contacts[contactIndex];
//		jacobian(0, 0) = collision.normal.x;
//		jacobian(1, 0) = collision.normal.y;
//		const auto raDerivative = ((contact.pos - a.transform.pos) * a.angularVel).rotBy90deg();
//		jacobian(2, 0) = dot(collision.normal, raDerivative);
//
//		jacobian(3, 0) = -collision.normal.x;
//		jacobian(4, 0) = -collision.normal.y;
//		const auto rbDerivative = ((contact.pos - b.transform.pos) * b.angularVel).rotBy90deg();
//		jacobian(5, 0) = -dot(collision.normal, raDerivative);
//		return jacobian;
//	};
//
//	for (const auto& [bodyPair, collision] : contacts) {
//		for (i32 i = 0; i < collision.contactCount; i++) {
//			const auto jacobian = computeJacobian(*ent.body.get(bodyPair.a), *ent.body.get(bodyPair.b), collision, i);
//			
//		}
//	}
//}

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

		for (auto joint : ent.revoluteJoint) {
			joint->preStep(invDt);
		}

		for (auto joint : ent.springJoint) {
			joint->preStep(invDt);
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
			for (auto joint : ent.revoluteJoint) {
				joint->applyImpluse();
			}
			for (auto joint : ent.springJoint) {
				joint->applyImpluse();
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

auto Game::draw(Vec2 cursorPos) -> void {
	for (const auto& [_, body] : ent.body) {
		const auto color = body.isStatic() ? Vec3::WHITE / 2.0f : Vec3::WHITE;
		Debug::drawCollider(body.collider, body.transform.pos, body.transform.angle(), color);
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
	case Game::Tool::CREATE_JOINT:
		break;
	case Game::Tool::CREATE_BODY:
		switch (selectedShape) {
		case Game::BodyShape::CIRCLE:
			Debug::drawHollowCircle(cursorPos, circleRadius);
			break;
		case Game::BodyShape::RECTANGLE:
			Debug::drawBox(cursorPos, boxOrientation, boxSize);
			break;
		default:
			break;
		}
		break;
	case Game::Tool::CREATE_LINE:
		if (lineStart.has_value()) {
			const auto end = cursorPos;
			const auto center = (end + *lineStart) / 2.0f;
			const auto line = end - *lineStart;
			auto length = line.length();
			if (endpointsInside) {
				length += lineWidth;
			}
			Debug::drawBox(center, line.angle(), Vec2{ length, lineWidth });
		}
		break;
	case Game::Tool::TRAIL:
		break;
	case Game::Tool::DISABLE_COLLISON:
		break;
	}

	for (const auto& [_, joint] : ent.distanceJoint) {
		const auto endpoints = joint.getEndpoints();
		Debug::drawRay(endpoints[0], (endpoints[1] - endpoints[0]).normalized() * joint.requiredDistance);
	}

	for (const auto& joint : ent.revoluteJoint) {
		const auto anchors = joint->anchorsWorldSpace();
		Debug::drawPoint(anchors[0]);
		Debug::drawPoint(anchors[1]);
	}

	for (const auto& joint : ent.springJoint) {
		const auto a = ent.body.get(joint->bodyA);
		const auto b = ent.body.get(joint->bodyB);
		Debug::drawLine(a->transform.pos, b->transform.pos);
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
	selected = std::nullopt;
}

auto Game::afterLoad() -> void {
	// For the simulation to be deterministic and to make reloading demos deterministic this code needs to run before before the physics step. If it doesn't there is going to be one step in which collision isn't checked, because the bodies aren't registered in the collisionSystem and won't be untill the next frame, because then they will be accessible throught entitiesAddedThisFrame. So the bodies will get integrated without detecting collisions. 
	// One way to make sure this works is to call ent.update after all the functions that create entites, but it seems simpler to just call it right after loading a level.
	// Hopefully there aren't any errors in this logic.
	ent.update();
	collisionSystem.update();

	const auto NOT_IGNORED_MAX_AREA = 1000.0f;
	// Using optional because any amount of bodies in the loop can be ignored.
	std::optional<Aabb> levelAabb;
	for (auto body = ++ent.body.begin(); body != ent.body.end(); ++body) {
		const auto box = aabb(body->collider, body->transform);
		if (box.area() > NOT_IGNORED_MAX_AREA) {
			continue;
		}

		if (levelAabb.has_value()) {
			levelAabb = (*levelAabb).combined(box);
		} else {
			levelAabb = box;
		}
	}
	if (levelAabb.has_value()) {
		camera.fitAabbInView((*levelAabb).addedPadding(levelAabb->size().x / 2.0f));
	} else {
		camera.zoom = 0.125f / 2.0f;
		camera.pos = Vec2{ 0.0f, 6.0f };
	}
}

auto Game::selectToolGui() -> void {
	if (!selected.has_value())
		return;

	using namespace ImGui;
	
	// Pushing the index because without this swithing to a different object would preserve the values from the previous object. So you could click on one distance joint set it's length then click on another and it would also change it's length to the previous selected one's length.
	PushID(entityIdIndex(*selected));

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
		[&](const RevoluteJointId& jointId) {
			auto joint = ent.revoluteJoint.get(jointId);
			if (!joint.has_value())
				return;
			
			InputFloat("motor speed", &joint->motorSpeedInRadiansPerSecond);
		},
		[&](const SpringJointId& jointId) {
			auto joint = ent.springJoint.get(jointId);
			if (!joint.has_value())
				return;
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
	PopID();
}

auto Game::selectToolUpdate(Vec2 cursorPos, const std::optional<BodyId>& bodyUnderCursor) -> void {
	if (!selected.has_value()) {
		focusingOnSelected = false;
	}

	if (selectedTool != Tool::SELECT) {
		selected = std::nullopt;
		return;
	} 

	if (Input::isMouseButtonDown(MouseButton::LEFT)) {
		const auto colliderThickness = 0.02f / camera.zoom;
		auto select = [&]() -> std::optional<Entity> {
			for (const auto& [id, joint] : ent.distanceJoint) {
				const auto endpoints = joint.getEndpoints();
				const auto jointCollider = LineSegment{ endpoints[0], endpoints[1] };
				if (jointCollider.asCapsuleContains(colliderThickness, cursorPos)) {
					if (selected != Entity{ id }) {
						return id;
					}
				}
			}

			for (const auto& joint : ent.revoluteJoint) {
				const auto anchors = joint->anchorsWorldSpace();
				if (distance(anchors[0], cursorPos) <= colliderThickness) {
					if (selected != Entity{ joint.id }) {
						return joint.id;
					}
				}
			}

			for (const auto& trail : ent.trail) {
				if (trail->history.size() > 0 && distance(trail->history.back(), cursorPos) < colliderThickness) {
					if (selected != Entity{ trail.id }) {
						return trail.id;
					}
					return trail.id;
				}
			}

			if (bodyUnderCursor.has_value()) {
				return bodyUnderCursor;
			}

			return std::nullopt;
		};
		selected = select();
	}

	if (!selected.has_value()) {
		return;
	}

	if (!entityIsAlive(*selected)) {
		selected = std::nullopt;
		return;
	}

	if (Input::isKeyDown(Keycode::DEL)) {
		entityDestroy(*selected);
		selected = std::nullopt;
		return;
	}

	// Don't think that focusing on a moving object should be considered a bug. Although the aabb changing due to rotation might be considered a bug. Could have a flag when the zoom reaches the desired value. The required aabb might still change due to rotation so it might be good to use the maximum possible aabb. This would require making a new function. For boxes this should be the diagonal, circles are easy. In general it needs to find the 2 points on the shape that are furthest apart from eachother.
	// Also don't know if swithing selected entites should disable the focus.
	if (const auto userMovedCamera = lastFrameFocusPos != camera.pos) {
		focusingOnSelected = false;
	}

	if (Input::isButtonDown(GameButton::FOCUS_SELECTED_ON_OBJECT)) {
		focusingOnSelected = !focusingOnSelected;
		elapsedSinceFocusStart = 0.0f;
	}

	if (focusingOnSelected) {
		auto selectedEntityAabb = std::visit(overloaded{
			[&](const BodyId& bodyId) -> std::optional<Aabb> {
				const auto body = ent.body.get(bodyId);
				if (!body.has_value())
					return std::nullopt;
				// @Hack: ignore rotation so the aabb doesn't change a lot.
				return aabb(body->collider, Transform{ body->transform.pos, 0.0f });
			},
			[&](const DistanceJointId& jointId) -> std::optional<Aabb> {
				const auto joint = ent.distanceJoint.get(jointId);
				if (!joint.has_value())
					return std::nullopt;
				const auto endpoints = joint->getEndpoints();
				return Aabb::fromCorners(endpoints[0], endpoints[1]);
			},
			[&](const RevoluteJointId&) -> std::optional<Aabb> {
				return std::nullopt;
			},
			[&](const SpringJointId&) -> std::optional<Aabb> {
				return std::nullopt;
			},
			[&](const TrailId& traildId) -> std::optional<Aabb> {
				const auto trail = ent.trail.get(traildId);
				if (!trail.has_value())
					return std::nullopt;

				if (trail->history.size() == 0)
					return std::nullopt;
				
				const auto p = trail->history.back();
				float s = 1.0f;
				return Aabb{ p - Vec2{ s }, p + Vec2{ s } };
			}
		}, *selected);

		if (selectedEntityAabb.has_value()) {
			elapsedSinceFocusStart = std::min(elapsedSinceFocusStart + Time::deltaTime(), 1.0f);
			const auto size = selectedEntityAabb->size() * 6.0f;
			const auto targetZoom = 1.0f / std::max(size.y, camera.heightIfWidthIs(size.x));

			const auto [pos, zoom] = lerpPosAndZoom({ camera.pos, camera.zoom }, { selectedEntityAabb->center(), targetZoom }, elapsedSinceFocusStart);
			camera.pos = pos;
			camera.zoom = zoom;

			lastFrameFocusPos = camera.pos;
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
		},
		[&](const RevoluteJointId& jointId) {
			const auto joint = ent.revoluteJoint.get(jointId);
			if (!joint.has_value())
				return;
			const auto anchors = joint->anchorsWorldSpace();
			Debug::drawPoint(anchors[0], Vec3::RED);
		},
		[&](const SpringJointId& jointId) {
			const auto joint = ent.springJoint.get(jointId);
			if (!joint.has_value())
				return;
			/*const auto anchors = joint->anchorsWorldSpace();
			Debug::drawPoint(anchors[0], Vec3::RED);*/
		},
		[&](const TrailId& traildId) {
			const auto trail = ent.trail.get(traildId);
			if (!trail.has_value())
				return;
			return;
		}

	}, *selected);
}

bool Game::warmStarting = true;
bool Game::accumulateImpulses = true;
