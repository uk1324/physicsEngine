#include <game/gameMain.hpp>

GameMain::GameMain(Gfx& gfx) 
	: renderer{ gfx }
	, gfx{ gfx } {}

auto GameMain::update() -> void {
	editor.update(gfx, renderer);
}
