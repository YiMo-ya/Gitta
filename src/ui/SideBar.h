#pragma once
#include "Xs/Xs.h"

namespace SideBar
{
	namespace Level1
	{
		extern int SideBarIndex;
		extern int w;

		void Draw(RenWin& window);

		void DrawTips(RenWin& window);
	}

	namespace Level2
	{
		extern bool NeedRedraw;
		extern xs::EV SideBarLevel2W;
		void Draw(RenWin& window);
	}
}