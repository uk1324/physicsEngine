#include <appMain.hpp>
#include <windows.h>

#include <utils/asserts.hpp>

auto WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) -> int {
	return appMain();
}