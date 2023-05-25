#include <messages.hpp>

// Not using struct initializers, because of an internal compiler error.
auto RefreshArray2dGridMessage::image32Grid(const void* data, Vec2T<i32> size) -> RefreshArray2dGridMessage {
	RefreshArray2dGridMessage image;
	image.type = Array2dType::IMAGE32;
	image.data = data;
	image.size = size;
	return image;
}

auto RefreshArray2dGridMessage::intGrid(const void* data, Vec2T<i32> size, Array2dType type, i32 intMin, i32 intMax, bool posXGoingRight, bool posYGoingUp) -> RefreshArray2dGridMessage {
	RefreshArray2dGridMessage image;
	image.type = type;
	image.data = data;
	image.size = size;
	image.intMin = intMin;
	image.intMax = intMax;
	image.posXGoingRight = posXGoingRight;
	image.posYGoingUp = posYGoingUp;
	return image;
}

auto RefreshArray2dGridMessage::floatGrid(const void* data, Vec2T<i32> size, Array2dType type, double floatMin, double floatMax, bool posXGoingRight, bool posYGoingUp) -> RefreshArray2dGridMessage {
	RefreshArray2dGridMessage image;
	image.type = type;
	image.data = data;
	image.size = size;
	image.floatMin = floatMin;
	image.floatMax = floatMax;
	image.posXGoingRight = posXGoingRight;
	image.posYGoingUp = posYGoingUp;
	return image;
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
