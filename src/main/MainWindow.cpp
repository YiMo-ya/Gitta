#include "MainWindow.h"
#include "Shared.h"

#include "user/Set.h"
#include "user/UserData.h"
#include "user/Login.h"
#include "user/RequirePassWord.h"

#include "ui/SideBar.h"
#include "ui/BottomBar.h"
#include "ui/WorkRect.h"
#include "ui/Search.h"

namespace MainWindow
{
	NOXS; NOSTD;

	void InitBackImage(IMAGE& image)
	{
		// uiParam 传的是“字符数”，不是字节数[3](@ref)
		wchar_t WallPaperPath[MAX_PATH] = { 0 };

		BOOL ok = SystemParametersInfoW(
			SPI_GETDESKWALLPAPER,
			MAX_PATH,          // 缓冲区容量（字符数）
			WallPaperPath,
			0                  // 只读查询，不需要 SPIF_* 标志
		);

		XImage::NewImage(image, WallPaperPath);
		XImageEx::ImageChange::Scale(image, ScreenSize.x / float(image.w), ScreenSize.y / float(image.h));
		XImageEx::ImageBlur::Box(image,15);
		image.color.a = Setting::BackColor == Color(30,30,30) ? 100 : 200;
	}

	void App(RenWin& window)
	{
		//当DWM不可用或被禁用时的选择
		static IMAGE BackImage;
		if (!EnableDWM) InitBackImage(BackImage);

		//图标
		static IMAGE AppIcon;
		XImage::NewImage(AppIcon, L"Icon.dll");
		float Scale = ScreenSize.x * 0.013 / 512.0;
		XImageEx::ImageChange::Scale(AppIcon, Scale, Scale);

		int TaskBarHeight = XSystem::Info::GetTaskbarSize().y;

		//自动登录
		if (!UserData::AutoLogin())
		{
			if (XFile::Exists(L"User"))
			{
				XFile::Remove(L"User");
			}
		}

		while (XMsg::IsOpen(window))
		{
			XWindow::DelayFps(window);

			WindowSize = XWindow::GetWindowSize(window);

			//未启用DWM
			if (!EnableDWM)
			{
				//绘制背景
				Vector2i WindowPos = XWindow::GetWindowPos(window);
				WindowPos.x = max(0, WindowPos.x);
				WindowPos.y = max(0, WindowPos.y);

				static EV2 BackImageDrawPos;
				static Vector2i WindowPosTemp;
				if (WindowPos.x != WindowPosTemp.x || WindowPos.y != WindowPosTemp.y)
				{
					BackImageDrawPos.x.SetAnimation(WindowPos.x, 30);
					BackImageDrawPos.y.SetAnimation(WindowPos.y + TaskBarHeight, 30);
					WindowPosTemp = WindowPos;
				}
				BackImageDrawPos.x.UpdateAnimation(XEase::EaseBasic::easeOut, 3);
				BackImageDrawPos.y.UpdateAnimation(XEase::EaseBasic::easeOut, 3);
				XImage::PutImage(BackImage, -BackImageDrawPos.x.value, -BackImageDrawPos.y.value, window);
			}
			else
			{
				if (XWindow::IsFocus(window))
				{
					XWindow::DWM::SetWindowBorderColor(window, Setting::MainColor);
				}
				else
				{
					XWindow::DWM::SetWindowBorderColor(window, Color(150,150,150));
				}

				static Color BackColor = Setting::BackColor == Color(30, 30, 30) ? Color(40, 40, 40, 100) : Color(255, 255, 255, 0);
				XGraph::SetFillColor(BackColor);
				XGraph::RectangleShape::FillRect_WithoutBorder(0, 0, WindowSize.x, WindowSize.y, window);
			}

			//绘制搜索栏
			Search::Draw(window);

			//绘制底部消息栏
			BottomBar::Draw(window);

			//绘制右侧侧边栏
			SideBar::Level1::Draw(window);

			//绘制工作区背景
			WorkRect::Draw(window);

			//绘制右侧侧边栏（1级）提示
			SideBar::Level1::DrawTips(window);

			//绘制左上角图标
			static Vector2i AppIconPos = { SideBar::Level1::w / 2,ScreenSize.y / 100 + Search::SearchBoxH / 2 };
			XImage::PutImage(AppIcon, AppIconPos.x, AppIconPos.y, window,0.5,0.5);

			static bool CheckUser = false;
			if (!CheckUser)
			{
				//启动后只检测一次本机是否已有账户存档，避免每帧都去扫磁盘
				CheckUser = true;

				if (!UserData::IsUserLogin() || !UserData::HasUser())
				{
					UserLogin::Enter(window);
				}
			}

			//当有密码需求时进入
			RequirePassWord::Enter(window);
		}
	}
}