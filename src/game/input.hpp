#pragma once

#include <engine/input.hpp>

enum class GameButton {
	LEFT,
	RIGHT,
	UP,
	DOWN,
	SELECT_GRAB_TOOL,
	SELECT_SELECT_TOOL,
	START_SELECT_JOINT_TOOL,
	SELECT_DISTANCE_JOINT_TOOL,
	SNAP_TO_IMPORTANT_FEATURES,
	// TODO: Maybe rename to follow
	FOCUS_SELECTED_ON_OBJECT
};