#pragma once

#include <utils/int.hpp>
#include <utils/imageRgba.hpp>

// If there are issues with alignment could use pragma pack. And don't use inheritance
enum class DebuggerMessageType : u8 {
	CONNECT,
	REFRESH_IMAGE,
	REFRESH_GRID
};

struct ConnectMessage {
	DebuggerMessageType msg = DebuggerMessageType::CONNECT;
	u32 processId = 0;
};

struct RefreshImageMessage {
	DebuggerMessageType msg = DebuggerMessageType::REFRESH_IMAGE;
	const ImageRgba* img = nullptr;
};

enum class GridType {
	U8
};

struct RefreshGridMessage {
	DebuggerMessageType msg = DebuggerMessageType::REFRESH_GRID;
	GridType type;
	void* data;
	Vec2T<i32> size;
	//union {
	//	struct {
	//		i32 intMin, intMax;
	//	};
	//	/*struct {
	//		float floatMin, floatMax;
	//	};*/
	//};
};