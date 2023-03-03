#pragma once

#include <engine/debug.hpp>
#include <imgui/imgui.h>
#include <utils/io.hpp>

#define chk(name) static bool name = false; ImGui::Checkbox(#name, &name); if (name)
#define chkbox(name) static bool name = false; ImGui::Checkbox(#name, &name);
#define floatin(name, value) static float name = value; ImGui::InputFloat(#name, &name)
#define intin(name, value) static int name = value; ImGui::InputInt(#name, &name)