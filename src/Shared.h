#pragma once

#include "Xs/Xs.h"

extern Vector2i WindowSize, ScreenSize;

extern int FONTSIZE;

extern std::wstring ExePath;

extern bool EnableDWM;

extern Color FONTCOLOR;

extern int ROUNDSIZE;

extern int UISPACE;

namespace Shared
{
	void DrawBorder(RenWin& window);
}