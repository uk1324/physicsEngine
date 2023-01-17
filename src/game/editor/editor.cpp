#include <game/editor/editor.hpp>
#include <game/debug.hpp>
#include <imgui/imgui.h>
#include <game/editor/input.hpp>
#include <game/collision/collision.hpp>
#include <game/levelFormat.hpp>
#include <engine/time.hpp>
#include <engine/frameAllocator.hpp>
#include <math/lineSegment.hpp>
#include <math/mat2.hpp>
#include <math/utils.hpp>
#include <engine/window.hpp>
#include <utils/io.hpp>
#include <utils/overloaded.hpp>
#include <../thirdParty/json/Json.hpp>

#include <fstream>

Editor::Editor() {
	camera.zoom = 0.125f / 2.0f;
	camera.pos = Vec2{ 0.0f, 0.0f };
	registerInputButtons();

	{
		std::ifstream level("./levels/test");
		std::stringstream buffer;
		buffer << level.rdbuf();
		try {
			const auto value = Json::parse(buffer.str());
			loadLevel(value);
		} catch (const Json::ParsingError&) {
			dbg("failed to load level");
		}
	}
}

auto Editor::registerInputButtons() -> void {
	Input::registerKeyButton(Keycode::F, EditorButton::FOCUS);
	Input::registerKeyButton(Keycode::CTRL, EditorButton::GIZMO_GRID_SNAP);
	Input::registerKeyButton(Keycode::SHIFT, EditorButton::SNAP_CURSOR_TO_SHAPE_FEATURES);
}

template<typename T>
auto sortedInsert(std::vector<T>& vec, const T& item) -> void {
	if (vec.empty()) {
		vec.push_back(item);
		return;
	} 

	for (usize i = 0; i <= vec.size(); i++) {
		if (i == vec.size()) {
			vec.push_back(item);
			break;
		} 
		
		if (item > vec[i]) {
			vec.insert(vec.begin() + i, item);
			break;
		}
	}
}

#include <filesystem>

auto Editor::update(Gfx& gfx, Renderer& renderer) -> void {
	using namespace ImGui;

	DockSpaceOverViewport(GetMainViewport());

	//if (ImGui::BeginMainMenuBar())
	//{
	//	if (ImGui::BeginMenu("File"))
	//	{
	//		ImGui::EndMenu();
	//	}
	//	if (ImGui::BeginMenu("Edit"))
	//	{
	//		if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
	//		if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {}  // Disabled item
	//		ImGui::Separator();
	//		if (ImGui::MenuItem("Cut", "CTRL+X")) {}
	//		if (ImGui::MenuItem("Copy", "CTRL+C")) {}
	//		if (ImGui::MenuItem("Paste", "CTRL+V")) {}
	//		ImGui::EndMenu();
	//	}
	//	ImGui::EndMainMenuBar();
	//}

	//bool openLevelsModal = false;

	//ImGui::ShowDemoWindow();

	//if (ImGui::BeginMainMenuBar()) {
	//	if (ImGui::BeginMenu("Editor")) {
	//		if (ImGui::MenuItem("Level")) {
	//			openLevelsModal = true;
	//		}
	//		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	//		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	//		ImGui::EndMenu();
	//	}
	//	ImGui::EndMainMenuBar();
	//}

	//if (openLevelsModal) {
	//	ImGui::OpenPopup("Levels");
	//}

	//ImGui::ViewportS
	//ImGui::SetNextWindowSize()
	//if (ImGui::BeginPopupModal("Levels", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
	//	//ImGui::BeginChild("left pane", ImVec2(150, 0), true);
	//	for (int i = 0; i < 100; i++)
	//	{
	//		// FIXME: Good candidate to use ImGuiSelectableFlags_SelectOnNav
	//		char label[128];
	//		snprintf(label, 128, "MyObject %d", i);
	//		if (ImGui::Selectable(label))
	//			;
	//	}
	//	//ImGui::EndChild();

	//	ImGui::EndPopup();
	//}

	//ImGui::ShowDemoWindow();

	//ImGui::OpenPopup("Delete?");
	//if (ImGui::BeginPopupModal("Delete?", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	//{
	//	ImGui::Text("All those beautiful files will be deleted.\nThis operation cannot be undone!\n\n");
	//	ImGui::Separator();

	//	//static int unused_i = 0;
	//	//ImGui::Combo("Combo", &unused_i, "Delete\0Delete harder\0");

	//	static bool dont_ask_me_next_time = false;
	//	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
	//	ImGui::Checkbox("Don't ask me next time", &dont_ask_me_next_time);
	//	ImGui::PopStyleVar();

	//	if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
	//	ImGui::SetItemDefaultFocus();
	//	ImGui::SameLine();
	//	if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
	//	ImGui::EndPopup();
	//}

	ImGui::Begin("editor");
	sceneWindowWindowSpace = Aabb::fromCorners(
		Vec2{ ImGui::GetWindowPos() } + ImGui::GetWindowContentRegionMin(),
		Vec2{ ImGui::GetWindowPos() } + ImGui::GetWindowContentRegionMax()
	);
	const auto sceneWindowSize = sceneWindowWindowSpace.size();
	ImGui::Image(reinterpret_cast<void*>(renderer.windowTextureShaderResourceView.Get()), sceneWindowSize, Vec2{ 0.0f }, sceneWindowSize / renderer.textureSize);

	if (IsWindowHovered()) {
		Input::ignoreImGuiWantCapture = true;
	} else {
		Input::ignoreImGuiWantCapture = false;
	}

	ImGui::End();

	Begin("entites");
	auto addBody = [this](Vec2 pos, const Collider& collider) -> void {
		const auto info = massInfo(collider, DEFAULT_DENSITY);
		const auto entity = entites.body.add(BodyEditor{
			.pos = pos,
			.orientation = 0.0f,
			.vel = Vec2{ 0.0f },
			.angularVel = 0.0f,
			.mass = info.mass,
			.rotationalInertia = info.rotationalInertia,
			.collider = collider
		});
		commands.addCommand(CreateEntityCommand{ entity });
	};

	if (Button("add rectangle")) {
		addBody(camera.pos, BoxColliderEditor{ Vec2{ 1.0f } });
	}
	if (Button("add circle")) {
		addBody(camera.pos, CircleColliderEditor{ 0.5f });
	}
	End();

	Begin("selected");
	for (const auto& entity : selectedEntities) {
		if (TreeNode(frameAllocator.format("entity%d", entity.index).data())) {

			PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
			if (!BeginTable("properites", 2, ImGuiTableFlags_SizingStretchProp)) {
				PopStyleVar();
				continue;
			}

			switch (entity.type) {
			case EntityType::Null: ImGui::Text("no entity selected"); break;
			case EntityType::Body: {
				auto& body = entites.body[entity.index];
				guiState.updateBeforeOpeningGui();
				body.editorGui(guiState, entites, entity, commands);
				break;
			}
				
			case EntityType::DistanceJoint: {
				auto& distanceJoint = entites.distanceJoint[entity.index];
				guiState.updateBeforeOpeningGui();
				distanceJoint.editorGui(guiState, entites, entity, commands);
				break;
			}
				
			}

			EndTable();
			PopStyleVar();

			TreePop();
		}
	}

	if (selectedEntities.size() > 0 && Button("make static")) {
		commands.beginMulticommand();
		for (const auto& entity : selectedEntities) {
			if (entity.type != EntityType::Body)
				continue;

			auto& body = entites.body[entity.index];
			constexpr auto newMassAndInertia = std::numeric_limits<float>::infinity();
			commands.addSetFieldCommand(entity, BODY_EDITOR_MASS_OFFSET, body.mass, newMassAndInertia);
			commands.addSetFieldCommand(entity, BODY_EDITOR_ROTATIONAL_INERTIA_OFFSET, body.rotationalInertia, newMassAndInertia);
			body.mass = newMassAndInertia;
			body.rotationalInertia = newMassAndInertia;
		}
		commands.endMulticommand();
	}

	if (selectedEntities.size() > 0) {
		auto recalculateMass = [this](const Entity& entity, float density) -> void {
			if (entity.type != EntityType::Body)
				return;
			auto& body = entites.body[entity.index];

			const auto info = massInfo(body.collider, density);
			body.mass = info.mass;
			body.rotationalInertia = info.rotationalInertia;
		};

		Checkbox("automatically recalculate mass", &automaticallyRecalculateMass);
		if (automaticallyRecalculateMass) {
			for (auto body = entites.body.alive().begin(); body != entites.body.alive().end(); ++body) {
				if (body->mass != std::numeric_limits<float>::infinity()) {
					recalculateMass(Entity{ EntityType::Body, body.index }, densityForRecalculation);
				}
			}
		} else {
			if (Button("recalculate mass")) {
				for (const auto& entity : selectedEntities) {
					recalculateMass(entity, densityForRecalculation);
				}
			}
			if (IsItemHovered())
				SetTooltip("recalculate mass and rotational inertia with uniform density");
		}
		InputFloat("density", &densityForRecalculation);
	}

	if (selectedEntities.size() == 2 && selectedEntities[0].type == EntityType::Body && selectedEntities[1].type == EntityType::Body
		&& Button("join with a distance joint")) {
		const auto entity = entites.distanceJoint.add(DistanceJointEntityEditor{
			.anchorA = { selectedEntities[0].index, Vec2{ 0.0f } },
			.anchorB = { selectedEntities[1].index, Vec2{ 0.0f } },
			.distance = 0.0f
		});
		commands.addCommand(CreateEntityCommand{ entity });
	}

	for (auto i = 0; i < entites.distanceJoint.size(); i++) {
		auto& distanceJoint = entites.distanceJoint[i];
		if (entites.distanceJoint.isAlive[i]) {
			const auto& bodyA = entites.body[distanceJoint.anchorA.body];
			const auto& bodyB = entites.body[distanceJoint.anchorB.body];
			distanceJoint.distance = distance(bodyA.pos, bodyB.pos);
		}
	}

	End();

	Begin("gizmo settings");
	selectedEntitiesGizmo.settingsGui();
	End();

	if (!guiState.inputing) {
		// TODO: This would be already be computed if the offests into the stack were stored instead of sizes. The sizes can be compuated by taking the difference between to positions.
		usize currentCommandStackPosition = 0;
		for (usize i = 0; i < commands.commandsSizesTop; i++) {
			currentCommandStackPosition += commands.commandSizes[i];
		}

		if (commands.commandsSizesTop > 0 && Input::isKeyHeld(Keycode::CTRL) && Input::isKeyDownWithAutoRepeat(Keycode::Z)) {
			commands.commandsSizesTop--;
			for (usize i = 0; i < commands.commandSizes[commands.commandsSizesTop]; i++) {
				// currentCommandStackPosition is either the last execute command or nothing so start one below it.
				undoCommand(commands.commandStack[currentCommandStackPosition - 1 - i]);
			}
		} else if (commands.commandsSizesTop < commands.commandSizes.size() && Input::isKeyHeld(Keycode::CTRL) && Input::isKeyDownWithAutoRepeat(Keycode::Y)) {
			for (usize i = 0; i < commands.commandSizes[commands.commandsSizesTop]; i++) {
				redoCommand(commands.commandStack[currentCommandStackPosition + i]);
			}
			commands.commandsSizesTop++;
		}
	}

	if (Input::isKeyHeld(Keycode::CTRL) && Input::isKeyDown(Keycode::S)) {
		saveCurrentLevel();
	}

	auto cursorPos = getCursorPos();

	if (Input::isButtonHeld(EditorButton::SNAP_CURSOR_TO_SHAPE_FEATURES)) {
		for (auto body = entites.body.alive().begin(); body != entites.body.alive().end(); ++body) {
			if (selectedEntitesSet.find(body.toEntity()) != selectedEntitesSet.end()) {
				continue;
			}

			const auto SNAP_DISTANCE = 0.025f / camera.zoom;
			const auto point = std::visit(overloaded{
				[&](const BoxCollider& box) -> std::optional<Vec2> {
					for(const auto& corner : box.getCorners(body->pos, body->orientation)) {
						if (distance(cursorPos, corner) < SNAP_DISTANCE) {
							return corner;
						}
					}
					return std::nullopt;
				},
				[](const CircleCollider&) -> std::optional<Vec2> { return std::nullopt; }
			}, body->collider);

			if (point.has_value()) {
				cursorPos = *point;
				break;
			}
		}
	}

	// For the camera not to lag behind it is updated first so all functions have the same information about the camera.
	const auto aspectRatio = sceneWindowSize.x / sceneWindowSize.y;
	updateCamera(aspectRatio);

	auto isGizmoSelected = false;

	isGizmoSelected = distanceJointGizmo.update(selectedEntities, entites, cursorPos, camera, commands);
	const auto distanceJointSelected = isGizmoSelected;

	if (!isGizmoSelected) {
		isGizmoSelected = selectedEntitiesGizmo.update(entites, commands, camera, selectedEntities, selectedEntitiesCenterPos, getCursorPos());
	}

	if (!isGizmoSelected) {
		isGizmoSelected = scalingGizmo.update(selectedEntities, selectedEntitesAabb, commands, cursorPos, entites, 0.05f / camera.zoom);
	}
	
	const auto distanceJointColliderThickness = 0.03f / camera.zoom;

	const auto oldSelectedEntites = selectedEntities;
	if (!isGizmoSelected) {
		if (Input::isMouseButtonDown(MouseButton::LEFT)) {
			selectGrabStartPos = cursorPos;
			selecting = true;
		}

		const auto selectedBox = Aabb::fromCorners(selectGrabStartPos, cursorPos);

		// When more entity types are added make a shared function that checks if the cursor intersects this shape.
		if (Input::isMouseButtonUp(MouseButton::LEFT)) {
			selecting = false;
			if (!Input::isKeyHeld(Keycode::CTRL)) {
				selectedEntities.clear();
			}

			if (const auto doBoxSelect = selectedBox.area() > 0.1f) {

				auto selectLogic = [this](const Entity& entity) -> void {
					const auto it = std::find(selectedEntities.begin(), selectedEntities.end(), entity);
					if (it == selectedEntities.end()) {
						selectedEntities.push_back(entity);
					} else if (Input::isKeyHeld(Keycode::CTRL)) {
						selectedEntities.erase(it);
					}
				};

				for (auto body = entites.body.alive().begin(); body != entites.body.alive().end(); ++body) {
					if (aabbContains(selectedBox, body->collider, body->pos, body->orientation)) {
						selectLogic(Entity{ EntityType::Body, body.index });
					}
				}

				for (auto distanceJoint = entites.distanceJoint.alive().begin(); distanceJoint != entites.distanceJoint.alive().end(); ++distanceJoint) {
					if (selectedBox.contains(entites.getDistanceJointLineSegment(*distanceJoint).aabb())) {
						selectLogic(Entity{ EntityType::DistanceJoint, distanceJoint.index });
					}
				}

			} else if (Input::isKeyHeld(Keycode::CTRL)) { // Point select remove

				auto selectLogic = [this](const Entity& entity) -> void {
					const auto it = std::find(selectedEntities.begin(), selectedEntities.end(), entity);
					if (it == selectedEntities.end()) {
						selectedEntities.push_back(entity);
					} else {
						selectedEntities.erase(it);
					}
				};

				for (auto body = entites.body.alive().begin(); body != entites.body.alive().end(); ++body) {
					if (contains(cursorPos, body->pos, body->orientation, body->collider)) {
						selectLogic(Entity{ EntityType::Body, body.index });
						break;
					}
				}

				for (auto distanceJoint = entites.distanceJoint.alive().begin(); distanceJoint != entites.distanceJoint.alive().end(); ++distanceJoint) {
					if (entites.getDistanceJointLineSegment(*distanceJoint).asCapsuleContains(distanceJointColliderThickness, cursorPos)) {
						selectLogic(Entity{ EntityType::DistanceJoint, distanceJoint.index });
						break;
					}
				}
			} else { // Point select add
				currentSelectedEntitesUnderCursor.clear();

				auto selectLogic = [this](const Entity& entity) -> bool {
					// With the current implementation the is no need for sorting because things are iterated in order, but with some acceleration structure in place it would be needed.
					// @Performance: Could also try using a set instead. This would make indexing slower which shouldn't be an issue.
					sortedInsert(currentSelectedEntitesUnderCursor, entity);

					// No reason to continue if the cycling select isn't going to happen. It only happens when the entites after a click are the same as for the previous click.
					return currentSelectedEntitesUnderCursor.size() > lastSelectedEntitesUnderCursor.size();
				};

				for (auto body = entites.body.alive().begin(); body != entites.body.alive().end(); ++body) {
					if (contains(cursorPos, body->pos, body->orientation, body->collider)) {
						if (selectLogic(Entity{ EntityType::Body, body.index }))
							break;
					} 
				}

				for (auto distanceJoint = entites.distanceJoint.alive().begin(); distanceJoint != entites.distanceJoint.alive().end(); ++distanceJoint) {
					if (entites.getDistanceJointLineSegment(*distanceJoint).asCapsuleContains(distanceJointColliderThickness, cursorPos)) {
						if (selectLogic(Entity{ EntityType::DistanceJoint, distanceJoint.index }))
							break;
					}
				}

				if (!currentSelectedEntitesUnderCursor.empty()) {
					if (currentSelectedEntitesUnderCursor == lastSelectedEntitesUnderCursor) {
						currentEntityIndexInSelectCycle = (currentEntityIndexInSelectCycle + 1) % currentSelectedEntitesUnderCursor.size();
						selectedEntities.push_back(currentSelectedEntitesUnderCursor[currentEntityIndexInSelectCycle]);
					} else {

						if (const auto fristEntityInNewCycleIsThePreviousSelectedEntity =
							currentSelectedEntitesUnderCursor.size() > 1
							&& !lastSelectedEntitesUnderCursor.empty()
							&& lastSelectedEntitesUnderCursor[currentEntityIndexInSelectCycle] == currentSelectedEntitesUnderCursor[0]) {
							currentEntityIndexInSelectCycle = 1;
						} else {
							currentEntityIndexInSelectCycle = 0;
						}

						selectedEntities.push_back(currentSelectedEntitesUnderCursor[currentEntityIndexInSelectCycle]);
					}
				}
				std::swap(lastSelectedEntitesUnderCursor, currentSelectedEntitesUnderCursor);
			}
		}

		if (selecting) {
			Debug::drawAabb(selectedBox, Vec3::WHITE / 4.0f);
		}
	}

	if (Input::isKeyHeld(Keycode::CTRL) && Input::isKeyDown(Keycode::A)) {
		selectedEntities.clear();
		for (auto body = entites.body.alive().begin(); body != entites.body.alive().end(); ++body) {
			selectedEntities.push_back(Entity{ .type = EntityType::Body, .index = body.index });
		}
	}

	// This only tracks changes that happen between the declaration of oldSelectedEntites and here.
	selectedEntitesChanged = false;
	if (selectedEntities != oldSelectedEntites) {
		commands.addSelectCommand(oldSelectedEntites, selectedEntities);
		selectedEntitesChanged = true;
	} 

	selectedEntitesMoved = false;
	updateSelectedEntitesData();
	if (selectedEntitiesCenterPos != lastFrameSelectedEntitiesCenterPos || selectedEntitesAabb != lastFrameSelectedEntitiesAabb) {
		selectedEntitesMoved = true;
	}
	lastFrameSelectedEntitiesCenterPos = selectedEntitiesCenterPos;
	lastFrameSelectedEntitiesAabb = selectedEntitesAabb;

	if (Input::isKeyDown(Keycode::DEL)) {
		commands.beginMulticommand();

		const Span<const Entity> empty{ nullptr, 0 };
		commands.addSelectCommand(selectedEntities, empty);

		for (const auto& entity : selectedEntities) {
			entites.setIsAlive(entity, false);
			commands.addCommand(DeleteEntityCommand{ entity });
		}

		
		for (auto distanceJoint = entites.distanceJoint.alive().begin(); distanceJoint != entites.distanceJoint.alive().end(); ++distanceJoint) {
			const auto bothBodiesAlive = entites.body.isAlive[distanceJoint->anchorA.body] && entites.body.isAlive[distanceJoint->anchorB.body];
			if (!bothBodiesAlive) {
				entites.distanceJoint.isAlive[distanceJoint.index] = false;
				commands.addCommand(DeleteEntityCommand{ distanceJoint.toEntity() });
			}
		}
		
		commands.endMulticommand();

		selectedEntities.clear();
	}

	auto copyToClipboard = [this]() -> void {
		clipboard = selectedEntities;
	};

	auto pasteClipboard = [this]() -> void {
		commands.beginMulticommand();

		std::vector<Entity> pastedEntites;
		std::unordered_map<usize, usize> originalBodyToCopy;
		for (const auto& entity : clipboard) {
			switch (entity.type) {
			case EntityType::Body: {
				const auto body = entites.body.add(entites.body[entity.index]);
				pastedEntites.push_back(body);
				originalBodyToCopy[entity.index] = body.index;
				commands.addCommand(CreateEntityCommand{ .entity = body });
				break;
			}
			
			case EntityType::DistanceJoint: {
				const auto& original = entites.distanceJoint[entity.index];
				const auto copyA = originalBodyToCopy.find(original.anchorA.body);
				const auto copyB = originalBodyToCopy.find(original.anchorB.body);
				if (const auto notConnectedToOtherPastedBodies = copyA == originalBodyToCopy.end() || copyB == originalBodyToCopy.end()) {
					break;
				}

				const auto& distanceJointEntity = entites.distanceJoint.add(entites.distanceJoint[entity.index]);
				pastedEntites.push_back(distanceJointEntity);
				commands.addCommand(CreateEntityCommand{ .entity = distanceJointEntity });
				auto& distanceJoint = entites.distanceJoint[distanceJointEntity.index];

				distanceJoint.anchorA.body = copyA->second;
				distanceJoint.anchorB.body = copyB->second;
				
				break;
			}
				
			default: ASSERT_NOT_REACHED();
			}
		}
		commands.addSelectCommand(selectedEntities, pastedEntites);
		selectedEntities = pastedEntites;

		commands.endMulticommand();
	};

	if (Input::isKeyHeld(Keycode::CTRL) && Input::isKeyDown(Keycode::C)) copyToClipboard();
	if (Input::isKeyHeld(Keycode::CTRL) && Input::isKeyDown(Keycode::V)) pasteClipboard();
	if (Input::isKeyHeld(Keycode::CTRL) && Input::isKeyDown(Keycode::D)) {
		copyToClipboard();
		pasteClipboard();
	}
		
	for (const auto& body : entites.body.alive()) {
		const auto color = (body.mass == std::numeric_limits<float>::infinity()) ? Vec3::WHITE : Vec3::GREEN;
		Debug::drawCollider(body.collider, body.pos, body.orientation, color);
	}

	for (const auto& distanceJoint : entites.distanceJoint.alive()) {
		const auto [posA, posB] = entites.getDistanceJointEndpoints(distanceJoint);
		Debug::drawLine(posA, posB);
	}

	distanceJointGizmo.draw(selectedEntities, entites);

	if (selectedEntities.size() != 0 && !DistanceJointGizmo::displayGizmo(selectedEntities)) {
		selectedEntitiesGizmo.draw(selectedEntitiesCenterPos);
		scalingGizmo.draw(selectedEntities, selectedEntitesAabb, entites);
	}

	renderer.update(gfx, camera, sceneWindowSize);

	debugChecks();
}

auto Editor::saveCurrentLevel() -> void {
	const auto level = saveLevel();
	std::ofstream file("levels/test");
	Json::prettyPrint(file, level);
}

auto Editor::updateCamera(float aspectRatio) -> void {
	camera.aspectRatio = aspectRatio;
	const auto cursorPos = getCursorPos();

	const auto userMovedCamera = lastFrameFocusPos != camera.pos;
	if (userMovedCamera || selectedEntitesChanged || selectedEntitesMoved) {
		focusing = false;
	}

	if (Input::isButtonDown(EditorButton::FOCUS)) {
		focusing = true;
		elapsedSinceFocusStart = 0.0f;
	}

	if (!selectedEntities.empty() && focusing) {
		elapsedSinceFocusStart = std::min(elapsedSinceFocusStart + Time::deltaTime(), 1.0f);
		// The camera view is 2 units wide at zoom = 1;
		const auto aabbSize = selectedEntitesAabb.max - selectedEntitesAabb.min;

		const auto targetZoom = 1.0f / std::max(aabbSize.y, camera.heightIfWidthIs(aabbSize.x));

		const auto [pos, zoom] = lerpPosAndZoom({ camera.pos, camera.zoom }, { selectedEntitesAabb.center(), targetZoom }, elapsedSinceFocusStart);
		camera.pos = pos;
		camera.zoom = zoom;

		lastFrameFocusPos = camera.pos;
	}

	if (Input::isMouseButtonDown(MouseButton::MIDDLE)) {
		screenGrabStartPos = cursorPos;
	} else if (Input::isMouseButtonHeld(MouseButton::MIDDLE)) {
		camera.pos -= (cursorPos - screenGrabStartPos);
	}

	if (const auto scroll = Input::scrollDelta(); scroll != 0.0f) {
		const auto cursorPosBeforeScroll = getCursorPos();
		const auto scrollSpeed = 15.0f * abs(scroll);
		const auto scrollIncrement = pow(scrollSpeed, Time::deltaTime());
		if (scroll > 0.0f) camera.zoom *= scrollIncrement;
		else camera.zoom /= scrollIncrement;

		camera.pos -= (getCursorPos() - cursorPosBeforeScroll);
	}
}

auto Editor::debugChecks() -> void {
	for (auto& entity : selectedEntities) {
		ASSERT(entites.isAlive(entity));
	}
}

auto Editor::saveLevel() -> Json::Value {
	auto level = Json::Value::emptyObject();

	auto& bodies = (level["bodies"] = Json::Value::emptyArray()).array();
	std::unordered_map<usize, usize> bodyMap;
	for (auto body = entites.body.alive().begin(); body != entites.body.alive().end(); ++body) {
		const auto newIndex = bodies.size();
		const auto oldIndex = body.index;
		bodyMap[oldIndex] = newIndex;
		bodies.push_back(body->toJson());
	}

	// TODO: Remove duplicates.

	auto& distanceJoints = (level["distanceJoints"] = Json::Value::emptyArray()).array();
	for (const auto& distanceJoint : entites.distanceJoint.alive()) {
		const auto& bodyA = bodyMap.find(distanceJoint.anchorA.body);
		const auto& bodyB = bodyMap.find(distanceJoint.anchorB.body);

		if (bodyA == bodyMap.end() || bodyB == bodyMap.end()) {
			ASSERT_NOT_REACHED();
			return Json::Value::null();
		}
		auto joint = distanceJoint;
		joint.anchorA.body = bodyA->second;
		joint.anchorB.body = bodyB->second;
		distanceJoints.push_back(joint.toJson());
	}
	return level;
}

auto Editor::loadLevel(const Json::Value& level) -> void {
	selectedEntities.clear();
	clipboard.clear();
	lastSelectedEntitesUnderCursor.clear();
	currentSelectedEntitesUnderCursor.clear();
	entites.body.clear();
	entites.distanceJoint.clear();

	try {
		const auto& bodies = level.at("bodies").array();
		for (const auto& body : bodies) {
			entites.body.add(BodyEditor::fromJson(body));
		}

		const auto& distanceJoints = level.at("distanceJoints").array();
		for (const auto& distanceJoint : distanceJoints) {
			const auto joint = DistanceJointEntityEditor::fromJson(distanceJoint);
			if (!isDistanceJointEntityValid(entites, joint)) {
				/*ASSERT_NOT_REACHED();
				return;*/
			}
			entites.distanceJoint.add(joint);
		}

	} catch (const Json::Value::InvalidTypeAccess&) {

	}
}

auto Editor::getCursorPos() -> Vec2 {
	const auto windowSpace = (Input::cursorPosWindowSpace() - sceneWindowWindowSpace.min) * (Window::size() / sceneWindowWindowSpace.size());
	const auto screenSpace = Input::windowSpaceToScreenSpace(windowSpace);
	return camera.screenSpaceToCameraSpace(screenSpace);
}

auto Editor::addToSelectedEntities(const Entity& entity) -> void {
	if (std::find(selectedEntities.begin(), selectedEntities.end(), entity) == selectedEntities.end()) {
		selectedEntities.push_back(entity);
	}
}

auto Editor::updateSelectedEntitesData() -> void {
	if (selectedEntities.empty())
		return;

	selectedEntitesSet.clear();
	for (const auto& entity : selectedEntities) {
		selectedEntitesSet.insert(entity);
	}

	selectedEntitiesCenterPos = Vec2{ 0.0f };
	usize positions = 0;
	for (const auto& entity : selectedEntities) {
		const auto pos = entites.getPos(entity);
		if (!pos.has_value())
			continue;

		positions++;
		selectedEntitiesCenterPos += *pos;
	}
	selectedEntitiesCenterPos /= static_cast<float>(positions);

	std::optional<Aabb> aabb;
	for (const auto& entity : selectedEntities) {
		if (aabb.has_value()) {
			const auto entityAabb = entites.getAabb(entity);
			if (entityAabb.has_value()) {
				aabb = aabb->combined(*entityAabb);
			}
		} else {
			aabb = entites.getAabb(entity);
		}
	}
	if (aabb.has_value()) {
		selectedEntitesAabb = *aabb;
	}
}

auto Editor::undoCommand(const Command& command) -> void {
	std::visit(overloaded{
		[this](const SetFieldCommand& setField) {
			auto field = entites.getFieldPointer(setField.entity, setField.pointerOffset);
			memcpy(field, commands.getPtr(setField.oldDataPtr), setField.size);
		},
		[this](const SelectCommand& select) {
			selectedEntities = select.oldSelectedEntites;
		},
		[this](const CreateEntityCommand& create) {
			entites.setIsAlive(create.entity, false);
		},
		[this](const DeleteEntityCommand& deleteEntity) {
			entites.setIsAlive(deleteEntity.entity, true);
		},
	}, command);
}

auto Editor::redoCommand(const Command& command) -> void {
	std::visit(overloaded{
		[this](const SetFieldCommand& setField) {
			auto field = entites.getFieldPointer(setField.entity, setField.pointerOffset);
			memcpy(field, commands.getPtr(setField.newDataPtr), setField.size);
		},
		[this](const SelectCommand& select) {
			selectedEntities = select.newSelectedEntites;
		},
		[this](const CreateEntityCommand& create) {
			entites.setIsAlive(create.entity, true);
		},
		[this](const DeleteEntityCommand& deleteEntity) {
			entites.setIsAlive(deleteEntity.entity, false);
		},
	}, command);
}
