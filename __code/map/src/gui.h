#pragma once

#include "raylib.h"

namespace GUI {
	bool inBounds(Vector2 mousePos, const int screenWidth, const int screenHeight, const int toolbarWidth) {
		return mousePos.x < (screenWidth - toolbarWidth);
	}
}