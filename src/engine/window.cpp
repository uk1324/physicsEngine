#include <pch.hpp>
#include <engine/window.hpp>
#include <engine/input.hpp>
#include <winUtils.hpp>
#include <stdlib.h>
#include <imgui/imgui_impl_win32.h>

auto Window::init(const char* title, Vec2 size) -> void {
	exitCode_ = EXIT_SUCCESS;
	running_ = true;
	size_ = size;
	resizedOnThisFrame = false;
	firstFrame = true;

	const auto hInstance = GetModuleHandle(nullptr);
	WNDCLASSEX windowsClassInfo{
		.cbSize = sizeof(windowsClassInfo),
		.style = CS_OWNDC,
		.lpfnWndProc = windowMessageCallback,
		.cbClsExtra = 0,
		.cbWndExtra = 0,
		.hInstance = hInstance,
		.hIcon = nullptr,
		.hCursor = LoadCursor(NULL, IDC_ARROW), // If hCurosr is set to nullptr then the busy cursor is displayed at the start and the cursor isn't updated correctly when moving it into the window.
		.hbrBackground = nullptr,
		.lpszMenuName = nullptr,
		.lpszClassName = WINDOW_CLASS_NAME,
		.hIconSm = nullptr,
	};
	// The arrow with busy circle cursor is displayed at start probably because in debug mode some DLLs need to be loaded. It doesn't happen when lauching without the debugger.
	CHECK_WIN_ZERO(RegisterClassEx(&windowsClassInfo));

	const DWORD windowStyle = WS_CAPTION | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU | WS_SIZEBOX;
	const auto left = 0;
	const auto top = 0;
	RECT windowRect{
		.left = left,
		.top = top,
		.right = left + static_cast<LONG>(size.x),
		.bottom = top + static_cast<LONG>(size.y)
	};

	// Adjust the size so the client region (no title bar or any other bars) has the desired size.
	CHECK_WIN_BOOL(AdjustWindowRect(&windowRect, windowStyle, FALSE));

	hWnd_ = CreateWindowEx(
		0,
		WINDOW_CLASS_NAME,
		title,
		windowStyle,
		CW_USEDEFAULT, CW_USEDEFAULT,
		windowRect.right - windowRect.left, windowRect.bottom - windowRect.top,
		nullptr, nullptr, hInstance, nullptr
	);
	CHECK_WIN_NULL(hWnd_);

	ShowWindow(hWnd_, SW_SHOW);
}

auto Window::destroy() -> void {
	CHECK_WIN_BOOL(DestroyWindow(hWnd_));
	CHECK_WIN_BOOL(UnregisterClass(WINDOW_CLASS_NAME, GetModuleHandle(nullptr)));
}

auto Window::update() -> void {
	if (firstFrame) {
		resizedOnThisFrame = true;
		firstFrame = false;
	}
	else if (resizedOnThisFrame)
		resizedOnThisFrame = false;

	MSG msg;
	if (bool anyNewMessages{ PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE) == TRUE }) {
		// Translate virtual-key messages (WM_KEY_DOWN) to character messages (VM_CHAR) and puts them back onto the message queue.
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	if (msg.message == WM_QUIT) {
		exitCode_ = static_cast<int>(msg.wParam);
		running_ = false;
	}
}

auto Window::maximize() -> void {
	PostMessage(hWnd_, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#include <utils/io.hpp>

auto WINAPI Window::windowMessageCallback(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) -> LRESULT {
	if (const auto result = ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) {
		return !result;
	}

	switch (msg) {
	case WM_CLOSE:
		PostQuitMessage(EXIT_SUCCESS);
		return 0;

	case WM_SIZE:
		// Resizing sends a lot of WM_SIZE messages and blocks the game loop by sending all the messages to this callback function. This is probably better because the swap chain doesn't need to get resized on every frame that the window is being resized and only gets updated once, when the application goes back to executing the game loop. Googling "win32 resizing blocking" gives good information about it.
		const auto size = MAKEPOINTS(lParam);
		size_.x = size.x;
		size_.y = size.y;
		resizedOnThisFrame = true;
		break;

	case WM_MOUSEMOVE:
		const auto pos = MAKEPOINTS(lParam);
		Input::onMouseMove(Vec2{ static_cast<float>(pos.x), static_cast<float>(pos.y) });
		break;

	case WM_KEYDOWN:
	case WM_SYSKEYDOWN: {
		const auto autoRepeat{ static_cast<bool>(lParam >> 30) };
		Input::onKeyDown(static_cast<u8>(wParam), autoRepeat);
	} break;

	case WM_KEYUP:
	case WM_SYSKEYUP:
	{
		Input::onKeyUp(static_cast<u8>(wParam));
	} break;

	case WM_LBUTTONDOWN: Input::onKeyDown(VK_LBUTTON, false); break;
	case WM_RBUTTONDOWN: Input::onKeyDown(VK_RBUTTON, false); break;
	case WM_MBUTTONDOWN: Input::onKeyDown(VK_MBUTTON, false); break;

	case WM_LBUTTONUP: Input::onKeyUp(VK_LBUTTON); break;
	case WM_RBUTTONUP: Input::onKeyUp(VK_RBUTTON); break;
	case WM_MBUTTONUP: Input::onKeyUp(VK_MBUTTON); break;

	case WM_MOUSEWHEEL:
		Input::onMouseScroll(GET_WHEEL_DELTA_WPARAM(wParam));
		break;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

HWND Window::hWnd_;

Vec2 Window::size_;
bool Window::firstFrame;
bool Window::resizedOnThisFrame;

int Window::exitCode_;
bool Window::running_;