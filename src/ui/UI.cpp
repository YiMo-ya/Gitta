#include "UI.h"

NOXS; NOSTD;

//矩形UI边框渐变速度
const int RECT_BORDER_ALPHA_SPEED = 15;

//工具函数
namespace ToolFc
{
	//渐变
	void ChangeInt(int& Value, const int& Min, const int& Max, const int& Speed)
	{
		Value += Speed;

		Value = max(Min, min(Max, Value));
	}

}

namespace UI
{

	//矩形
#pragma region MyRegion

	void Rect::Draw(RenderTarget& target,int x,int y,int w,int h,int r)
	{
		IsInUI = XMsg::MouseMsg::IsMouseIn(x, y, w, h);

		//绘制边框
		if (EnableBorder)
		{
			if (IsInUI) ToolFc::ChangeInt(BorderAlpha, 0, 255, RECT_BORDER_ALPHA_SPEED);
			else ToolFc::ChangeInt(BorderAlpha, 0, 255, -RECT_BORDER_ALPHA_SPEED);

			if (BorderAlpha > 0)
			{
				XGraph::SetColor(BorderColor);
				if(r > 0) XGraph::RectangleShape::RoundRect(x, y, w, h, r, target);
				else XGraph::RectangleShape::Rect(x, y, w, h, target);
			}
		}

		//绘制背景
		XGraph::SetFillColor(BackColor);
		if (r > 0) XGraph::RectangleShape::FillRoundRect_WithoutBorder(x, y, w, h, r, target);
		else XGraph::RectangleShape::FillRect_WithoutBorder(x, y, w, h, target);
	}

	bool Rect::IsIn()
	{
		return IsInUI;
	}

	bool Rect::IsClick()
	{
		return XMsg::MouseMsg::IsMouseDown(VK::MouseLeft) && IsInUI;
	}

#pragma endregion

}