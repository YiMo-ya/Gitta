#include "BottomBar.h"

#include "Shared.h"
#include "user/Set.h"

namespace BottomBar
{
	NOXS; NOSTD;

	static wstring Type = L"normal";

	static wstring Text = L"准备就绪";

	int h;

	void Draw(RenWin& window)
	{
		static bool Init = false;
		if (!Init)
		{
			h = ScreenSize.y * 0.03;

			Init = true;
		}

		//配色
		if (Type == L"error")
		{
			XGraph::SetFillColor(Color(255,100,100));
		}
		else if (Type == L"success")
		{
			XGraph::SetFillColor(Color(100, 255, 100));
		}
		else if (Type == L"stop")
		{
			XGraph::SetFillColor(Color(155, 155, 50));
		}

		int StartY = WindowSize.y - h;

		//绘制底色
		if (Type != "normal" && Type != L"process" && Type != L"loading")
		{
			XGraph::RectangleShape::FillRect_WithoutBorder(0, StartY, WindowSize.x, h, window);
		}

		//绘制文字
		if (!Text.empty())
		{
			XText::SetFontConfig(FONTCOLOR, FONTSIZE);
			XText::SetFontAdjust(ADJUST_LEFT, ADJUST_CENTER);

			static int StartX = ScreenSize.x * 0.01;
			XText::Xyprintf(StartX, StartY + h / 2, Text, window);
		}
	}

	void SetText(const wstring& text)
	{

	}

	void SetShowType(const wstring& type)
	{
		Type = type;
	}
}