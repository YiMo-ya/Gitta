#include "WorkRect.h"
#include "Menu.h"
#include "SideBar.h"
#include "BottomBar.h"
#include "Shared.h"
#include "user/User.h"

namespace WorkRect
{
	NOXS; NOSTD;

	bool NeedRedraw;

	void Draw(RenWin& window)
	{
		if (EnableDWM)
		{
			static Color BackColor = UserData::BackColor == Color(30, 30, 30) ? Color(40, 40, 40, 200) : Color(255, 255, 255, 200);
			XGraph::SetFillColor(Color(BackColor));
			XGraph::RectangleShape::FillRect_WithoutBorder(0, 0, WindowSize.x, WindowSize.y, window);
		}

		static Color BackColor = UserData::BackColor == Color(30, 30, 30) ? Color(20, 20, 20, 200) : Color(200, 200, 200, 200);
		XGraph::SetFillColor(Color(BackColor));
		static int y = Menu::h;

		static int lw = ScreenSize.x / 1920;
		XGraph::LineShape::SetLineWidth(lw);
		static Color LineColor = UserData::BackColor == Color(30, 30, 30) ? Color(100, 100, 100) : Color(100, 100, 255);
		XGraph::SetColor(Color(120, 120, 120));

		int x = SideBar::Level1::w;
		if (SideBar::Level2::SideBarLevel2W.value > 0) x += SideBar::Level2::SideBarLevel2W.value + UISPACE;
		int w = WindowSize.x - x - UISPACE * 3;
		int h = WindowSize.y - y - BottomBar::h - UISPACE;

		XGraph::RectangleShape::FillRoundRect(x,y,w,h, ROUNDSIZE, window);
	}
}