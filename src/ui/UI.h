#pragma once
#include "Xs/Xs.h"

namespace UI
{
	class Rect
	{
		int BorderAlpha = 0;

		int x, y, w, h;

		bool IsInUI = false;
	public:

		bool EnableBorder;

		Color BackColor, BorderColor;

		//绘制
		void Draw(RenderTarget& target, int x, int y, int w, int h, int r);

		//是否在UI里面
		bool IsIn();

		//是否被点击
		bool IsClick();
	};
}
