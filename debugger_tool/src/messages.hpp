#pragma once

#include <utils/int.hpp>

// If there are issues with alignment could use pragma pack. And don't use inheritance
enum class DebuggerMessageType : u8 {
	CONNECT,
};

struct ConnectMessage {
	DebuggerMessageType msg = DebuggerMessageType::CONNECT;
	u32 processId;
	//void* levelDataA;
};