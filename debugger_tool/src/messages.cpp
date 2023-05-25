#include <messages.hpp>

// Not using struct initializers, because of an internal compiler error.
auto RefreshArray2dGridMessage::image32Grid(std::string_view windowName, const void* data, Vec2T<i32> size) -> RefreshArray2dGridMessage {
	RefreshArray2dGridMessage msg;
	msg.windowName = windowName;
	msg.type = Array2dType::IMAGE32;
	msg.data = data;
	msg.size = size;
	return msg;
}

auto RefreshArray2dGridMessage::intGrid(std::string_view windowName, const void* data, Vec2T<i32> size, Array2dType type, i32 intMin, i32 intMax, bool posXGoingRight, bool posYGoingUp) -> RefreshArray2dGridMessage {
	ASSERT(array2dTypeIsInt(type));
	RefreshArray2dGridMessage msg;
	msg.windowName = windowName;
	msg.type = type;
	msg.data = data;
	msg.size = size;
	msg.intMin = intMin;
	msg.intMax = intMax;
	msg.posXGoingRight = posXGoingRight;
	msg.posYGoingUp = posYGoingUp;
	return msg;
}

auto RefreshArray2dGridMessage::floatGrid(std::string_view windowName, const void* data, Vec2T<i32> size, Array2dType type, double floatMin, double floatMax, bool posXGoingRight, bool posYGoingUp) -> RefreshArray2dGridMessage {
	ASSERT(!array2dTypeIsInt(type));
	RefreshArray2dGridMessage msg;
	msg.windowName = windowName;
	msg.type = type;
	msg.data = data;
	msg.size = size;
	msg.floatMin = floatMin;
	msg.floatMax = floatMax;
	msg.posXGoingRight = posXGoingRight;
	msg.posYGoingUp = posYGoingUp;
	return msg;
}

auto array2dTypeIsInt(Array2dType t) -> bool {
	switch (t) {
	case Array2dType::IMAGE32:
	case Array2dType::U8:
		return true;
	case Array2dType::F32:
		return false;
	}
}
