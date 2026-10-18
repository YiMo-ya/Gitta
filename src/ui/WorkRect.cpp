#include "WorkRect.h"
#include "SideBar.h"
#include "BottomBar.h"
#include "Shared.h"
#include "user/User.h"

namespace WorkRect
{
	NOXS; NOSTD;

	RenderTexture rt;

	bool NeedRedraw = true;

	//绘制主页
	void DrawHome(int w,int h)
	{

	}

	void Draw(RenWin& window)
	{
		static int StartY = ScreenSize.y * 0.01;
		int x = SideBar::Level1::w;

		//自动切换逻辑
		static EV YOffset;
		static int SideBaiIndexTemp = -1;
		if (SideBar::Level1::SideBarIndex != SideBaiIndexTemp)
		{
			SideBaiIndexTemp = SideBar::Level1::SideBarIndex;
			YOffset.SetAnimationStartValue(WindowSize.y / 3);
			YOffset.SetAnimation(0, 30);
		}

		YOffset.UpdateAnimation(XEase::EaseBasic::easeOut, 3);

		if(NeedRedraw || XMsg::WindowMsg::IsWindowResize() || YOffset.IsAnimation())
		{
			int w = WindowSize.x - x - UISPACE * 3;
			int h = WindowSize.y - StartY - BottomBar::h - UISPACE;

			if(rt.resize({unsigned int(w),unsigned int(h)}))
			{
				rt.clear(Color::Transparent);

				static Color BackColor = UserData::BackColor == Color(30, 30, 30) ? Color(20, 20, 20, 200) : Color(255, 255, 255, 200);
				XGraph::SetFillColor(Color(BackColor));

				static int lw = ScreenSize.x / 1920;
				XGraph::LineShape::SetLineWidth(lw);
				static Color LineColor = UserData::BackColor == Color(30, 30, 30) ? Color(100, 100, 100) : Color(100, 100, 255);
				XGraph::SetColor(Color(120, 120, 120));

				XGraph::RectangleShape::FillRoundRect(lw, lw, w - lw * 2, h - lw * 2, ROUNDSIZE, rt);

				if (SideBar::Level1::SideBarIndex == 0) DrawHome(w - lw * 2, h - lw * 2);

				rt.display();
			}

			NeedRedraw = false;
		}

		Sprite s(rt.getTexture());
		s.setPosition({ float(x),float(StartY) });
		window.draw(s);
	}
}