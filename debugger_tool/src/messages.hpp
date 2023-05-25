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
	IMAGE32, U8, F32
};

auto array2dTypeIsInt(Array2dType t) -> bool;

// Could have a different api. Instead of a single message type that refreshes the image. 
// Could make 3 types
// AddImage(windowName, struct of info)
// Refresh(windowName)
// Update(struct of optional info changes) // changes the debugger image state like address, type, min, max.
// This api would allow the debugger to modify the values on it's side like min max, array direction (currently this is info is sent on every frame), but the state of the app and the debugger wouldn't be synchronized.
// What info should be kept on the debugger side only and which sent by the app. Should the app sent only the intial info?
// For now I am going to choose the simplest option. Treat the first Refresh message as an add message and the rest as refresh messages (even if the data changes). This will allow modifying the state on the debugger side. For now the debugger side state changes won't be persisent.
struct RefreshArray2dGridMessage {
	DebuggerMessageType msg = DebuggerMessageType::REFRESH_ARRAY_2D;
	// Storing the window name so the window positions are preserved between program launches by ImGui.
	// It would be nice for the window id generation this to be automatic, but I couldn't come up with anything good.
	// The array data addresses change every launch.
	// Line numbers change.
	// Call order changes.
	// An alternative would be to automatically lay out window positions to make them not overlap, but it is probably better if the user positions the windows themselves.
	std::string_view windowName;
	Array2dType type;
	const void* data;
	Vec2T<i32> size;
	union {
		struct {
			// Useful for limited ranges and enums
			i32 intMin, intMax;
		};
		struct {
			double floatMin, floatMax;
		};
	};
	bool posXGoingRight;
	bool posYGoingUp;

	static auto image32Grid(std::string_view windowName, const void* data, Vec2T<i32> size) -> RefreshArray2dGridMessage;
	static auto intGrid(
		std::string_view windowName,
		const void* data, 
		Vec2T<i32> size, 
		Array2dType type, 
		i32 intMin, 
		i32 intMax, 
		bool posXGoingRight, 
		bool posYGoingUp) -> RefreshArray2dGridMessage;

	static auto floatGrid(
		std::string_view windowName,
		const void* data,
		Vec2T<i32> size,
		Array2dType type,
		double floatMin,
		double floatMax,
		bool posXGoingRight,
		bool posYGoingUp) -> RefreshArray2dGridMessage;
};