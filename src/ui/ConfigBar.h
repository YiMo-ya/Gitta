#pragma once
#include "Xs/Xs.h"

namespace ConfigBar
{
	extern xs::EV ConfigBarW;

	extern bool NeedRedraw;

	void Draw(RenWin& window);
}