#include <game/gameMain.hpp>

GameMain::GameMain(Gfx& gfx) 
	: renderer{ gfx }
	, gfx{ gfx } {}

#include <imgui/imgui.h>

auto GameMain::update() -> void {
	editor.update(gfx, renderer);
}
