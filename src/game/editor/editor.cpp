#include <game/editor/editor.hpp>
#include <game/debug.hpp>
#include <imgui/imgui.h>
#include <game/editor/input.hpp>
#include <game/collision/collision.hpp>
#include <engine/time.hpp>
#include <math/lineSegment.hpp>
#include <math/mat2.hpp>
#include <engine/window.hpp>
#include <utils/io.hpp>
#include <utils/overloaded.hpp>

#include <engine/frameAllocator.hpp>
Editor::Editor() {
	camera.zoom = 0.125f / 2.0f;
	camera.pos = Vec2{ 0.0f, 0.0f };
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
	camera.aspectRatio = Window::aspectRatio();	

	ImGui::ShowDemoWindow();

	using namespace ImGui;
	Begin("entites");

	if (Button("+")) {
		entites.entitesBody.push_back(BodyEditor{
			.pos = Vec2{ camera.pos },
			.orientation = 0.0f,
			.vel = Vec2{ 0.0f },
			.angularVel = 0.0f,
			.mass = 20.0f,
			.rotationalInertia = 1.0f,
			/*.collider = CircleColliderEditor{ 1.0f }*/
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
				// The state can get modifed by next commands and this changes need to be saved. Either create a copy each time or double buffer 2 currentInputStates.
				// It might be better to add the command inside the gui method. This will allow for custom guis to use multicommands. Like defaulting collider. This might be better as a separe command thogh becaouse of vectors. This also stores less state, which makes it simpler to understand the program.
				// Could pass the offset to which field needs to be saved and do the copying here. This might make the generated code simpler, but It requires the custom code to pass the data and also this change wouldn't count towards a custom multicommand.
				// For storing variable sized types could store pointers to data allocated on the command allocator stack.
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

		if (commands.commandsSizesTop > 0 && Input::isKeyHeld(Keycode::CTRL) && Input::isKeyDown(Keycode::Z)) {
			commands.commandsSizesTop--;
			for (usize i = 0; i < commands.commandSizes[commands.commandsSizesTop]; i++) {
				// currentCommandStackPosition is either the last execute command or nothing so start one below it.
				undoCommand(commands.commandStack[currentCommandStackPosition - 1 - i]);
			}
			dbg("undo");
		} else if (commands.commandsSizesTop < commands.commandSizes.size() && Input::isKeyHeld(Keycode::CTRL) && Input::isKeyDown(Keycode::Y)) {
			for (usize i = 0; i < commands.commandSizes[commands.commandsSizesTop]; i++) {
				redoCommand(commands.commandStack[currentCommandStackPosition + i]);
			}
			dbg("redo");
			commands.commandsSizesTop++;
		}
	}

	// Chain editor. Half spaces or lines?
	// Proceduraly generating terrain under the chain.

	const auto isGizmoSelected = selectedEntityGizmo.update(entites, camera, selectedEntities, selectedEntitiesCenterPos, getCursorPos());

	const auto cursorPos = getCursorPos();

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
					// @Performance: Could also try using a set instead. This would make indexing solwer which shouldn't be an issue.
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

	if (Input::isMouseButtonDown(MouseButton::MIDDLE)) {
		screenGrabStartPos = cursorPos;
	} else {
		if (Input::isMouseButtonHeld(MouseButton::MIDDLE)) {
			camera.pos -= (cursorPos - screenGrabStartPos);
		}
	}

	updateSelectedEntitesCenterPos();

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

auto Editor::updateSelectedEntitesCenterPos() -> void {
	selectedEntitiesCenterPos = Vec2{ 0.0f };
	for (const auto& entity : selectedEntities)
		selectedEntitiesCenterPos += entites.getPosOrOrigin(entity);
	selectedEntitiesCenterPos /= static_cast<float>(selectedEntities.size());
}

auto Editor::undoCommand(const Command& command) -> void {
	std::visit(overloaded{
		[this](const SetFieldCommand& setField) {
			auto field = entites.getFieldPointer(setField.entity, setField.pointerOffset);
			memcpy(field, commands.getPtr(setField.oldDataPtr), setField.size);
		},
	}, command);
}

auto Editor::redoCommand(const Command& command) -> void {
	std::visit(overloaded{
		[this](const SetFieldCommand& setField) {
			auto field = entites.getFieldPointer(setField.entity, setField.pointerOffset);
			memcpy(field, commands.getPtr(setField.newDataPtr), setField.size);
		},
	}, command);
}
