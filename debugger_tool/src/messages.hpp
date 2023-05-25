#pragma once

#include <utils/int.hpp>
#include <utils/imageRgba.hpp>

// If there are issues with alignment could use pragma pack. And don't use inheritance
enum class DebuggerMessageType : u8 {
	CONNECT,
	REFRESH_IMAGE,
	REFRESH_ARRAY_2D
};

struct ConnectMessage {
	DebuggerMessageType msg = DebuggerMessageType::CONNECT;
	u32 processId = 0;
};

// Not having a special case for ImageRgba Array2ds is better because you don't have to prevent against invalid data reads of the image size. This can happen if the debugged process is closing. 

enum class Array2dType {
	IMAGE32, U8
};

struct RefreshArray2dGridMessage {
	DebuggerMessageType msg = DebuggerMessageType::REFRESH_ARRAY_2D;
	Array2dType type;
	const void* data;
	Vec2T<i32> size;
	union {
		struct {
			// Useful for limited ranges and enums
			i32 intMin, intMax;
		};
		struct {
			float floatMin, floatMax;
		};
	};
	bool posXGoingRight;
	bool posYGoingUp;

	static auto image32Grid(const void* data, Vec2T<i32> size) -> RefreshArray2dGridMessage;
	static auto intGrid(const void* data, Vec2T<i32> size, Array2dType type, i32 intMin, i32 intMax, bool posXGoingRight, bool posYGoingUp) -> RefreshArray2dGridMessage;
};