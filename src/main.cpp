#include <win.hpp>
#include <winUtils.hpp>
#include <gfx/gfx.hpp>
#include <game/game.hpp>
#include <engine/time.hpp>

#include <chrono>

auto WINAPI windowMessageCallback(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) -> LRESULT {
	switch (msg)
	{
	case WM_CLOSE:
		PostQuitMessage(EXIT_SUCCESS);
		return 0;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

auto WINAPI WinMain(
	_In_ HINSTANCE hInstance, // Application instance. Can also get it by calling GetModuleHandle(nullptr).
	_In_opt_ HINSTANCE,
	_In_ LPSTR argsString, 
	_In_ int
) -> int {
	const auto windowClassName = "game";
	auto createWindow = [&](LONG width, LONG height, const char* title) -> HWND {
		WNDCLASSEX windowsClassInfo{
			.cbSize = sizeof(windowsClassInfo),
			.style = CS_OWNDC,
			.lpfnWndProc = windowMessageCallback,
			.cbClsExtra = 0,
			.cbWndExtra = 0,
			.hInstance = hInstance,
			.hIcon = nullptr,
			.hCursor = nullptr,
			.hbrBackground = nullptr,
			.lpszMenuName = nullptr,
			.lpszClassName = windowClassName,
			.hIconSm = nullptr,
		};
		CHECK_WIN_ZERO(RegisterClassEx(&windowsClassInfo));

		const DWORD windowStyle = WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU;
		const auto left = 0;
		const auto top = 0;
		RECT windowRect{
			.left = left,
			.top = top,
			.right = left + width,
			.bottom = top + height
		};

		// Adjust the size so the client region (no title bar or any other bars) has the desired size.
		CHECK_WIN_BOOL(AdjustWindowRect(&windowRect, windowStyle, FALSE));

		HWND hWnd = CreateWindowEx(
			0,
			windowClassName,
			title,
			windowStyle,
			CW_USEDEFAULT, CW_USEDEFAULT,
			windowRect.right - windowRect.left, windowRect.bottom - windowRect.top,
			nullptr, nullptr, hInstance, nullptr
		);
		CHECK_WIN_NULL(hWnd);

		ShowWindow(hWnd, SW_SHOW);
		return hWnd;
	};

	auto destroyWindow = [&](HWND hWnd) {
		CHECK_WIN_BOOL(DestroyWindow(hWnd));
		CHECK_WIN_BOOL(UnregisterClass(windowClassName, hInstance));
	};

	HWND hWnd{ createWindow(640, 480, "game") };
	Gfx gfx{ hWnd };

	Game game{ gfx };

	auto currentTime = []() -> float {
		using namespace std::chrono;
		return duration<float> { duration_cast<seconds>(high_resolution_clock::now().time_since_epoch()) }.count();
	};

	MSG msg;
	int exitCode;
	auto previousFrameStart{ currentTime() }, accumulated{ 0.0f };
	static constexpr auto FRAME_TIME = 1.0f / 60.0f;
	for (;;) {
		const auto frameStart{ currentTime() };
		const auto elapsed{ frameStart - previousFrameStart };
		accumulated += elapsed;
		previousFrameStart = frameStart;

		while (accumulated >= FRAME_TIME) {
			if (bool anyNewMessages{ PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE) == TRUE }) {
				// Translate virtual-key messages (WM_KEY_DOWN) to character messages (VM_CHAR) and puts them back onto the message queue.
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}

			if (msg.message == WM_QUIT) {
				exitCode = static_cast<int>(msg.wParam);
				goto exit;
			}

			accumulated -= FRAME_TIME;
			Time::update(FRAME_TIME);

			game.update(gfx);
			// If the rendering is the bottleneck it might be better to take it out of this loop so the game can catch up be updating multiple times.
			gfx.present();
		}
	}

	exit:

	destroyWindow(hWnd);

	return exitCode;
}