#include <game/editor/editor.hpp>
#include <game/debug.hpp>
#include <imgui/imgui.h>
#include <game/editor/input.hpp>
#include <game/collision/collision.hpp>
#include <engine/time.hpp>
#include <math/lineSegment.hpp>
#include <math/mat2.hpp>
#include <math/utils.hpp>
#include <engine/window.hpp>
#include <utils/io.hpp>
#include <utils/overloaded.hpp>

#include <engine/frameAllocator.hpp>
Editor::Editor() {
	camera.zoom = 0.125f / 2.0f;
	camera.pos = Vec2{ 0.0f, 0.0f };
	registerInputButtons();
}

auto Editor::registerInputButtons() -> void {
	Input::registerKeyButton(Keycode::F, EditorButton::FOCUS);
	Input::registerKeyButton(Keycode::CTRL, EditorButton::GIZMO_GRID_SNAP);
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

auto Editor::update(Gfx& gfx, Renderer& renderer) -> void {
	using namespace ImGui;

	DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

	ImGui::Begin("testabc");
	sceneWindowWindowSpace = Aabb::fromCorners(
		Vec2{ ImGui::GetWindowPos() } + ImGui::GetWindowContentRegionMin(),
		Vec2{ ImGui::GetWindowPos() } + ImGui::GetWindowContentRegionMax()
	);
	const auto sceneWindowSize = sceneWindowWindowSpace.size();
	ImGui::Image(reinterpret_cast<void*>(renderer.textureShaderResourceView.Get()), sceneWindowSize, Vec2{ 0.0f }, sceneWindowSize / renderer.textureSize);

	if (IsWindowHovered()) {
		Input::ignoreImGuiWantCapture = true;
	} else {
		Input::ignoreImGuiWantCapture = false;
	}

	ImGui::End();

	Begin("entites");
	ImGui::Checkbox("testa", &Input::ignoreImGuiWantCapture);
	if (Button("+")) {
		const auto entity = entites.body.add(BodyEditor{
			.pos = Vec2{ camera.pos },
			.orientation = 0.0f,
			.vel = Vec2{ 0.0f },
			.angularVel = 0.0f,
			.mass = 20.0f,
			.rotationalInertia = 1.0f,
			.collider = BoxColliderEditor{ Vec2{ 1.0f } }
		});
		commands.addCommand(CreateEntityCommand{ entity });
	}
	End();

	Begin("selected");
	for (const auto& entity : selectedEntities) {
		if (TreeNode(frameAllocator.format("entity%d", entity.index).data())) {
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

			TreePop();
		}
	}
	if (selectedEntities.size() == 1 && selectedEntities[0].type == EntityType::Body && Button("add distance joint")) {
		const auto entity = entites.distanceJoint.add(DistanceJointEntityEditor{
			.anchorA = { selectedEntities[0].index, Vec2{ 0.0f } },
			.staticWorldSpaceAnchorOrBodyAnchorB = entites.getPosOrOrigin(selectedEntities[0]) + Vec2{ 0.0f, 2.0f },
			.distance = 1.0f 
		});
		commands.addCommand(CreateEntityCommand{ entity });
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

	const auto cursorPos = getCursorPos();

	// For the camera not to lag behind it is updated first so all functions have the same information about the camera.
	const auto aspectRatio = sceneWindowSize.x / sceneWindowSize.y;
	updateCamera(aspectRatio);

	auto isGizmoSelected = false;

	isGizmoSelected = distanceJointGizmo.update(selectedEntities, entites, cursorPos, camera, commands);
	const auto distanceJointSelected = isGizmoSelected;
	
	if (!isGizmoSelected) {
		isGizmoSelected = selectedEntitiesGizmo.update(entites, commands, camera, selectedEntities, selectedEntitiesCenterPos, getCursorPos());
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
		
		commands.endMulticommand();

		selectedEntities.clear();
	}

	auto copyToClipboard = [this]() -> void {
		clipboard.clear();
		for (const auto& entity : selectedEntities) {
			clipboard.push_back(entity);
		}
	};

	auto pasteClipboard = [this]() -> void {
		commands.beginMulticommand();
		for (const auto& entity : clipboard) {
			switch (entity.type) {
			case EntityType::Body: {
				const auto body = entites.body.add(entites.body[entity.index]);
				commands.addCommand(CreateEntityCommand{ .entity = body });
				break;
			}
				
			default: ASSERT_NOT_REACHED();
			}
		}
		commands.endMulticommand();
	};

	if (Input::isKeyHeld(Keycode::CTRL) && Input::isKeyDown(Keycode::C)) copyToClipboard();
	if (Input::isKeyHeld(Keycode::CTRL) && Input::isKeyDown(Keycode::V)) pasteClipboard();
	if (Input::isKeyHeld(Keycode::CTRL) && Input::isKeyDown(Keycode::D)) {
		copyToClipboard();
		pasteClipboard();
	}
		

	for (const auto& body : entites.body.alive()) {
		Debug::drawCollider(body.collider, body.pos, body.orientation, Vec3::WHITE);
	}

	for (const auto& distanceJoint : entites.distanceJoint.alive()) {
		const auto [posA, posB] = entites.getDistanceJointEndpoints(distanceJoint);
		Debug::drawLine(posA, posB);
	}

	distanceJointGizmo.draw(selectedEntities, entites);

	if (!selectedEntities.empty() && !DistanceJointGizmo::displayGizmo(selectedEntities)) {
		selectedEntitiesGizmo.draw(selectedEntitiesCenterPos);
	}

	renderer.update(gfx, camera, sceneWindowSize, true);

	debugChecks();
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
	level["bodies"] = Json::Value::emptyArray();
	auto& bodies = level["bodies"].array();
	for (const auto& body : entites.body.alive()) {
		bodies.push_back(body.toJson());
	}
	for (const auto& distanceJoint : entites.distanceJoint.alive()) {
		if (!entites.body.isAlive[distanceJoint.anchorA.body]) {
			ASSERT_NOT_REACHED();
			continue;
		}
		if (const auto& anchorB = std::get_if<DistanceJointAnchorEditor>(&distanceJoint.staticWorldSpaceAnchorOrBodyAnchorB); 
			anchorB != nullptr && !entites.body.isAlive[anchorB->body]) {
			ASSERT_NOT_REACHED();
			continue;
		}
		bodies.push_back(distanceJoint.toJson());
	}
	return level;
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
