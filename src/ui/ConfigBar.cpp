#include "Shared.h"
#include "ConfigBar.h"
#include "SideBar.h"
#include "Menu.h"
#include "TimeLine.h"
#include "BottomBar.h"

namespace ConfigBar
{
	NOXS; NOSTD;

	RenderTexture rt;

	EV ConfigBarW;

	bool NeedRedraw;

	//0对象1控件
	int State = 0;

	//绘制
	void Draw(RenWin& window)
	{
		//更新动画
		ConfigBarW.UpdateAnimation(XEase::EaseBasic::easeOut, 4);

		//开始Y坐标
		static int StartY = Menu::h;
		//高度
		int h = WindowSize.y - StartY - BottomBar::h - UISPACE;
		if (TimeLine::TimeLineH.value > 0) h -= (UISPACE + TimeLine::TimeLineH.value);

		//纹理尺寸大于0时绘制
		if (ConfigBarW.value > 0)
		{
			if (ConfigBarW.IsAnimation() || NeedRedraw || XMsg::WindowMsg::IsWindowResize())
			{
				if (rt.resize({ unsigned int(ConfigBarW.value),unsigned int(h) }))
				{
					rt.clear(Color::Transparent);

					//绘制底色
					static int lw = ScreenSize.x / 500;
					XGraph::LineShape::SetLineWidth(lw);
					XGraph::SetColor(Color(120, 120, 120));
					XGraph::RectangleShape::RoundRect(0, 0, ConfigBarW.value, h, ROUNDSIZE, rt);

					rt.display();
				}
			}
		}

	}

}