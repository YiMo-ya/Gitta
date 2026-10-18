#include "MainWindow.h"
#include "Shared.h"
#include "user/User.h"

#include "ui/SideBar.h"
#include "ui/BottomBar.h"
#include "ui/WorkRect.h"

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
		XImageEx::ImageChange::Scale(image, ScreenSize.x, ScreenSize.y);
		for (int i = 0; i < 100;i++) XImageEx::ImageBlur::Gaussian(image, 10);
		image.color.a = 100;
	}

	void App(RenWin& window)
	{
		//当DWM不可用或被禁用时的选择
		static IMAGE BackImage;
		if (!EnableDWM) InitBackImage(BackImage);

		while (XMsg::IsOpen(window))
		{
			XWindow::DelayFps(window);

			WindowSize = XWindow::GetWindowSize(window);

			//未启用DWM
			if (!EnableDWM)
			{
				//绘制背景
				Vector2i WindowPos = XWindow::GetWindowPos(window);
				WindowPos.x = max(0, WindowPos.x, 0);
				WindowPos.y = max(0, WindowPos.y, 0);

				static EV2 BackImageDrawPos;
				static Vector2i WindowPosTemp;
				if (WindowPos.x != WindowPosTemp.x || WindowPos.y != WindowPosTemp.y)
				{
					BackImageDrawPos.x.SetAnimation(WindowPos.x, 30);
					BackImageDrawPos.y.SetAnimation(WindowPos.y, 30);
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
					XWindow::DWM::SetWindowBorderColor(window, UserData::MainColor);
				}
				else
				{
					XWindow::DWM::SetWindowBorderColor(window, Color(150,150,150));
				}

				static Color BackColor = UserData::BackColor == Color(30, 30, 30) ? Color(40, 40, 40, 100) : Color(255, 255, 255, 200);
				XGraph::SetFillColor(BackColor);
				XGraph::RectangleShape::FillRect_WithoutBorder(0, 0, WindowSize.x, WindowSize.y, window);
			}

			//绘制工作区背景
			WorkRect::Draw(window);

			//绘制底部消息栏
			BottomBar::Draw(window);

			//绘制右侧侧边栏
			SideBar::Level1::Draw(window);

			//绘制右侧侧边栏（1级）提示
			SideBar::Level1::DrawTips(window);
		}
	}
}