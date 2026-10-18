#include "WorkRect.h"
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
		static Color BackColor = UserData::BackColor == Color(30, 30, 30) ? Color(20, 20, 20, 200) : Color(200, 200, 200, 200);
		XGraph::SetFillColor(Color(BackColor));
		static int StartY = ScreenSize.y * 0.01;

		static int lw = ScreenSize.x / 1920;
		XGraph::LineShape::SetLineWidth(lw);
		static Color LineColor = UserData::BackColor == Color(30, 30, 30) ? Color(100, 100, 100) : Color(100, 100, 255);
		XGraph::SetColor(Color(120, 120, 120));

		int x = SideBar::Level1::w;
		int w = WindowSize.x - x - UISPACE * 3;
		int h = WindowSize.y - StartY - BottomBar::h - UISPACE;

		XGraph::RectangleShape::FillRoundRect(x,StartY,w,h, ROUNDSIZE, window);
	}
}