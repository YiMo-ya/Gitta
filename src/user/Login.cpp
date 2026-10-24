#include "Login.h"
#include "Set.h"
#include "ui/UI.h"

//打开外部帮助文件需要 ShellExecuteW
#include <shellapi.h>
#pragma comment(lib, "shell32.lib")

namespace UserLogin
{
	NOXS;NOSTD;

	std::string UserName, Password;

	namespace
	{
		//登录界面共用的界面布局缓存，由 Enter 中的背景方块动画写入，供各绘制函数读取
		int LoginBarX = 0, LoginBarY = 0;        //背景方块左上角坐标
		int LoginBarW = 0, LoginBarH = 0;        //背景方块宽高

		/// <summary>
		/// 绘制标题文字，并处理标题的下落与淡入动画。
		/// </summary>
		/// <param name="window">渲染目标窗口。</param>
		/// <param name="Clock">全局计时器，用于延迟启动淡入动画。</param>
		void DrawTitle(RenWin& window, int Clock)
		{
			//动画每帧推进一次，用静态量保证跨帧持续生效
			static EV TitlePosYOffset;
			static bool init = false;
			if (!init) 
			{
				init = true;
				TitlePosYOffset.SetAnimationStartValue(ScreenSize.y * 0.05);
				TitlePosYOffset.SetAnimation(0, 30);
			}

			static Color FontColor = Setting::BackColor == Color(30, 30, 30) ? Color(255, 255, 255) : Color(0, 0, 0);

			if (Clock > 10)
			{
				TitlePosYOffset.UpdateAnimation(XEase::EaseBasic::easeOut, 4);
				if (TitlePosYOffset.IsAnimation())
					FontColor.a = TitlePosYOffset.frame * 255 / TitlePosYOffset.totalframe;
				else
					FontColor.a = 255;
			}
			else
				FontColor.a = 0;

			XText::SetFontConfig(FontColor, FONTSIZE * 1.5);
			XText::SetFontAdjust(ADJUST_CENTER, ADJUST_TOP);
			XText::Xyprintf(LoginBarX + LoginBarW / 2,
				LoginBarY + int(LoginBarH * 0.08) + int(TitlePosYOffset.value),
				L"登录到您本地Gitta的账户", window);
		}

		/// <summary>
		/// 绘制用户名与密码输入框，并处理淡入、悬停边框高亮与文本输入。
		/// </summary>
		/// <param name="window">渲染目标窗口。</param>
		/// <param name="Clock">全局计时器，用于延迟启动淡入动画。</param>
		void DrawInputBoxes(RenWin& window, int Clock)
		{
			static UI::InPutRectUI UserInput, PassInput;
			//两个输入框各自持有独立的位移动画，才能让密码框晚于用户名框入场
			static EV UserPosYOffset, PassPosYOffset;
			static Color UserBoxColor = Color(255, 255, 255, 0);
			static Color PassBoxColor = Color(255, 255, 255, 0);
			static int alpha1 = 0, alpha2 = 0;
			static bool init = false;

			//密码动画相对用户名动画延迟的帧数
			constexpr int PassDelayFrame = 5;

			if (!init)
			{
				init = true;

				//用户名框：作为第一段动画，起始时机不变
				UserPosYOffset.SetAnimationStartValue(ScreenSize.y * 0.05);
				UserPosYOffset.SetAnimation(0, 20);

				//密码框：下移量更大，形成从更远处上浮的层次感；帧数保持不变
				PassPosYOffset.SetAnimationStartValue(ScreenSize.y * 0.05);
				PassPosYOffset.SetAnimation(0, 20);
			}

			//淡入：比标题更晚启动，形成依次入场效果。
			if (Clock > 15)
			{
				UserPosYOffset.UpdateAnimation(XEase::EaseBasic::easeOut, 4);
				if (UserPosYOffset.IsAnimation())
					UserBoxColor.a = UserPosYOffset.frame * 255 / UserPosYOffset.totalframe;
				else
					UserBoxColor.a = 255;
			}
			else
			{
				UserBoxColor.a = 0;
			}
			if (Clock > 20)
			{
				PassPosYOffset.UpdateAnimation(XEase::EaseBasic::easeOut, 4);
				if (PassPosYOffset.IsAnimation())
					PassBoxColor.a = PassPosYOffset.frame * 255 / PassPosYOffset.totalframe;
				else
					PassBoxColor.a = 255;
			}
			else
			{
				PassBoxColor.a = 0;
			}

			//输入框尺寸：每帧按背景方块实际宽度计算，避免首帧为 0 导致两框重叠
			int BoxW = int(LoginBarW * 0.5);
			int BoxH = int(BoxW * 0.08);
			if (BoxH < FONTSIZE * 2) BoxH = FONTSIZE * 2;

			//从标题下方开始依次向下排布两个输入框
			int boxX = LoginBarX + (LoginBarW - BoxW) / 2;
			int titleBottom = LoginBarY + int(LoginBarH * 0.25) + FONTSIZE * 3;
			int boxGap = BoxH / 2;

			//基础排版位置(不动画时的最终落点)
			int userYBase = titleBottom;
			int passYBase = userYBase + BoxH + boxGap;
			int userY = userYBase + int(UserPosYOffset.value);
			int passY = passYBase + int(PassPosYOffset.value);

			//配色：底色比背景方块略深(暗色) / 略浅(亮色)，并套用淡入 alpha
			const Color BoxBaseFill = Setting::BackColor == Color(30, 30, 30) ? Color(60, 60, 60) : Color(235, 235, 235);
			Color UserFillColor = XColor::AlphaColor(BoxBaseFill, (uint8_t)UserBoxColor.a);
			Color PassFillColor = XColor::AlphaColor(BoxBaseFill, (uint8_t)PassBoxColor.a);

			//两个输入框的提示文字透明度各自独立
			Color UserTipColor = Color(150, 150, 150);
			UserTipColor.a = UserBoxColor.a;
			Color PassTipColor = Color(150, 150, 150);
			PassTipColor.a = PassBoxColor.a;

			static Color FontColor = Setting::BackColor == Color(30, 30, 30) ? Color(255, 255, 255) : Color(0, 0, 0);
			if (Clock > 10) FontColor.a = 255;

			//绘制单个输入框外框：圆角底色 + 悬停时主色细边框。
			//边框 alpha 必须按 ba 即时构造颜色，不能改动外部共享颜色对象，
			//否则两个输入框会共用同一份颜色导致透明度互相覆盖
			auto DrawInputBoxBorder = [&](int bY, int ba, const Color& fillColor)
			{
				XGraph::SetFillColor(fillColor);
				XGraph::RectangleShape::FillRoundRect_WithoutBorder(boxX, bY, BoxW, BoxH, ROUNDSIZE, window);
				if (ba > 0)
				{
					XGraph::SetColor(XColor::AlphaColor(Setting::MainColor, (uint8_t)ba));
					XGraph::LineShape::SetLineWidth(1);
					XGraph::RectangleShape::RoundRect(boxX, bY, BoxW, BoxH, ROUNDSIZE, window);
				}
			};

			//悬停渐变：仅在输入框实际可见后才推进，避免淡入阶段边框提前亮起。
			//alpha 按各自框的透明度判断，密码框延迟期间不会响应悬停
			auto UpdateHover = [&](int bY, int& alpha, uint8_t boxAlpha)
			{
				if (boxAlpha <= 0) return;
				if (XMsg::MouseMsg::IsMouseIn(boxX, bY, BoxW, BoxH))
				{
					if (alpha + 30 < 255) alpha += 30;
					else alpha = 255;
				}
				else
				{
					if (alpha - 30 > 0) alpha -= 30;
					else alpha = 0;
				}
			};

			XText::SetFontSize(FONTSIZE);
			XText::SetFontAdjust(ADJUST_LEFT, ADJUST_CENTER);

			//先画外框再交给组件处理输入，避免组件绘制的文字被底色覆盖
			UpdateHover(userY, alpha1, UserBoxColor.a);
			DrawInputBoxBorder(userY, alpha1, UserFillColor);
			UserInput.x = boxX, UserInput.y = userY, UserInput.w = BoxW, UserInput.h = BoxH;
			UserInput.fx = boxX + UISPACE, UserInput.fy = userY + BoxH / 2;
			UserInput.prefix = "用户名：";
			UserInput.tips = "用户名：";
			UserInput.TextColor = FontColor;
			UserInput.CurColor = FontColor;
			UserInput.TipColor = UserTipColor;
			UserInput.Draw(window, UserName);

			UpdateHover(passY, alpha2, PassBoxColor.a);
			DrawInputBoxBorder(passY, alpha2, PassFillColor);
			PassInput.x = boxX, PassInput.y = passY, PassInput.w = BoxW, PassInput.h = BoxH;
			PassInput.fx = boxX + UISPACE, PassInput.fy = passY + BoxH / 2;
			PassInput.prefix = "密码：";
			PassInput.tips = "密码：";
			PassInput.TextColor = FontColor;
			PassInput.CurColor = FontColor;
			PassInput.TipColor = PassTipColor;
			PassInput.Draw(window, Password);
		}

		/// <summary>
		/// 绘制背景方块左下角的蓝色帮助文字，并在点击时打开本地帮助页面。
		/// </summary>
		/// <param name="window">渲染目标窗口。</param>
		void DrawHelpLink(RenWin& window)
		{
			const wchar_t* HelpText = L"<为什么Gitta需要本地的账户?>";

			//小字号，保证不同分辨率下都保持「小字」观感
			static int HelpFontSize =FONTSIZE * 0.8;
			int HelpTextW = WindowSize.x / 10;
			int HelpTextH = WindowSize.y / 20;

			//内边距随方块宽度缩放，避免小窗口下贴边
			static int HelpPadding = int(ScreenSize.x / 50);
			int HelpX = LoginBarX + HelpPadding;
			int HelpY = LoginBarY + int(LoginBarH * 0.99) - HelpTextH;

			//鼠标悬停在矩形内即高亮，给用户「可点击」的反馈
			bool inHelp = XMsg::MouseMsg::IsMouseIn(HelpX, HelpY, HelpTextW, HelpTextH);
			Color HelpColor = inHelp ? Color(120, 190, 255) : Color(80, 150, 235);
			HelpColor.a = 255;

			//文字以矩形中心对齐绘制
			XText::SetFontConfig(HelpColor, HelpFontSize);
			XText::SetFontAdjust(ADJUST_CENTER, ADJUST_CENTER);
			XText::Xyprintf(HelpX + HelpTextW / 2, HelpY + HelpTextH / 2, HelpText, window);

			if (inHelp && XMsg::MouseMsg::IsMouseDown(VK::MouseLeft))
			{
				//以程序所在目录为基准定位帮助文件，避免受当前工作目录影响
				std::wstring HelpPath = ExePath + L"\\Html\\UserHelp.html";
				ShellExecuteW(nullptr, L"open", HelpPath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
			}
		}

		/// <summary>
		/// 绘制背景方块右下角的「登录 / 注册」按钮，悬停时以主色填充作为可点击反馈。
		/// </summary>
		/// <param name="window">渲染目标窗口。</param>
		/// <param name="Clock">全局计时器，用于与输入框同步淡入，避免按钮提前出现。</param>
		/// <returns>本帧按钮是否被点击，供外部触发登录或注册流程。</returns>
		bool DrawLoginButton(RenWin& window, int Clock)
		{
			static EV LoginBoxSize;
			static IMAGE img;
			static bool init = false;
			if (!init)
			{
				init = true;

				LoginBoxSize.SetAnimationStartValue(0);
				LoginBoxSize.SetAnimation(100, 20);

				XImage::NewImage(img, ExePath + L"\\Image\\Login\\Go.dll");
				img.color = Setting::BackColor == Color(30, 30, 30) ? Color(255, 255, 255) : Color(0, 0, 0);
			}

			if(Clock > 25)
			{
				LoginBoxSize.UpdateAnimation(XEase::EaseBasic::easeOut, 4);
			}

			static int BasicSize = ScreenSize.x / 80;
			int x = WindowSize.x / 2, y = LoginBarY + LoginBarH * 0.65 + (100 - LoginBoxSize.value) * BasicSize / 40;

			bool IsIn = XMsg::MouseMsg::IsMouseIn(x - BasicSize, y - BasicSize, BasicSize * 2, BasicSize * 2);

			//绘制圆形背景
			const Color BoxBaseFill = Setting::BackColor == Color(30, 30, 30) ? Color(60, 60, 60) : Color(235, 235, 235);
			XGraph::SetFillColor(BoxBaseFill);
			XGraph::CircleShape::FillCircle_WithoutBorder(x, y,BasicSize * LoginBoxSize.value / 100, window);

			//绘制边框
			static int BorderAlpha = 0;
			if (IsIn)
			{
				if (BorderAlpha + 30 < 255) BorderAlpha += 30;
				else BorderAlpha = 255;
			}
			else
			{
				if (BorderAlpha - 30 > 0) BorderAlpha -= 30;
				else BorderAlpha = 0;
			}
			if (BorderAlpha > 0)
			{
				static Color BorderColor = Setting::MainColor;
				BorderColor.a = BorderAlpha;
				XGraph::SetColor(BorderColor);
				XGraph::LineShape::SetLineWidth(1);
				XGraph::CircleShape::Circle(x, y, BasicSize * LoginBoxSize.value / 100, window);
			}

			//绘制箭头
			float Scale = BasicSize / 512.0 * LoginBoxSize.value / 100.0;
			XImage::PutScaleImage(img, x, y, Scale, Scale, window, 0.5, 0.5);

			return false;
		}
	}

	void Enter(RenWin& window)
	{
		IMAGE back;
		XImage::NewImage(window, back);

		EV2 LoginBar;

		int BackAlpha = 255;
		const int BACKALPHA = 100;
		const int BACKALPHA_SPEED = 15;

		XWindow::SetBackGroundColor(Color::Black);

		int Clock = 0;

		while (XMsg::IsOpen(window))
		{
			XWindow::DelayFps(window);
			WindowSize = XWindow::GetWindowSize(window);

			//全局计时器
			if (Clock < 120) Clock += 1;

			//背景图片
			back.color.a = BackAlpha;
			XImage::PutScaleImage(back, 0, 0, WindowSize.x / float(back.w), WindowSize.y / float(back.h), window);
			if (BackAlpha - BACKALPHA_SPEED > BACKALPHA) BackAlpha -= BACKALPHA_SPEED;
			else BackAlpha = BACKALPHA;

			//背景方块
			static bool init = false;
			if (!init || XMsg::WindowMsg::IsWindowResize())
			{
				int w = WindowSize.x / 1.5, h = WindowSize.y / 1.5;
				if (!init) LoginBar.x.SetAnimationStartValue(w * 0.5);
				LoginBar.x.SetAnimation(w, 15);
				if (!init) LoginBar.y.SetAnimationStartValue(h * 0.5);
				LoginBar.y.SetAnimation(h, 15);

				init = true;
			}

			LoginBar.x.UpdateAnimation(XEase::EaseBasic::easeOut, 4);
			LoginBar.y.UpdateAnimation(XEase::EaseBasic::easeOut, 4);

			static Color FillColor = Setting::BackColor == Color(30, 30, 30) ? Color(45, 45, 45) : Color(220, 220, 220);
			XGraph::SetFillColor(FillColor);

			//缓存方块布局，供各绘制函数统一使用，保证它们与背景方块始终对齐
			LoginBarW = int(LoginBar.x.value);
			LoginBarH = int(LoginBar.y.value);
			LoginBarX = WindowSize.x / 2 - LoginBarW / 2;
			LoginBarY = WindowSize.y / 2 - LoginBarH / 2;
			XGraph::RectangleShape::FillRoundRect_WithoutBorder(LoginBarX, LoginBarY, LoginBarW, LoginBarH, ROUNDSIZE, window);

			//内容绘制：标题 → 输入框 → 帮助链接，按从上到下的顺序叠加
			DrawTitle(window, Clock);
			DrawInputBoxes(window, Clock);
			DrawLoginButton(window, Clock);
			DrawHelpLink(window);
		}
	}
}
