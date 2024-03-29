/*
Async client.
Could use overlapped io, tried, but there were some weird issues with multiple connect messages being send I think.
Could just have a seperate thread on the server that would but the messages onto a queue.
It seems that it just just simpler to stay make the writes synchronous, because then there won't be any issues relating to that.
Also it seems that client NO_WAIT flag is completly obsolete. Tried setting it using the SetNamedPipeHandleState and the checking the state using the get function and it just doesn't get set.

DON'T set the update rate. Currently the codes just reads a single message and the updates contitnues with the loop, which means the next messages will get read on the next frame so the client has to wait. This can be modified to check if there are any messages pending, but if the messages is completed fast enought the client might still need to pointlesly wait. A complete solution would obviously be to implement something asnychronous.

Creating a separate thread is not worth it. The app sends too many messages and this overflows the message queue. To fix this its best to have a synchronization point that switches 
// If the app sends too many messages the debugger might not be fast enought to process them all before new messages come which would overflow the queue. It might be good to either but a cap on the number of messages or maybe create a synchoronization point for example on the clear screen message.
*/

#include <utils/imageRgba.hpp>
#include <raylib.h>

#include <imgui/imgui.h>

#include <winUtils.hpp>
#include <messages.hpp>
#include <iostream>
#include <fstream>
#include <thread>
#include <mutex>
#include <queue>
#include <math/aabb.hpp>
#include <utils/refOptional.hpp>
#include <utils/overloaded.hpp>

auto to(Vec2 v) -> Vector2 {
	return Vector2{ v.x, v.y };
}

// Raylib name conflicts: https://github.com/raysan5/raylib/issues/1217
#define NOGDI
#define NOUSER
#define WIN32_LEAN_AND_MEAN 
#define NOMINMAX
#include <windows.h>
#include <psapi.h>
#include <inttypes.h>

#include <filesystem>
namespace fs = std::filesystem;

#include <inttypes.h>

#include <imgui/imgui_impl_win32.h>
#include <imgui/imgui_impl_opengl3.h>

#include <init.hpp>

// Memory locations of libraries and the executable can differ between executions.
// The base address is only used for reading static memory locations.
// // https://stackoverflow.com/questions/14467229/get-base-address-of-process
auto getProcessBaseAddress(HANDLE processHandle) -> std::optional<HMODULE> {
	// https://stackoverflow.com/questions/14467229/get-base-address-of-process
	char executableName[1024] = "";
	CHECK_WIN_ZERO(GetProcessImageFileNameA(processHandle, executableName, sizeof(executableName)));
	HMODULE modules[1024];
	DWORD sizeNeededModules;
	if (EnumProcessModules(processHandle, modules, sizeof(modules), &sizeNeededModules)) {
		ASSERT(sizeNeededModules <= sizeof(modules)); // Error handling?
		for (int i = 0; i < sizeNeededModules / sizeof(HMODULE); i++) {
			char moduleFilename[1024] = "";
			CHECK_WIN_ZERO(GetModuleFileNameExA(processHandle, modules[i], moduleFilename, sizeof(moduleFilename)));
			// This isn't a very good way to compare the names, because someone could impersonate the library. GetProcessImageFileNameA outputs a name with something like \\HardDrive\\[...]\\<actual path> so I am not sure what part to compare.
			// TODO: Shouldn't there be a built in win32 function to compare paths?
			if (fs::path(executableName).stem() == fs::path(moduleFilename).stem()) {
				return modules[i];
			}
		}
	}
	ASSERT_NOT_REACHED();
	return std::nullopt;
}

std::vector<u8> copyBuffer;

HANDLE debuggedProcessHandle = nullptr;

auto readMemory(void* dst, const void* src, size_t size) -> bool {
	SIZE_T bytesRead;
	if (!ReadProcessMemory(debuggedProcessHandle, src, dst, size, &bytesRead)) {
		std::cout << GetLastError() << '\n';
		return false;
	}
	if (bytesRead != size) {
		ASSERT_NOT_REACHED();
		return false;
	}
	return true;
}

auto readMemoryToCopyBuffer(const void* src, size_t size) -> bool {
	if (size > copyBuffer.size()) {
		copyBuffer.resize(size);
	}
	return readMemory(copyBuffer.data(), src, size);
}

auto textureFromBuffer(u8* pixelData, Vec2T<i64> size) -> Texture {
	Image img;
	img.width = static_cast<int>(size.x);
	img.height = static_cast<int>(size.y);
	img.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
	img.mipmaps = 1;
	img.data = pixelData;
	return LoadTextureFromImage(img);
}

auto textureSize(const Texture& texture) -> Vec2 {
	return Vec2{ static_cast<float>(texture.width), static_cast<float>(texture.height) };
}

struct DebuggedImage {
	std::string windowName;
	Array2dType type;
	union {
		struct {
			i32 intMin, intMax;
		};
		struct {
			double floatMin, floatMax;
		};
	};
	Texture texture;
	const void* address = nullptr;
	bool autoRefresh = false;
	bool isWindowOpen = true;
	bool posYGoingUp;
	bool posXGoingRight;

private:
	DebuggedImage(const DebuggedImage&);
	auto operator=(const DebuggedImage&) -> DebuggedImage& = default;
public:

	DebuggedImage(DebuggedImage&& other) noexcept {
		*this = other;
		other.texture.id = 0;
	}
	auto operator=(DebuggedImage&& other) noexcept -> DebuggedImage& {
		*this = other;
		other.texture.id = 0;
		return *this;
	}

	DebuggedImage(
		std::string_view windowName, 
		const void* address, 
		Array2dType type, 
		Vec2T<i64> size, 
		bool posXGoingRight, 
		bool posYGoingUp)
		: windowName{ windowName }
		, address{ address }
		, type{ type }
		, posXGoingRight{ posXGoingRight }
		, posYGoingUp{ posYGoingUp } {
		// @Hack: There is no way to create a buffer without allocating. This should matter because the buffer would need to be resized later anyaway.
		if (const auto pixelBufferSize = size.x * size.y * sizeof(u32); pixelBufferSize > copyBuffer.size()) {
			copyBuffer.resize(pixelBufferSize);
		}
		texture = textureFromBuffer(copyBuffer.data(), size);
		refresh();
	}

	auto refresh() -> bool {
		if (type == Array2dType::IMAGE32) {
			readMemoryToCopyBuffer(address, size().x * size().y * sizeof(u32));
			UpdateTexture(texture, copyBuffer.data());
		} else {
			auto array2dDataTypeSizeBytes = [](Array2dType t) -> usize {
				switch (t) {
				case Array2dType::IMAGE32: ASSERT_NOT_REACHED(); return 0;
				case Array2dType::U8: return 1;
				case Array2dType::F32: return 4;
					break;
				}
			};

			const auto arrayDataSize = size().x * size().y * array2dDataTypeSizeBytes(type);
			readMemoryToCopyBuffer(address, arrayDataSize);
			copyBuffer.resize(copyBuffer.size() + arrayDataSize * sizeof(PixelRgba));
			const auto imageData = reinterpret_cast<PixelRgba*>(copyBuffer.data() + arrayDataSize);
			for (i32 y = 0; y < size().y; y++) {
				for (i32 x = 0; x < size().x; x++) {
					// Flip the data here or in outisde the switch later.
					i64 gridX = posXGoingRight ? x : (size().x - x - 1);
					i64 gridY = posYGoingUp ? y : (size().y - y - 1);
					const auto gridOffset = gridX * size().y + gridY;
					auto& outPixel = imageData[y * size().x + x];
					switch (type)
					{
					case Array2dType::IMAGE32: ASSERT_NOT_REACHED(); break;
					case Array2dType::U8: {
						const auto data = reinterpret_cast<u8*>(copyBuffer.data());
						const auto value = data[gridOffset];
						outPixel = PixelRgba::scientificColoring(value, static_cast<float>(intMin), static_cast<float>(intMax));
						break;
					}
						 
					case Array2dType::F32: {
						const auto data = reinterpret_cast<float*>(copyBuffer.data());
						const auto value = data[gridOffset];
						outPixel = PixelRgba::scientificColoring(value, static_cast<float>(floatMin), static_cast<float>(floatMax));
						break;
					}
						
					}
				}
			}
			UpdateTexture(texture, imageData);
		}
		//switch (type) {
		//case Array2dType::IMAGE32:
		//	readMemoryToCopyBuffer(address, size().x * size().y * sizeof(u32));
		//	UpdateTexture(texture, copyBuffer.data());
		//	break;

		//case Array2dType::U8:
		//	const auto arrayDataSize = size().x * size().y;
		//	readMemoryToCopyBuffer(address, arrayDataSize);
		//	copyBuffer.resize(copyBuffer.size() + arrayDataSize * sizeof(u32));
		//	const auto arrayData = reinterpret_cast<u8*>(copyBuffer.data());
		//	const auto imageData = reinterpret_cast<PixelRgba*>(copyBuffer.data() + arrayDataSize);
		//	for (i32 y = 0; y < size().y; y++) {
		//		for (i32 x = 0; x < size().x; x++) {
		//			// Flip the data here or in outisde the switch later.
		//			i64 gridX = posXGoingRight ? x : (size().x - x - 1);
		//			i64 gridY = posYGoingUp ? y : (size().y - y - 1);
		//			const auto value = arrayData[gridX * size().y + gridY];
		//			imageData[y * size().x + x] = PixelRgba::scientificColoring(value, intMin, intMax);
		//		}
		//	}
		//	UpdateTexture(texture, imageData);
		//	break;
		//}

		return true;
	}

	auto size() -> Vec2T<i64> {
		return Vec2T<i64>{ texture.width, texture.height };
	}

	~DebuggedImage() {
		UnloadTexture(texture);
	}
};

std::vector<DebuggedImage> debuggedImages;

auto debuggedImagesFind(std::string_view windowName) -> std::optional<DebuggedImage&> {
	auto it = std::find_if(debuggedImages.begin(), debuggedImages.end(), [&](const DebuggedImage& img) { 
		return img.windowName == windowName; 
	});
	if (it == debuggedImages.end()) {
		return std::nullopt;
	}
	return *it;
};

bool connected = false;

HANDLE serverHandle = INVALID_HANDLE_VALUE;

// Needs to be thread safe. GetLastError is thread safe.
auto readMessage(void* output, DWORD size) -> bool {
	DWORD bytesRead;
	if (ReadFile(serverHandle, output, size, &bytesRead, nullptr)) {
		ASSERT(bytesRead == size);
		return true;
	} else if (connected && GetLastError() == ERROR_BROKEN_PIPE) {
		std::cout << "client disconnected\n";
		connected = false;
		debuggedImages.clear();
	} else if (GetLastError() == ERROR_NO_DATA) {
		return false;
	}
	return false;
};

template<typename T>
auto readDebuggerMessageAfterHeader(T* output) -> void {
	readMessage(reinterpret_cast<u8*>(output) + sizeof(DebuggerMessageType), sizeof(T) - sizeof(DebuggerMessageType));
}

auto drawUi() -> void {
	using namespace ImGui;

	const Vec2 windowSize{ static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight()) };

	Begin("tool");
	if (BeginTabBar("test")) {

		if (BeginTabItem("image")) {
			static_assert(sizeof(u64) == sizeof(intptr_t));
			static void* address = 0;
			static char windowNameInput[256] = "";
			static Vec2T<i64> size{ 1, 1 };
			InputText("window name", windowNameInput, sizeof(windowNameInput));
			InputScalar("address", ImGuiDataType_U64, &address, nullptr, nullptr, "%p", ImGuiInputTextFlags_CharsHexadecimal);
			InputScalarN("size", ImGuiDataType_S64, size.data(), 2);
			size = size.clamped(Vec2T<i64>{ 1 }, Vec2T<i64>{ 10000 });
			// TODO: Choose array type, automatically read size from ImageRgba.
			if (Button("add")) {
				if (size.x < 0 || size.y < 0) {
					size.x = 0;
					size.y = 0;
				} else if (!debuggedImagesFind(windowNameInput).has_value()) {
					debuggedImages.push_back(DebuggedImage{ windowNameInput, address, Array2dType::IMAGE32, size, true, true });
				}
			}

			BeginChild("images");
			for (int i = static_cast<int>(debuggedImages.size()) - 1; i >= 0; i--) {
				auto& img = debuggedImages[i];
				Text("%p", img.address);
				SameLine();
				PushID(img.texture.id);
				if (Button("open")) {
					img.isWindowOpen = true;
				}
				SameLine();
				if (Button("remove")) {
					debuggedImages.erase(debuggedImages.begin() + i);
				}
				PopID();
				SameLine();
				Checkbox("auto refresh", &img.autoRefresh);
			}
			EndChild();
			EndTabItem();
		}

		EndTabBar();
	}

	End();

	for (auto& img : debuggedImages) {
		if (!img.isWindowOpen) {
			continue;
		}

		auto aspectRatioConstraint = [](ImGuiSizeCallbackData* data) -> void {
			// https://github.com/ocornut/imgui/issues/6210
			float aspect_ratio = *reinterpret_cast<float*>(data->UserData);
			data->DesiredSize.x = std::max(data->DesiredSize.x, data->DesiredSize.y);
			data->DesiredSize.y = (float)(int)(data->DesiredSize.x / aspect_ratio);
		};
		float aspectRatio = textureSize(img.texture).xOverY();
		SetNextWindowSizeConstraints(Vec2{ 0.0f }, Vec2{ INFINITY }, aspectRatioConstraint, &aspectRatio);

		const auto textureSize = ::textureSize(img.texture);
		SetNextWindowSize(Vec2{ windowSize.x * 0.3f, windowSize.x * 0.3f / textureSize.xOverY() }, ImGuiCond_Appearing);
		Begin(img.windowName.c_str(), &img.isWindowOpen);

		const auto sceneWindowWindowSpace = Aabb::fromCorners(
			Vec2{ ImGui::GetWindowPos() } + ImGui::GetWindowContentRegionMin(),
			Vec2{ ImGui::GetWindowPos() } + ImGui::GetWindowContentRegionMax()
		);
		const auto sceneWindowSize = sceneWindowWindowSpace.size();
		ImGui::Image(reinterpret_cast<void*>(static_cast<intptr_t>(img.texture.id)), sceneWindowSize);
		End();
		if (img.autoRefresh) {
			img.refresh();
		}
	}
}

struct DebuggerLine {
	Vec2 start;
	Vec2 end;
	Vec3 color;
};

std::vector<DebuggerLine> lines;
std::vector<DebuggerLine> nextFrameLines;
std::queue<DebuggerMessage> debuggerMessageQueue;
//std::mutex messageQueueMutex;

#include <utils/timer.hpp>

auto main() -> int {
	SetTraceLogLevel(LOG_DEBUG | LOG_INFO | LOG_WARNING);
	// PIPE_NOWAIT makes it so the pipe returns immediately if there is no data and sets last error to ERROR_NO_DATA. Tried using PeekNamedPipe to check if there is data before reading, but it errors if there is no connection client connected yet, also I couldn't find a function that check if a any client is connected the server.
	serverHandle = CreateNamedPipeA(
		"\\\\.\\pipe\\debugger",
		PIPE_ACCESS_INBOUND,
		PIPE_TYPE_MESSAGE | PIPE_NOWAIT,
		1,
		0, // Server doesn't send any messages so output buffer size = 0
		sizeof(2048),
		0,
		nullptr
	);
	CHECK_WIN_HANDLE(serverHandle);

	u8* baseAddress; // Probably only useful for reading static values.
	std::ostream msgLog{ std::cout.rdbuf() };
	std::fstream nullStream; // An unopened fstream doesn't output anywhere.
	static constexpr auto DISABLE_MSG_LOG = true;
	if constexpr (DISABLE_MSG_LOG) {
		msgLog.set_rdbuf(nullStream.rdbuf());
	}

	auto updateServer = [&]() -> void {
		//for (;;) {
		//	{
		//		DebuggerMessageType messageType;

		//		if (!readMessage(&messageType, sizeof(messageType))) {
		//			continue;
		//			//return;
		//		}
		//		static float x = 0.0f;
		//		Timer timer;
		//		if (timer.elapsedMilliseconds() > x) {
		//			x = timer.elapsedMilliseconds();
		//			std::cout << x << '\n';
		//		}

		//		#define READ_AND_BREAK(MsgType) { \
		//			MsgType msg; \
		//			readDebuggerMessageAfterHeader(&msg); \
		//			debuggerMessageQueue.push(msg); \
		//			break; \
		//		}
		//		switch (messageType) {
		//		case DebuggerMessageType::CONNECT: READ_AND_BREAK(ConnectMessage);
		//		case DebuggerMessageType::REFRESH_ARRAY_2D: READ_AND_BREAK(RefreshArray2dGridMessage)
		//		case DebuggerMessageType::CLEAR_SCREEN: READ_AND_BREAK(ClearScreenMessage)
		//		case DebuggerMessageType::DRAW_LINE: READ_AND_BREAK(DrawLineMessage)
		//		}
		//	}

		//	if (debuggerMessageQueue.empty()) {
		//		continue;
		//	}

		//	const auto message = debuggerMessageQueue.back();
		//	debuggerMessageQueue.pop();

		//	if (const auto connect = std::get_if<ConnectMessage>(&message)) {
		//		if (connected) {
		//			ASSERT_NOT_REACHED()
		//		} else {
		//			debuggedProcessHandle = OpenProcess(PROCESS_VM_READ | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION, false, connect->processId);
		//			// Should it assume the client disconnected because it closed? Should it handle it?
		//			CHECK_WIN_ZERO(debuggedProcessHandle);
		//			const auto address = getProcessBaseAddress(debuggedProcessHandle);
		//			if (address.has_value()) {
		//				baseAddress = reinterpret_cast<u8*>(*address);
		//			} else {
		//				baseAddress = nullptr;
		//			}
		//		}
		//	} else if (connected) {
		//		std::cout << "message received before connecting\n";
		//	} else {
		//		bool endProcessingMessagesForThisFrame = false;

		//		std::visit(overloaded{
		//			[](const ConnectMessage& connect) {},
		//			[](const RefreshArray2dGridMessage& refresh) {
		//				readMemoryToCopyBuffer(refresh.windowName.data(), refresh.windowName.size());
		//				std::string_view windowName{ reinterpret_cast<const char*>(copyBuffer.data()), refresh.windowName.size() };

		//				auto image = debuggedImagesFind(windowName);
		//				if (image.has_value()) {
		//					image->refresh();
		//				} else {
		//					debuggedImages.push_back(DebuggedImage{ windowName, refresh.data, refresh.type, Vec2T<i64>{ refresh.size }, refresh.posXGoingRight, refresh.posYGoingUp });
		//					auto& image = debuggedImages.back();
		//					if (array2dTypeIsInt(refresh.type)) {
		//						image.intMin = refresh.intMin;
		//						image.intMax = refresh.intMax;
		//					} else {
		//						image.floatMin = refresh.floatMin;
		//						image.floatMax = refresh.floatMax;
		//					}
		//				}
		//			},
		//			[&](const ClearScreenMessage& clear) { 
		//				std::swap(lines, nextFrameLines);
		//				nextFrameLines.clear();
		//				endProcessingMessagesForThisFrame = true;
		//			},
		//			[](const DrawLineMessage& draw) {
		//				nextFrameLines.push_back(DebuggerLine{ draw.start, draw.end, draw.color });
		//			}
		//		}, message);
		//		if (endProcessingMessagesForThisFrame)
		//			return;
		//	}
		//}
		// https://stackoverflow.com/questions/14467229/get-base-address-of-process


		// The standard says that padding is never added before the first member.
		DebuggerMessageType messageType;

		if (!readMessage(&messageType, sizeof(messageType))) {
			return;
		}

		// https://stackoverflow.com/questions/14467229/get-base-address-of-process
		if (messageType == DebuggerMessageType::CONNECT) {
			msgLog << "connect message\n";
			if (connected) {
				ASSERT_NOT_REACHED();
			} else {
				ConnectMessage connect;
				readDebuggerMessageAfterHeader(&connect);
				std::cout << "client " << connect.processId << " connected\n";
				connected = true;

				debuggedProcessHandle = OpenProcess(PROCESS_VM_READ | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION, false, connect.processId);
				// Should it assume the client disconnected because it closed? Should it handle it?
				CHECK_WIN_ZERO(debuggedProcessHandle);

				auto address = getProcessBaseAddress(debuggedProcessHandle);
				if (address.has_value()) {
					baseAddress = reinterpret_cast<u8*>(*address);
				} else {
					baseAddress = nullptr;
				}
			}

		} else if (connected == false) {
			std::cout << "message received before connecting\n";
		} else {
			switch (messageType) {

			case DebuggerMessageType::REFRESH_ARRAY_2D: {
				RefreshArray2dGridMessage refresh;
				readDebuggerMessageAfterHeader(&refresh);
				msgLog << "REFRESH_ARRAY_2D " << refresh.data << '\n';

				readMemoryToCopyBuffer(refresh.windowName.data(), refresh.windowName.size());
				std::string_view windowName{ reinterpret_cast<const char*>(copyBuffer.data()), refresh.windowName.size() };
				
				auto image = debuggedImagesFind(windowName);
				if (image.has_value()) {
					image->refresh();
				} else {
					debuggedImages.push_back(DebuggedImage{ windowName, refresh.data, refresh.type, Vec2T<i64>{ refresh.size }, refresh.posXGoingRight, refresh.posYGoingUp });
					auto& image = debuggedImages.back();
					if (array2dTypeIsInt(refresh.type)) {
						image.intMin = refresh.intMin;
						image.intMax = refresh.intMax;
					} else {
						image.floatMin = refresh.floatMin;
						image.floatMax = refresh.floatMax;
					}
				}
				break;
			}

			case DebuggerMessageType::CLEAR_SCREEN: {
				ClearScreenMessage clearScreen;
				readDebuggerMessageAfterHeader(&clearScreen);
				lines.clear();
				break;
			}

			case DebuggerMessageType::DRAW_LINE: {
				DrawLineMessage drawLine;
				readDebuggerMessageAfterHeader(&drawLine);
				lines.push_back(DebuggerLine{ drawLine.start, drawLine.end, drawLine.color });
				break;
			}

			case DebuggerMessageType::CONNECT: break;
			default:
				std::cout << "invalid message type\n";
			}
		}
	};

	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(640, 480, "game");
	SetWindowState(FLAG_WINDOW_MAXIMIZED);

	initImGui(GetWindowHandle());

	Camera2D camera{
		.offset = Vector2{ 0.0f, 0.0f },
		.target = Vector2{ 0.0f, 0.0f },
		.rotation = 0.0f,
		.zoom = 1.0f,
	};
	//SetTargetFPS(60);

	std::optional<Vec2> screenGrabStartPos;

	static constexpr int MOUSE_RIGHT = 1;

	auto vec2 = [](Vector2 v) -> Vec2 {
		return Vec2{ v.x, v.y };
	};

	auto vector2 = [](Vec2 v) -> Vector2 {
		return Vector2{ .x = v.x, .y = v.y };
	};

	auto cameraSpaceCursorPos = [&]() -> Vec2 {
		return vec2(GetScreenToWorld2D(GetMousePosition(), camera));
	};

	int currentFrame = 0;
	while (!WindowShouldClose()) {
		currentFrame++;
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_NoDockingInCentralNode | ImGuiDockNodeFlags_PassthruCentralNode);

		if (IsMouseButtonPressed(MOUSE_RIGHT)) {
			screenGrabStartPos = cameraSpaceCursorPos();
		}
		if (screenGrabStartPos.has_value()) {
			const auto currentCursorPos = cameraSpaceCursorPos();
			camera.offset = vector2(vec2(camera.offset) - (*screenGrabStartPos - currentCursorPos) * camera.zoom);
		}
		if (IsMouseButtonReleased(MOUSE_RIGHT)) {
			screenGrabStartPos = std::nullopt;
		}

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
			const auto cursorPosBeforeScroll = cameraSpaceCursorPos();
			const auto oldZoom = camera.zoom;
			if (scroll < 0.0f) {
				camera.zoom /= 2.0f;
			} else {
				camera.zoom *= 2.0f;
			}
			const auto cursorPosAfterScroll = cameraSpaceCursorPos();
			// !!!!
			// The weird values in the calculations might be because i am not setting target of the camera correctly.
			camera.offset = vector2(vec2(camera.offset) - (cursorPosBeforeScroll - cursorPosAfterScroll) * camera.zoom);
		}

		BeginDrawing();
		ClearBackground(BLACK);


		drawUi();

		BeginMode2D(camera);

		DrawCircleLines(20, 20, 50, BLUE);
		ImGui::Text("%d", nextFrameLines.size());
		ImGui::Text("%d", lines.size());
		ImGui::Text("%d", lines.size() + nextFrameLines.size());
		for (auto line : lines) {
			auto convertPos = [](Vec2 v) -> Vec2 {
				v *= 100.0f;
				v.y = -v.y;
				return v;
			};
			line.start = convertPos(line.start);
			line.end = convertPos(line.end);
			const PixelRgba color{ line.color };
			DrawLine(
				static_cast<int>(line.start.x), 
				static_cast<int>(line.start.y), 
				static_cast<int>(line.end.x), 
				static_cast<int>(line.end.y), 
				Color{ color.r, color.g, color.b, 255 });
			//DrawLineEx(vector2(line.start), vector2(line.end), 1, Color{ color.r, color.g, color.b, 255 });
		}

		EndMode2D();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		EndDrawing();
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	CloseWindow();
}