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

	//绘制没有DWM的边框
	void DrawBorder(RenWin& window)
	{
		if(XWindow::IsFocus(window)) XGraph::SetColor(UserData::MainColor);
		else XGraph::SetColor(Color(150,150,150));

		static int LineWidth = ScreenSize.x / 1000;
		XGraph::LineShape::SetLineWidth(LineWidth);

		XGraph::RectangleShape::Rect(0, 0, WindowSize.x, WindowSize.y, window);
	}
}