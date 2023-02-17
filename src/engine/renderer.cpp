#include <engine/renderer.hpp>
#include <engine/dx11Renderer.hpp>
#include <engine/window.hpp>
#include <imgui/imgui_impl_dx11.h>
#include <imgui/imgui_impl_win32.h>

static std::unique_ptr<Gfx> gfx;
// Could make renderer an interface.
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

auto Renderer::update(Camera& camera, std::optional<float> gridSmallCellSize, std::optional<Vec2> windowSizeIfRenderingToTexture) -> void {
	renderer->update(camera, gridSmallCellSize, windowSizeIfRenderingToTexture);
}

auto Renderer::updateFrameStart() -> void {
	gfx->update();
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

auto Renderer::updateFrameEnd(bool enableVsync) -> void {
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	gfx->present(enableVsync);
}

auto Renderer::screenshot() -> ImageRgba {
	return renderer->screenshot();
}

auto Renderer::drawDynamicTexture(Vec2 pos, float height, DynamicTexture& dynamicTexture, bool interpolate) -> void {
	renderer->drawDynamicTexture(pos, height, dynamicTexture, interpolate);
}

auto Renderer::outputTextureHandle() -> void* {
	return renderer->windowTextureShaderResourceView.Get();
}

auto Renderer::textureSize() -> Vec2 {
	return Dx11Renderer::textureSize;
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

DynamicTexture::DynamicTexture(DynamicTexture&& other) noexcept 
	: ImageRgba{ other }
	, textureHandle{ other.textureHandle }{
	other.textureHandle = 0;
}

auto DynamicTexture::operator=(DynamicTexture&& other) noexcept {
	ImageRgba::operator=(std::move(other));
	textureHandle = other.textureHandle;
	other.textureHandle = 0;
}

DynamicTexture::~DynamicTexture() {
	if (textureHandle != 0) {
		Renderer::destroyDynamicTexture(textureHandle);
	}
}
