#include "Search.h"
#include "SideBar.h"
#include "Shared.h"
#include "UI.h"
#include "user/Set.h"

namespace Search
{
	NOXS; NOSTD;

	int SearchBoxH;

	void Draw(RenWin& window)
	{
		static string SearchText;

		//位置
		int x = SideBar::Level1::w;
		static int y = ScreenSize.y * 0.01;
		int w = WindowSize.x / 2;

		static UI::InPutRectUI ui;
		ui.x = x, ui.y = y, ui.w = w, ui.h = SearchBoxH;
		ui.fx = x + UISPACE, ui.fy = y + SearchBoxH / 2;
		ui.tips = "> 键入以搜索";
		ui.prefix = "> ";

		static bool init = false;
		if (!init)
		{
			SearchBoxH = ScreenSize.y  * 0.04;

			init = true;
		}

		//绘制底色
		static Color FillColor = Setting::BackColor == Color(30, 30, 30) ? Color(20, 20, 20, 200) : Color(255, 255, 255, 200);
		static Color BorderColor = Setting::BackColor == Color(30, 30, 30) ? Color(100, 100, 100, 255) : Color(200, 200, 200, 255);
		static int lw = ScreenSize.x / 1920.0;
		static int BorderAlpha;
		XGraph::SetFillColor(FillColor);
		XGraph::SetColor(BorderColor);
		XGraph::LineShape::SetLineWidth(lw);
		XGraph::RectangleShape::FillRoundRect(x, y, w, SearchBoxH, ROUNDSIZE, window);
		if (BorderAlpha > 0)
		{
			static Color color = Setting::MainColor;
			color.a = BorderAlpha;
			XGraph::SetColor(color);
			XGraph::RectangleShape::RoundRect(x, y, w, SearchBoxH, ROUNDSIZE, window);
		}

		static int SPEED = 50;
		if (XMsg::MouseMsg::IsMouseIn(x, y, w, SearchBoxH) || ui.IsCin)
		{
			if (BorderAlpha + SPEED < 255) BorderAlpha += SPEED;
			else BorderAlpha = 255;
		}
		else
		{
			if (BorderAlpha - SPEED > 0) BorderAlpha -= SPEED;
			else BorderAlpha = 0;
		}

		XText::SetFontAdjust(ADJUST_LEFT, ADJUST_CENTER);
		XText::SetFontSize(FONTSIZE);

		//输入逻辑
		ui.Draw(window, SearchText);
	}
}