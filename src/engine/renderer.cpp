#include <engine/renderer.hpp>
#include <engine/dx11Renderer.hpp>
#include <engine/window.hpp>
#include <imgui/imgui_impl_dx11.h>
#include <imgui/imgui_impl_win32.h>

static std::unique_ptr<Gfx> gfx;
static std::unique_ptr<Dx11Renderer> renderer;

auto Renderer::init() -> void {
	gfx = std::make_unique<Gfx>(reinterpret_cast<HWND>(Window::hWnd()));
	renderer = std::make_unique<Dx11Renderer>(*gfx);
	ImGui_ImplDX11_Init(gfx->device.Get(), gfx->ctx.Get());
	ImGui_ImplWin32_Init(Window::hWnd());
}

auto Renderer::terminate() -> void {
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
}

auto Renderer::update(Camera& camera, std::optional<Vec2> windowSizeIfRenderingToTexture) -> void {
	renderer->update(camera, windowSizeIfRenderingToTexture);
}

auto Renderer::updateFrameStart() -> void {
	gfx->update();
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

auto Renderer::updateFrameEnd() -> void {
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	gfx->present();
}

auto Renderer::drawDynamicTexture(Vec2 pos, float height, DynamicTexture& dynamicTexture, bool interpolate) -> void {
	renderer->drawDynamicTexture(pos, height, dynamicTexture, interpolate);
}

auto Renderer::outputTextureHandle() -> void* {
	return renderer->windowTextureShaderResourceView.Get();
}

auto Renderer::createDynamicTexture(Vec2T<i64> size) -> u64 {
	return renderer->createDynamicTextureData(size);
}

auto Renderer::destroyDynamicTexture(u64 textureHandle) {
	renderer->destroyDynamicTextureData(textureHandle);
}

DynamicTexture::DynamicTexture(Vec2T<i64> size)
	: ImageRgba{ size }
	, textureHandle{ Renderer::createDynamicTexture(size) } {}

DynamicTexture::~DynamicTexture() {
	Renderer::destroyDynamicTexture(textureHandle);
}
