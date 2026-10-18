#include "Shared.h"
#include "user/User.h"

Vector2i WindowSize, ScreenSize;

int FONTSIZE;

std::wstring ExePath = std::filesystem::current_path().wstring();

bool EnableDWM = false;

Color FONTCOLOR;

int ROUNDSIZE;

int UISPACE;

namespace Shared
{
	NOXS; NOSTD;
}