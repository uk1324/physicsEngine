#define _CRT_SECURE_NO_WARNINGS
#include <utils/imageRgba.hpp>
#include <raylib.h>
#define RAYGUI_IMPLEMENTATION
#include <raygui.h>

#include <winUtils.hpp>
#include <messages.hpp>
#include <iostream>

#include "../../src/math/vec2.hpp"

auto to(Vec2 v) -> Vector2 {
	return Vector2{ v.x, v.y };
}

// Raylib name conflicts: https://github.com/raysan5/raylib/issues/1217
#define NOGDI
#define NOUSER

#include <windows.h>
#include <psapi.h>
#include <inttypes.h>

#include <filesystem>
namespace fs = std::filesystem;

auto main() -> int {
	u8 inputBuffer[2048];
	const auto serverHandle = CreateNamedPipeA(
		"\\\\.\\pipe\\debugger",
		PIPE_ACCESS_INBOUND,
		PIPE_TYPE_MESSAGE | PIPE_NOWAIT,
		1,			
		0, // Output buffer size
		sizeof(inputBuffer),
		0,
		nullptr
	);
	Image im;
	Texture2D texture;
	im.data = nullptr;

	CHECK_WIN_HANDLE(serverHandle);
	bool connected = false;
	HANDLE debuggedProcessHandle = nullptr;
	void* baseAddress; // Probably only useful for reading static values.
	auto read = [&](void* output, DWORD size) -> bool {
		DWORD bytesRead;
		if (ReadFile(serverHandle, output, size, &bytesRead, nullptr)) {
			ASSERT(bytesRead == size);
			return true;
		} else if (connected && GetLastError() == ERROR_BROKEN_PIPE) {
			std::cout << "client disconnected\n";
			connected = false;
		} else if (GetLastError() == ERROR_NO_DATA) {
			return false;
		}
		/*if (ReadFile(serverHandle, output, size, &bytesRead, nullptr)) {
			ASSERT(bytesRead == size);
			return true;
		} else if (connected && GetLastError() == ERROR_BROKEN_PIPE) {
			std::cout << "client disconnected\n";
			connected = false;
		}*/
		return false;
	};

	auto updateServer = [&]() -> void {
		// The standard says that padding is never added before the first member.
		DebuggerMessageType messageType;
		/*DWORD bytesToRead;
		CHECK_WIN_BOOL(PeekNamedPipe(serverHandle, nullptr, 0, nullptr, &bytesToRead, nullptr));
		if (bytesToRead <= 0) {
			return;
		}*/

		if (!read(&messageType, sizeof(messageType))) {
			return;
		}
		
		// https://stackoverflow.com/questions/14467229/get-base-address-of-process
		if (messageType == DebuggerMessageType::CONNECT) {
			if (connected) {
				ASSERT_NOT_REACHED();
			} else {
				ConnectMessage connect;
				read(reinterpret_cast<u8*>(&connect) + sizeof(DebuggerMessageType), sizeof(connect) - sizeof(DebuggerMessageType));
				std::cout << "client " << connect.processId << " connected\n";
				connected = true;
				debuggedProcessHandle = OpenProcess(PROCESS_VM_READ | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION, false, connect.processId);

				char executableName[1024] = "";
				CHECK_WIN_ZERO(GetProcessImageFileNameA(debuggedProcessHandle, executableName, sizeof(executableName)));

				HMODULE modules[1024];
				DWORD sizeNeededModules;
				if (EnumProcessModules(debuggedProcessHandle, modules, sizeof(modules), &sizeNeededModules)) {
					ASSERT(sizeNeededModules <= sizeof(modules)); // Error handling?
					for (int i = 0; i < sizeNeededModules / sizeof(HMODULE); i++) {
						char moduleFilename[1024] = "";
						CHECK_WIN_ZERO(GetModuleFileNameExA(debuggedProcessHandle, modules[i], moduleFilename, sizeof(moduleFilename)));
						const auto a = fs::path(executableName).stem();
						const auto b = fs::path(moduleFilename).stem();
						if (a == b) {
							baseAddress = modules[i];
							break;
						}
					}
				}
				// Should it assume the client disconnected? Should it handle it?
				CHECK_WIN_ZERO(debuggedProcessHandle);
			}

		} else if (connected == false) {
			std::cout << "message received before connecting\n";
		} else {
			switch (messageType) {
			case DebuggerMessageType::CONNECT: break;

			default:
				std::cout << "invalid message type\n";
			}
		}
	};

	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(640, 480, "game");
	SetWindowState(FLAG_WINDOW_MAXIMIZED);

	Camera2D camera{
		.offset = Vector2{ 0.0f, 0.0f },
		.target = Vector2{ 0.0f, 0.0f },
		.rotation = 0.0f,
		.zoom = 1.0f,
	};
	SetTargetFPS(60);

	while (!WindowShouldClose()) {
		updateServer();
		Vec2 dir{ 0.0f };
		if (IsKeyDown(KEY_W)) {
			dir.y += 1.0f;
		}
		if (IsKeyDown(KEY_S)) {
			dir.y -= 1.0f;
		}
		if (IsKeyDown(KEY_A)) {
			dir.x += 1.0f;
		}
		if (IsKeyDown(KEY_D)) {
			dir.x -= 1.0f;
		}
		dir = dir.normalized();
		camera.offset.x += dir.x;
		camera.offset.y += dir.y;

		const auto scroll = GetMouseWheelMove();
		if (scroll != 0.0f) {
			if (scroll < 0.0f) {
				camera.zoom /= 2.0f;
			} else {
				camera.zoom *= 2.0f;
			}
		}

		BeginDrawing();
		ClearBackground(BLACK);

		auto rectCenter = [&](const Rectangle& rect) -> Vec2 {
			return Vec2{ rect.x + rect.width / 2.0f, rect.x + rect.height / 2.0f };
		};

		auto rectSize = [&](const Rectangle& rect) -> Vec2 {
			return Vec2{ rect.width, rect.height };
		};

		auto makeRect = [&](Vec2 pos, Vec2 size) -> Rectangle {
			return Rectangle{ pos.x, pos.y, size.x, size.y };
		};
		
		auto centered = [&](const Rectangle& relativeTo, Vec2 size) -> Rectangle {
			const auto top = rectCenter(relativeTo) - size / 2.0f;
			return makeRect(top, size);
		};

		//What is GuiTabBar?
		const Rectangle screenRect{ 0.0f, 0.0f, static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight()) };
		static char input[255] = "";
		// There can only be a single GuiTextInputBox open at a time. It stores the selected state in an inaccessible static variable.
		int result = GuiTextInputBox(centered(screenRect, Vec2{ 500.0f, 100.0f }), "address", nullptr, "ok; cancel", input, sizeof(input), nullptr);
		/*if (result > 0) {
			std::cout << result << '\n';
		}*/
		if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_V)) {
			const auto length = strlen(input);
			snprintf(input + length, sizeof(input) - length, "%s", GetClipboardText());
		}
		if (result == 1 || im.data != nullptr) {
			alignas(alignof(ImageRgba)) static u8 image[sizeof(ImageRgba)];
			u64 address;
			if (sscanf(input, "%" SCNx64, &address) == 1) {
				SIZE_T bytesRead;
				/*CHECK_WIN_ZERO(ReadProcessMemory(debuggedProcessHandle, reinterpret_cast<u8*>(baseAddress) + address, image, sizeof(image), &bytesRead));*/
				CHECK_WIN_ZERO(ReadProcessMemory(debuggedProcessHandle, reinterpret_cast<void*>(address), image, sizeof(image), &bytesRead));
				auto img = reinterpret_cast<ImageRgba*>(&image);
				std::cout << "image size " << img->size() << '\n';
				im.width = img->size().x;
				im.height = img->size().y;
				im.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
				im.mipmaps = 1;
				im.data = malloc(img->dataSizeBytes());
				CHECK_WIN_ZERO(ReadProcessMemory(debuggedProcessHandle, img->data(), im.data, img->dataSizeBytes(), &bytesRead));
				texture = LoadTextureFromImage(im);
			}
		}
		static int address = 0;
		//GuiValueBox(centered(screenRect, Vec2{ 500.0f, 100.0f }), "address", &address, 0, 1000, true);

		if (im.data != nullptr) {
			DrawTextureEx(texture, Vector2{ 0.0f, 0.0f }, 0.0f, 5.0f, WHITE);
		}

		BeginMode2D(camera);

		DrawCircleLines(20, 20, 50, BLUE);
		//GuiDropdownBox(Rectangle{ 0, 0, 10, 10 }, "mode", );
		//GuiPanel(Rectangle{ 0, 0, static_cast<float>(GetScreenWidth()), 10 }, "test");
		//GuiTextInputBox()
		
		//DrawGrid(5, 0.2f);

		EndMode2D();

		EndDrawing();
	}

	CloseWindow();
}