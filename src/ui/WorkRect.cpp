#include "WorkRect.h"
#include "SideBar.h"
#include "BottomBar.h"
#include "Shared.h"
#include "Search.h"
#include "user/Set.h"
#include "user/UserData.h"
#include <random>

namespace WorkRect
{
	NOXS; NOSTD;

	//随机数
	namespace
	{
		int rand_int(int min, int max) {
			// 检查有效性
			if (min > max) {
				std::swap(min, max);  // 适当交换，保证 min <= max
			}

			// 每个线程一个引擎，避免多线程竞争
			thread_local std::mt19937 rng{
				std::random_device{}()
			};

			std::uniform_int_distribution<int> dist(min, max);
			return dist(rng);
		}
	}

	RenderTexture rt;

	bool NeedRedraw = true;

	static EV YOffset;

	//绘制主页
	void DrawHome(int w,int h)
	{
		static IMAGE Icon;
		static Color BorderColor = Color(120,120,120);
		static EV TextOffset;

		int a = 255 * YOffset.frame / YOffset.totalframe;
		if (!YOffset.IsAnimation()) a = 255;
		
		//初始化
		static bool init = false;
		if (!init)
		{
			init = true;

			TextOffset.SetAnimation(w * 0.03, 15);

			//加载图标
			XImage::NewImage(Icon, L"Icon.dll");
		}

		TextOffset.UpdateAnimation(XEase::EaseBasic::easeOut, 4);

		//标题
#pragma region MyRegion

		//绘制用户信息
		static Color FontColor = Setting::BackColor == Color(30, 30, 30) ? Color(255, 255, 255) : Color(0, 0, 0);
		FontColor.a = a;
		XText::SetFontConfig(FontColor, FONTSIZE * 1.4);
		XText::SetFontAdjust(ADJUST_LEFT, ADJUST_CENTER);
		static int x = ScreenSize.x / 20, y = ScreenSize.x / 40;
		XText::Xyprintf(x, y + YOffset.value, L"Gitta - 你好     " + UserData::UserName, rt);

		//绘制图标
		static float Scale = ScreenSize.x / 45 / 512.0;
		Icon.color.a = a;
		XImage::PutScaleImage(Icon, x / 2, y + YOffset.value, Scale, Scale, rt, 0.5, 0.5);

#pragma endregion

		//显示仓库
#pragma region MyRegion

		//绘制仓库信息外框
		int rx = w * 0.01, ry = y + ScreenSize.y / 20, rw = w * 0.98, rh = h * 0.4;
		XGraph::LineShape::SetLineWidth(1);
		BorderColor.a = a;
		XGraph::SetColor(BorderColor);
		XGraph::RectangleShape::RoundRect(rx, ry + YOffset.value * 1.5, rw, rh, ROUNDSIZE, rt);

		//显示仓库信息
		FontColor.a = a;
		XText::SetFontConfig(FontColor, FONTSIZE * 1);
		XText::SetFontAdjust(ADJUST_LEFT, ADJUST_CENTER);
		XText::Xyprintf(rx + rw * 0.01 + TextOffset.value, ry + rw * 0.02 + YOffset.value * 1.5, L"最近使用的代码仓库: ", rt);

		//绘制加载动画
		static Color LoadingColor = Setting::MainColor;
		LoadingColor.a = a;
		XGraph::SetColor(LoadingColor);
		static int lw = ScreenSize.x / 480;
		XGraph::LineShape::SetLineWidth(lw);
		static bool Fast = false;
		static int Start;
		static EV Angle;
		if (!Angle.IsAnimation())
		{
			if (Angle.end == 240)
			{
				Angle.SetAnimation(20, 20);
			}
			else
			{
				Angle.SetAnimation(240, 20);
			}
		}
		int temp = Angle.value;
		Angle.UpdateAnimation(XEase::EaseBasic::easeOut, 4);
		if (Angle.end == 240) Start += 10;
		else
		{
			Start += 10 + temp - Angle.value;
		}
		if (Start + Angle.value > 360)
		{
			Start -= 360;
		}
		XGraph::CircleShape::Arc(rx + rw * 0.02, ry + rw * 0.02 + YOffset.value * 1.3, rw / 120, Start, Angle.value, rt);

#pragma endregion

		//最近消息
#pragma region MyRegion

		//绘制最近消息外框
		int ix = w * 0.01, iy = ry + rh + ScreenSize.y / 50, iw = w * 0.98, ih = h * 0.4;
		XGraph::LineShape::SetLineWidth(1);
		BorderColor.a = a;
		XGraph::SetColor(BorderColor);
		XGraph::RectangleShape::RoundRect(ix, iy + YOffset.value * 2, iw, ih, ROUNDSIZE, rt);

		//显示最近消息
		FontColor.a = a;
		XText::SetFontConfig(FontColor, FONTSIZE * 1);
		XText::SetFontAdjust(ADJUST_LEFT, ADJUST_CENTER);
		XText::Xyprintf(ix + iw * 0.01, iy + iw * 0.02 + YOffset.value * 2, L"最新的消息: ", rt);
		
#pragma endregion

		NeedRedraw = true;
	}

	void Draw(RenWin& window)
	{
		static int StartY = ScreenSize.y * 0.01 + Search::SearchBoxH + UISPACE;
		int x = SideBar::Level1::w;

		//自动切换逻辑
		static int SideBaiIndexTemp = -1;
		if (SideBar::Level1::SideBarIndex != SideBaiIndexTemp)
		{
			SideBaiIndexTemp = SideBar::Level1::SideBarIndex;
			YOffset.SetAnimationStartValue(WindowSize.y / 10);
			YOffset.SetAnimation(0, 15);
		}

		YOffset.UpdateAnimation(XEase::EaseBasic::easeOut, 4);

		

		if(NeedRedraw || XMsg::WindowMsg::IsWindowResize() || YOffset.IsAnimation())
		{
			int w = WindowSize.x - x - UISPACE * 3;
			int h = WindowSize.y - StartY - BottomBar::h - UISPACE;

			NeedRedraw = false;

			if(rt.resize({unsigned int(w),unsigned int(h)}))
			{
				rt.clear(Color::Transparent);

				static Color BackColor = Setting::BackColor == Color(30, 30, 30) ? Color(20, 20, 20, 200) : Color(255, 255, 255, 200);
				XGraph::SetFillColor(BackColor);

				static int lw = ScreenSize.x / 1920;
				XGraph::LineShape::SetLineWidth(lw);
				static Color LineColor = Setting::BackColor == Color(30, 30, 30) ? Color(100, 100, 255) : Color(200, 200, 255);
				XGraph::SetColor(Color(120, 120, 120));

				XGraph::RectangleShape::FillRoundRect(lw, lw, w - lw * 2, h - lw * 2, ROUNDSIZE, rt);

				if (SideBar::Level1::SideBarIndex == 0) DrawHome(w - lw * 2, h - lw * 2);

				rt.display();
			}
		}

		Sprite s(rt.getTexture());
		s.setPosition({ float(x),float(StartY) });
		window.draw(s);
	}
}