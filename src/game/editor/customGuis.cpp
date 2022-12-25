#include <game/editor/customGuis.hpp>
#include <math/utils.hpp>

#include <imgui/imgui.h>

using namespace ImGui;

auto inputAngle(const char* label, float* angle) -> bool {
	float angleDeg = radToDeg(*angle);
	const auto changed = InputFloat(label, &angleDeg);
	*angle = degToRad(angleDeg);
	return changed;
}
