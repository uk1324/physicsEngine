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
	//camera.zoom = 0.125f / 2.0f;
	camera.zoom = 1.0f;
	camera.pos = Vec2{ 0.0f, 0.0f };
	registerInputButtons();
}

auto Editor::registerInputButtons() -> void {
	Input::registerKeyButton(Keycode::F, EditorButton::FOCUS);
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

	camera.aspectRatio = Window::aspectRatio();	

	ImGui::ShowDemoWindow();

	Begin("entites");
	if (Button("+")) {
		entites.entitesBody.push_back(BodyEditor{
			.pos = Vec2{ camera.pos },
			.orientation = 0.0f,
			.vel = Vec2{ 0.0f },
			.angularVel = 0.0f,
			.mass = 20.0f,
			.rotationalInertia = 1.0f,
			.collider = BoxColliderEditor{ Vec2{ 1.0f } }
		});
	}
	End();

	Begin("selected");
	for (const auto& entity : selectedEntities) {
		if (TreeNode(frameAllocator.format("entity%d", entity.index).data())) {
			switch (entity.type) {
			case EntityType::Null: ImGui::Text("no entity selected"); break;
			case EntityType::Body: 
				auto& body = entites.entitesBody[entity.index];
				guiState.updateBeforeOpeningGui();
				body.editorGui(guiState, entites, entity, commands);
				break;
			}
			TreePop();
		}
	}
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

	const auto isGizmoSelected = selectedEntityGizmo.update(entites, camera, selectedEntities, selectedEntitiesCenterPos, getCursorPos());

	const auto cursorPos = getCursorPos();

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

			if (selectedBox.area() > 0.1f) {
				for (usize i = 0; i < entites.entitesBody.size(); i++) {
					const auto& body = entites.entitesBody[i];
					if (aabbContains(selectedBox, body.collider, body.pos, body.orientation)) {
						const auto entity = Entity{ EntityType::Body, i };
						const auto it = std::find(selectedEntities.begin(), selectedEntities.end(), entity);
						if (it == selectedEntities.end()) {
							selectedEntities.push_back(entity);
						} else if (Input::isKeyHeld(Keycode::CTRL)) {
							selectedEntities.erase(it);
						}
					}
				}
			} else if (Input::isKeyHeld(Keycode::CTRL)) {
				for (usize i = 0; i < entites.entitesBody.size(); i++) {
					const auto& body = entites.entitesBody[i];
					if (contains(cursorPos, body.pos, body.orientation, body.collider)) {
						const auto entity = Entity{ EntityType::Body, i };
						const auto it = std::find(selectedEntities.begin(), selectedEntities.end(), entity);
						if (it == selectedEntities.end()) {
							selectedEntities.push_back(entity);
						} else {
							selectedEntities.erase(it);
						}
						break;
					}
				}
			} else {
				currentSelectedEntitesUnderCursor.clear();
				for (usize i = 0; i < entites.entitesBody.size(); i++) {
					const auto& body = entites.entitesBody[i];
					if (!contains(cursorPos, body.pos, body.orientation, body.collider)) {
						continue;
					}

					const auto selectedEntity = Entity{ EntityType::Body, i };

					// With the current implementation the is no need for sorting because things are iterated in order, but with some acceleration structure in place it would be needed.
					// @Performance: Could also try using a set instead. This would make indexing slower which shouldn't be an issue.
					sortedInsert(currentSelectedEntitesUnderCursor, selectedEntity);

					// No reason to continue if the cycling select isn't going to happen.
					if (currentSelectedEntitesUnderCursor.size() > lastSelectedEntitesUnderCursor.size()) {
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
		for (usize i = 0; i < entites.entitesBody.size(); i++) {
			selectedEntities.push_back(Entity{ .type = EntityType::Body, .index = i });
		}
	}

	selectedEntitesChanged = false;
	if (selectedEntities != oldSelectedEntites) {
		commands.addSelectCommand(oldSelectedEntites, selectedEntities);
		selectedEntitesChanged = true;
	} 

	if ((focusing && lastFrameFocusPos != camera.pos) || selectedEntitesChanged || selectedEntitesMoved) {
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
		const auto targetZoom = 1.0f / std::max(aabbSize.y, aabbSize.x / camera.aspectRatio);
		const auto zoomRatio = camera.zoom / targetZoom;

		// This code is interpolating between the current state and the target state, but it also works for interpolating between the start state and an end state. This just looks better.
		if (zoomRatio == 1.0f) {
			camera.pos = lerp(camera.pos, selectedEntitesAabb.center(), elapsedSinceFocusStart);
		} else {
			// https://gamedev.stackexchange.com/questions/188841/how-to-smoothly-interpolate-2d-camera-with-pan-and-zoom/188859#188859
			// Interpolate zoom using logarithms because for example if zooms were powers of 2 then to linearly interpolate between 0.5 and 2 it should reach 1.0 at t = 0.5. The interpolated value should be how many times should the start value be multipled by 2 which is what a logarithm is. To make this work for non integer arguments the exponential function is used because it's rate of changes is equal to itself for all arguments.
			camera.zoom = exp(lerp(log(camera.zoom), log(targetZoom), elapsedSinceFocusStart));
			// To make it look like the position is moving with the same speed as the zooming at each point in time the rate of change of position should be proportional to the camera zoom. This proportion can be calculated by integrating the zoom speed which is zoomRatio^x. This integral = (zoomRatio^x - 1) / ln(zoomRatio). The constant ln(zoomRatio) can be ignored because only the proportions are needed for normalizing the values. To normalize the values to the range <0, 1> the value needs to be divided. For 0 the integral is equal to 0 and for 1 it is equals (zoomRatio - 1).
			const auto posInterpolationSpeed = (pow(zoomRatio, elapsedSinceFocusStart) - 1) / (zoomRatio - 1);
			camera.pos = lerp(camera.pos, selectedEntitesAabb.center(), posInterpolationSpeed);
		}
		
		lastFrameFocusPos = camera.pos;
	}
	
	selectedEntitesMoved = false;
	updateSelectedEntitesData();
	if (selectedEntitiesCenterPos != lastFrameSelectedEntitiesCenterPos) {
		selectedEntitesMoved = true;
	}
	lastFrameSelectedEntitiesCenterPos = selectedEntitiesCenterPos;

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

	for (const auto& body : entites.entitesBody)
		Debug::drawCollider(body.collider, body.pos, body.orientation, Vec3::WHITE);

	if (!selectedEntities.empty()) {
		selectedEntityGizmo.draw(selectedEntitiesCenterPos);
	}

	renderer.update(gfx, camera);
}

auto Editor::getCursorPos() -> Vec2 {
	return camera.screenSpaceToCameraSpace(Input::cursorPos());
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
	}, command);
}
