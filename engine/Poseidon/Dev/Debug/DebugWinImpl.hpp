#pragma once

#include <Poseidon/Foundation/Common/Win.h>
#include <Poseidon/Dev/Debug/DebugWin.hpp>

namespace Poseidon::Dev
{

class OnPaintContext
{
	public:
	HDC dc;
};

HWND GetWindowHandle( DebugWindow *window );

} // namespace Poseidon::Dev
