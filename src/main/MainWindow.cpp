#include "MainWindow.h"
#include "Shared.h"
#include "user/User.h"

#include "ui/Menu.h"
#include "ui/SideBar.h"
#include "ui/BottomBar.h"
#include "ui/WorkRect.h"

namespace MainWindow
{
	NOXS; NOSTD;

	void App(RenWin& window)
	{

		Menu::h = ScreenSize.y * 0.04;

		while (XMsg::IsOpen(window))
		{
			XWindow::DelayFps(window);

			WindowSize = XWindow::GetWindowSize(window);

			//未启用DWM绘制边框
			if (!EnableDWM)
			{
				Shared::DrawBorder(window);
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
			}

			//绘制工作区背景
			WorkRect::Draw(window);

			//菜单消息
			Menu::Msg(window);

			//绘制底部消息栏
			BottomBar::Draw(window);

			//绘制右侧侧边栏
			SideBar::Level1::Draw(window);

			//绘制右侧侧边栏（2级）
			SideBar::Level2::Draw(window);

			//绘制右侧侧边栏（1级）提示
			SideBar::Level1::DrawTips(window);

			//绘制菜单
			Menu::Draw(window);
		}
	}
}