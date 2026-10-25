#include "WorkRect.h"
#include "SideBar.h"
#include "BottomBar.h"
#include "Shared.h"
#include "Search.h"
#include "user/Set.h"
#include "user/UserData.h"
#include "user/RequirePassWord.h"
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
	static Color BorderColor = Color(120, 120, 120);

	//绘制主页
	void DrawHome(int w,int h)
	{
		static IMAGE Icon;
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

	//绘制工作区
	void DrawWork(int w, int h)
	{
		int a = 255 * YOffset.frame / YOffset.totalframe;
		if (!YOffset.IsAnimation()) a = 255;

		static Color FontColor = Setting::BackColor == Color(30, 30, 30) ? Color(255, 255, 255) : Color(0, 0, 0);

		//标题
#pragma region MyRegion

		//绘制工作区信息
		FontColor.a = a;
		XText::SetFontConfig(FontColor, FONTSIZE * 1.4);
		XText::SetFontAdjust(ADJUST_LEFT, ADJUST_CENTER);
		static int x = ScreenSize.x / 60, y = ScreenSize.x / 60;
		XText::Xyprintf(x, y + YOffset.value, L"工作区域", rt);

#pragma endregion

		//区域
#pragma region MyRegion

		//绘制外框
		int ix = w * 0.01, iy = y + ScreenSize.y / 40, iw = w * 0.98, ih = h - iy - ix * 1.1;
		XGraph::LineShape::SetLineWidth(1);
		BorderColor.a = a;
		XGraph::SetColor(BorderColor);
		XGraph::RectangleShape::RoundRect(ix, iy + YOffset.value * 2, iw, ih, ROUNDSIZE, rt);

#pragma endregion

		//分支
#pragma region MyRegion

		//绘制分支底色
		static Color BackColor = Setting::BackColor == Color(30, 30, 30) ? Color(60, 60, 60) : Color(230, 230, 230);
		static Color BorderColor = Setting::BackColor == Color(30, 30, 30) ? Color(100, 100, 100) : Color(200, 200, 200);
		BackColor.a = a;
		XGraph::SetFillColor(BackColor);
		XGraph::SetColor(BorderColor);
		int bw = w * 0.48, bh = ScreenSize.y / 30;
		XGraph::RectangleShape::FillRoundRect(w * 0.99 - bw, y - bh / 2 + YOffset.value, bw, bh, ROUNDSIZE, rt);

		//绘制分支信息
		FontColor.a = a;
		XText::SetFontConfig(FontColor, FONTSIZE);
		XText::SetFontAdjust(ADJUST_LEFT, ADJUST_CENTER);
		int vx = w - bw, vy = ScreenSize.x / 60;
		XText::Xyprintf(vx, vy + YOffset.value, L"当前分支：无分支", rt);

#pragma endregion

	}

	//绘制分支管理
	void DrawBranchManage(int w, int h)
	{
		int a = 255 * YOffset.frame / YOffset.totalframe;
		if (!YOffset.IsAnimation()) a = 255;

		static Color FontColor = Setting::BackColor == Color(30, 30, 30) ? Color(255, 255, 255) : Color(0, 0, 0);

		//标题
#pragma region MyRegion

		//绘制
		FontColor.a = a;
		XText::SetFontConfig(FontColor, FONTSIZE * 1.4);
		XText::SetFontAdjust(ADJUST_LEFT, ADJUST_CENTER);
		static int x = ScreenSize.x / 60, y = ScreenSize.x / 60;
		XText::Xyprintf(x, y + YOffset.value, L"仓库-分支管理", rt);

#pragma endregion

		//区域
#pragma region MyRegion

		//绘制外框
		int ix = w * 0.01, iy = y + ScreenSize.y / 40, iw = w * 0.98, ih = h - iy - ix * 1.1;
		XGraph::LineShape::SetLineWidth(1);
		BorderColor.a = a;
		XGraph::SetColor(BorderColor);
		XGraph::RectangleShape::RoundRect(ix, iy + YOffset.value * 2, iw, ih, ROUNDSIZE, rt);

#pragma endregion

		//仓库
#pragma region MyRegion

		//绘制仓库底色
		static Color BackColor = Setting::BackColor == Color(30, 30, 30) ? Color(60, 60, 60) : Color(230, 230, 230);
		static Color BorderColor = Setting::BackColor == Color(30, 30, 30) ? Color(100, 100, 100) : Color(200, 200, 200);
		BackColor.a = a;
		XGraph::SetFillColor(BackColor);
		XGraph::SetColor(BorderColor);
		int bw = w * 0.48, bh = ScreenSize.y / 30;
		XGraph::RectangleShape::FillRoundRect(w * 0.99 - bw, y - bh / 2 + YOffset.value, bw, bh, ROUNDSIZE, rt);

		//绘制仓库信息
		FontColor.a = a;
		XText::SetFontConfig(FontColor, FONTSIZE);
		XText::SetFontAdjust(ADJUST_LEFT, ADJUST_CENTER);
		int vx = w - bw, vy = ScreenSize.x / 60;
		XText::Xyprintf(vx, vy + YOffset.value, L"当前仓库：无", rt);

#pragma endregion

	}

	//绘制网络协作
	void DrawNetCooperation(int w, int h)
	{
		int a = 255 * YOffset.frame / YOffset.totalframe;
		if (!YOffset.IsAnimation()) a = 255;

		static Color FontColor = Setting::BackColor == Color(30, 30, 30) ? Color(255, 255, 255) : Color(0, 0, 0);

		//标题
#pragma region MyRegion

		//绘制
		FontColor.a = a;
		XText::SetFontConfig(FontColor, FONTSIZE * 1.4);
		XText::SetFontAdjust(ADJUST_LEFT, ADJUST_CENTER);
		static int x = ScreenSize.x / 60, y = ScreenSize.x / 60;
		XText::Xyprintf(x, y + YOffset.value, L"网络协作", rt);

#pragma endregion

		//区域
#pragma region MyRegion

		//绘制外框
		int ix = w * 0.01, iy = y + ScreenSize.y / 40, iw = w * 0.98, ih = h - iy - ix * 1.1;
		XGraph::LineShape::SetLineWidth(1);
		BorderColor.a = a;
		XGraph::SetColor(BorderColor);
		XGraph::RectangleShape::RoundRect(ix, iy + YOffset.value * 2, iw, ih, ROUNDSIZE, rt);

#pragma endregion

		//共享
#pragma region MyRegion

		//绘制仓库底色
		static Color BackColor = Setting::BackColor == Color(30, 30, 30) ? Color(60, 60, 60) : Color(230, 230, 230);
		static Color BorderColor = Setting::BackColor == Color(30, 30, 30) ? Color(100, 100, 100) : Color(200, 200, 200);
		BackColor.a = a;
		XGraph::SetFillColor(BackColor);
		XGraph::SetColor(BorderColor);
		int bw = w * 0.48, bh = ScreenSize.y / 30;
		XGraph::RectangleShape::FillRoundRect(w * 0.99 - bw, y - bh / 2 + YOffset.value, bw, bh, ROUNDSIZE, rt);

		//绘制共享信息
		FontColor.a = a;
		XText::SetFontConfig(FontColor, FONTSIZE);
		XText::SetFontAdjust(ADJUST_LEFT, ADJUST_CENTER);
		int vx = w - bw, vy = ScreenSize.x / 60;
		XText::Xyprintf(vx, vy + YOffset.value, L"共享仓库：关闭", rt);

#pragma endregion

	}

	bool IsUserConfigPass = false;

	//绘制用户配置
	void DrawUserConfig(int w, int h)
	{
		int a = 255 * YOffset.frame / YOffset.totalframe;
		if (!YOffset.IsAnimation()) a = 255;

		static Color FontColor = Setting::BackColor == Color(30, 30, 30) ? Color(255, 255, 255) : Color(0, 0, 0);
		static Color BackColor = Setting::BackColor == Color(30, 30, 30) ? Color(50, 50, 50) : Color(230, 230, 230);

		//标题
#pragma region MyRegion

		//绘制
		FontColor.a = a;
		XText::SetFontConfig(FontColor, FONTSIZE * 1.4);
		XText::SetFontAdjust(ADJUST_LEFT, ADJUST_CENTER);
		static int x = ScreenSize.x / 60, y = ScreenSize.x / 60;
		XText::Xyprintf(x, y + YOffset.value, L"用户配置", rt);

#pragma endregion

		//区域
#pragma region MyRegion

		//绘制外框
		int ix = w * 0.01, iy = y + ScreenSize.y / 40, iw = w * 0.98, ih = h - iy - ix * 1.1;
		XGraph::LineShape::SetLineWidth(1);
		BorderColor.a = a;
		XGraph::SetColor(BorderColor);
		XGraph::RectangleShape::RoundRect(ix, iy + YOffset.value * 2, iw, ih, ROUNDSIZE, rt);

#pragma endregion

		//用户配置
#pragma region MyRegion

		if (!IsUserConfigPass)
		{
			XText::SetFontAdjust(ADJUST_CENTER, ADJUST_CENTER);
			FontColor.a = a;
			XText::SetFontConfig(FontColor, FONTSIZE * 1.5);
			XText::Xyprintf(w / 2, h / 3 + YOffset.value * 2, L"你需要提供账户的密码才能修改账户设置", rt);

			BackColor.a = a;
			int BarW = w / 5, BarH = h / 20;
			int BarX = w / 2 - BarW / 2, BarY = h / 2 - BarH / 2 + YOffset.value * 2.5;
			XGraph::SetFillColor(BackColor);
			XGraph::SetColor(Color(120,120,120));
			XGraph::LineShape::SetLineWidth(1);
			XGraph::RectangleShape::FillRoundRect(BarX, BarY, BarW, BarH, ROUNDSIZE, rt);

			XText::SetFontAdjust(ADJUST_CENTER, ADJUST_CENTER);
			FontColor.a = a;
			XText::SetFontConfig(FontColor, FONTSIZE);
			XText::Xyprintf(w / 2, h / 2 + YOffset.value * 2.5, L"< 提供密码并更改设置 >", rt);

			if (
				XMsg::Composite::IsMouseInRectDown(
					VK::MouseLeft, 
					SideBar::Level1::w + BarX, ScreenSize.y * 0.01 + Search::SearchBoxH + UISPACE + BarY, BarW, BarH))
				RequirePassWord::RequirePassWord();

			if (RequirePassWord::IsPassWordFinish())
			{
				IsUserConfigPass = true;

				YOffset.SetAnimationStartValue(WindowSize.y / 10);
				YOffset.SetAnimation(0, 15);
			}

			NeedRedraw = true;
		}

#pragma endregion

	}

	//绘制软件设置
	void DrawSetting(int w, int h)
	{
		int a = 255 * YOffset.frame / YOffset.totalframe;
		if (!YOffset.IsAnimation()) a = 255;

		static Color FontColor = Setting::BackColor == Color(30, 30, 30) ? Color(255, 255, 255) : Color(0, 0, 0);

		//标题
#pragma region MyRegion

		//绘制
		FontColor.a = a;
		XText::SetFontConfig(FontColor, FONTSIZE * 1.4);
		XText::SetFontAdjust(ADJUST_LEFT, ADJUST_CENTER);
		static int x = ScreenSize.x / 60, y = ScreenSize.x / 60;
		XText::Xyprintf(x, y + YOffset.value, L"软件设置", rt);

#pragma endregion

		//区域
#pragma region MyRegion

		//绘制外框
		int ix = w * 0.01, iy = y + ScreenSize.y / 40, iw = w * 0.98, ih = h - iy - ix * 1.1;
		XGraph::LineShape::SetLineWidth(1);
		BorderColor.a = a;
		XGraph::SetColor(BorderColor);
		XGraph::RectangleShape::RoundRect(ix, iy + YOffset.value * 2, iw, ih, ROUNDSIZE, rt);

#pragma endregion

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

			if (SideBaiIndexTemp != 4) IsUserConfigPass = false;
		}

		YOffset.UpdateAnimation(XEase::EaseBasic::easeOut, 4);

		
		if(NeedRedraw || XMsg::WindowMsg::IsWindowResize() || YOffset.IsAnimation())
		{
			int w = WindowSize.x - x - UISPACE * 3;
			int h = WindowSize.y - StartY - BottomBar::h - UISPACE;

			NeedRedraw = false;

			if(rt.resize({unsigned int(w),unsigned int(h)}))
			{
				static Color ClearColor = Setting::BackColor == Color(30, 30, 30) ? Color(0, 0, 0, 0) : Color(255, 255, 255, 0);
				rt.clear(ClearColor);

				static Color BackColor = Setting::BackColor == Color(30, 30, 30) ? Color(20, 20, 20, 200) : Color(255, 255, 255, 200);
				XGraph::SetFillColor(BackColor);

				static int lw = ScreenSize.x / 1920;
				XGraph::LineShape::SetLineWidth(lw);
				static Color LineColor = Setting::BackColor == Color(30, 30, 30) ? Color(100, 100, 255) : Color(200, 200, 255);
				XGraph::SetColor(Color(120, 120, 120));

				XGraph::RectangleShape::FillRoundRect(lw, lw, w - lw * 2, h - lw * 2, ROUNDSIZE, rt);

				if (SideBar::Level1::SideBarIndex == 0) DrawHome(w - lw * 2, h - lw * 2);
				if (SideBar::Level1::SideBarIndex == 1) DrawWork(w - lw * 2, h - lw * 2);
				if(SideBar::Level1::SideBarIndex == 2) DrawBranchManage(w - lw * 2, h - lw * 2);
				if (SideBar::Level1::SideBarIndex == 3) DrawNetCooperation(w - lw * 2, h - lw * 2);
				if (SideBar::Level1::SideBarIndex == 4) DrawUserConfig(w - lw * 2, h - lw * 2);
				if(SideBar::Level1::SideBarIndex == 5) DrawSetting(w - lw * 2, h - lw * 2);

				rt.display();
			}
		}

		Sprite s(rt.getTexture());
		s.setPosition({ float(x),float(StartY) });
		window.draw(s);
	}
}