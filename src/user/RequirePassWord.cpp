#include "RequirePassWord.h"
#include "UserData.h"
#include "lock/Lock.h"
#include "ui/UI.h"
#include "Set.h"

namespace RequirePassWord
{
	NOXS; NOSTD;

	bool IsRequire = false;
	bool IsPassWordF = false;

	//校验通过的密码明文，仅保留到业务方取走一次
	static std::string PassWord;

	//密码框输入缓冲：界面关闭后清空，避免明文残留可被再次读取
	static std::string PassWordInput;

	//密码框排版缓存，由 Enter 中的背景方块动画写入，供绘制函数读取
	static int BarX = 0, BarY = 0;
	static int BarW = 0, BarH = 0;

	//密码输入框排版缓存，由 DrawInputBox 每帧写入，供 DrawLoginButton 定位到输入框右侧
	static int InputBoxX = 0, InputBoxY = 0;
	static int InputBoxW = 0, InputBoxH = 0;

	//每次进入界面时的初始化标记，替代各绘制函数内的 static init：
	//static init 只会为 true 一次，第二次调用 Enter 时入场动画不再重新配置，导致显示异常。
	//这里改为在 Enter 开头统一复位，保证每次打开的入场表现完全一致。
	static bool TitleInit = true;
	static bool InputBoxInit = true;
	static bool LoginButtonInit = true;

	//密码错误抖动动画的总帧数与剩余帧数。
	static int ShakeTotalFrame = 0;
	static int ShakeRemainFrame = 0;

	/// <summary>
	/// 计算当前帧的抖动水平偏移量，并推进抖动帧计数。
	/// 无抖动（剩余帧为 0）时返回 0，保证输入框回到原位。
	/// </summary>
	/// <param name="intensity">起始抖动幅度（像素）。</param>
	/// <returns>本帧应叠加的水平偏移量，正负交替实现左右抖动。</returns>
	static float ShakeAnimation(float intensity)
	{
		if (ShakeRemainFrame <= 0) return 0.0f;

		//已播放帧数：总帧数减去剩余帧数
		int played = ShakeTotalFrame - ShakeRemainFrame;

		//衰减系数随时间线性减小，抖动幅度逐渐收敛
		float t = ShakeTotalFrame > 0 ? static_cast<float>(played) / ShakeTotalFrame : 1.0f;
		float damping = 1.0f - t;

		//高频左右抖动：奇偶帧交替方向
		float sign = (played % 2 == 0) ? 1.0f : -1.0f;

		ShakeRemainFrame--;
		if (ShakeRemainFrame <= 0)
		{
			//最后一帧必须归位，避免抖动残留半像素偏移
			ShakeRemainFrame = 0;
			return 0.0f;
		}

		return sign * intensity * damping;
	}

	/// <summary>
	/// 判断错误抖动是否仍在播放，供绘制函数决定是否维持红色描边。
	/// </summary>
	/// <returns>抖动尚未播放完毕返回 true。</returns>
	static bool IsShakePlaying()
	{
		return ShakeRemainFrame > 0;
	}

	void RequirePassWord()
	{
		IsRequire = true;
		//每次重新要求输入密码时清空上一次的结果，业务方必须先处理上一条密码
		PassWord.clear();
		IsPassWordF = false;
	}

	bool IsPassWordFinish()
	{
		if (!IsPassWordF) return false;
		IsPassWordF = false;
		return true;
	}

	//当前帧的抖动水平偏移量，由 Enter 每帧写入，DrawInputBox 读取
	static float ShakeOffset = 0.0f;

	//绘制函数共享的动画与描边状态。放在文件作用域而非函数内 static，
	//这样 Enter 每次进入时都能统一复位，避免第二次调用沿用上一轮的动画结束态与残留高亮
	static EV TitlePosYOffset;
	static EV PassPosYOffset;
	static EV LoginBoxSize;
	static int BorderAlpha_Input = 0;
	static int ErrorBorderAlpha_Input = 0;
	static int BorderAlpha_Login = 0;

	/// <summary>
	/// 复位界面在多次调用之间共享的动画与描边状态。
	/// Enter 每次进入时调用一次，使第二次及之后的显示与首次完全一致。
	/// </summary>
	static void ResetViewState()
	{
		//初始化标记复位后，各绘制函数会重新配置入场动画的起始值与目标值
		TitleInit = true;
		InputBoxInit = true;
		LoginButtonInit = true;

		//动画对象内部保存起始值、目标值与当前帧，必须重新赋值才能重播入场效果
		TitlePosYOffset = EV();
		PassPosYOffset = EV();
		LoginBoxSize = EV();

		//悬停与错误描边透明度清零，否则上一次残留的高亮会直接显示在新界面首帧上
		BorderAlpha_Input = 0;
		ErrorBorderAlpha_Input = 0;
		BorderAlpha_Login = 0;

		//抖动动画计数归零，防止上轮未播完的抖动延续到本次
		ShakeOffset = 0.0f;
		ShakeTotalFrame = 0;
		ShakeRemainFrame = 0;
	}

	/// <summary>
	/// 绘制标题文字，并处理标题的下落与淡入动画。
	/// </summary>
	/// <param name="window">渲染目标窗口。</param>
	/// <param name="Clock">全局计时器，用于延迟启动淡入动画。</param>
	static void DrawTitle(RenWin& window, int Clock)
	{
		//动画每帧推进一次；初始化标记由 ResetViewState 每次进入时复位，保证重复调用可重播入场
		if (TitleInit)
		{
			TitleInit = false;
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
		XText::Xyprintf(BarX + BarW / 2,
			BarY + int(BarH * 0.08) + int(TitlePosYOffset.value),
			L"请输入账户密码", window);
	}

	/// <summary>
	/// 绘制受保护的密码输入框，并处理淡入、悬停边框高亮与输入。
	/// </summary>
	/// <param name="window">渲染目标窗口。</param>
	/// <param name="Clock">全局计时器，用于延迟启动淡入动画。</param>
	static void DrawInputBox(RenWin& window, int Clock)
	{
		static UI::InPutRectUI PassInput;
		static Color PassBoxColor = Color(255, 255, 255, 0);
		//悬停边框与错误描边透明度使用文件作用域变量，便于每次进入界面时统一复位
		int& BorderAlpha = BorderAlpha_Input;
		int& ErrorBorderAlpha = ErrorBorderAlpha_Input;

		//初始化标记由 ResetViewState 每次进入复位，保证第二次调用也能重播上浮入场
		if (InputBoxInit)
		{
			InputBoxInit = false;
			PassBoxColor = Color(255, 255, 255, 0);

			//与登录界面密码框一致的上浮入场
			PassPosYOffset.SetAnimationStartValue(ScreenSize.y * 0.05);
			PassPosYOffset.SetAnimation(0, 20);
		}

		//淡入：比标题更晚启动，形成依次入场效果
		if (Clock > 15)
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

		//输入框尺寸：每帧按背景方块实际宽度计算，避免首帧为 0 导致尺寸错乱
		int BoxW = int(BarW * 0.5);
		int BoxH = int(BoxW * 0.08);
		if (BoxH < FONTSIZE * 2) BoxH = FONTSIZE * 2;

		//只放一个输入框，因此纵向居中于方块中部，视觉上更平衡
		int boxX = BarX + (BarW - BoxW) / 2;
		int boxY = BarY + int(BarH * 0.42) + int(PassPosYOffset.value);

		//抖动偏移只叠加在水平方向，保持纵向布局稳定
		boxX += int(ShakeOffset);

		//缓存当前帧的输入框矩形，供登录按钮做右侧对齐
		InputBoxX = boxX; InputBoxY = boxY; InputBoxW = BoxW; InputBoxH = BoxH;

		//配色：底色比背景方块略深(暗色) / 略浅(亮色)，并套用淡入 alpha
		const Color BoxBaseFill = Setting::BackColor == Color(30, 30, 30) ? Color(60, 60, 60) : Color(235, 235, 235);
		Color FillColor = XColor::AlphaColor(BoxBaseFill, (uint8_t)PassBoxColor.a);

		//悬停渐变：仅在输入框实际可见后才推进，避免淡入阶段边框提前亮起
		if (PassBoxColor.a > 0 && XMsg::MouseMsg::IsMouseIn(boxX, boxY, BoxW, BoxH))
		{
			if (BorderAlpha + 30 < 255) BorderAlpha += 30;
			else BorderAlpha = 255;
		}
		else
		{
			if (BorderAlpha - 30 > 0) BorderAlpha -= 30;
			else BorderAlpha = 0;
		}

		//先画圆角底色，再按需叠加主色细边框（边框 alpha 即时构造，避免污染共享颜色对象）
		XGraph::SetFillColor(FillColor);
		XGraph::RectangleShape::FillRoundRect_WithoutBorder(boxX, boxY, BoxW, BoxH, ROUNDSIZE, window);
		if (BorderAlpha > 0)
		{
			XGraph::SetColor(XColor::AlphaColor(Setting::MainColor, (uint8_t)BorderAlpha));
			XGraph::LineShape::SetLineWidth(1);
			XGraph::RectangleShape::RoundRect(boxX, boxY, BoxW, BoxH, ROUNDSIZE, window);
		}

		//校验失败时叠加红色描边反馈：抖动期间保持高亮，动画结束后渐隐
		if (IsShakePlaying())
		{
			ErrorBorderAlpha = 255;
		}
		else
		{
			if (ErrorBorderAlpha - 30 > 0) ErrorBorderAlpha -= 30;
			else ErrorBorderAlpha = 0;
		}
		if (ErrorBorderAlpha > 0)
		{
			XGraph::SetColor(Color(255, 0, 50, ErrorBorderAlpha));
			XGraph::LineShape::SetLineWidth(1);
			XGraph::RectangleShape::RoundRect(boxX, boxY, BoxW, BoxH, ROUNDSIZE, window);
		}

		static Color FontColor = Setting::BackColor == Color(30, 30, 30) ? Color(255, 255, 255) : Color(0, 0, 0);
		if (Clock > 10) FontColor.a = 255;

		Color TipColor = Color(150, 150, 150);
		TipColor.a = PassBoxColor.a;

		//不使用掩码：组件直接编辑并回写明文，PassWordInput 作为唯一数据源
		XText::SetFontSize(FONTSIZE);
		XText::SetFontAdjust(ADJUST_LEFT, ADJUST_CENTER);
		PassInput.x = boxX, PassInput.y = boxY, PassInput.w = BoxW, PassInput.h = BoxH;
		PassInput.fx = boxX + UISPACE, PassInput.fy = boxY + BoxH / 2;
		PassInput.prefix = "密码：";
		PassInput.tips = "密码：";
		PassInput.TextColor = FontColor;
		PassInput.CurColor = FontColor;
		PassInput.TipColor = TipColor;
		PassInput.Draw(window, PassWordInput);
	}

	/// <summary>
	/// 绘制密码框右侧的「登录」按钮，悬停时以主色描边作为可点击反馈。
	/// 按钮纵向与密码输入框居中对齐，横向紧贴输入框右边缘。
	/// </summary>
	/// <param name="window">渲染目标窗口。</param>
	/// <param name="Clock">全局计时器，用于与输入框同步淡入，避免按钮提前出现。</param>
	/// <returns>本帧按钮是否被点击，供调用方触发密码校验流程。</returns>
	bool DrawLoginButton(RenWin& window, int Clock)
	{
		static IMAGE img;
		//初始化标记由 ResetViewState 每次进入复位，让按钮动画在第二次调用时同样从 0 展开
		if (LoginButtonInit)
		{
			LoginButtonInit = false;

			LoginBoxSize.SetAnimationStartValue(0);
			LoginBoxSize.SetAnimation(100, 20);

			XImage::NewImage(img, ExePath + L"\\Image\\Login\\Go.dll");
			img.color = Setting::BackColor == Color(30, 30, 30) ? Color(255, 255, 255) : Color(0, 0, 0);
		}

		if (Clock > 20)
			LoginBoxSize.UpdateAnimation(XEase::EaseBasic::easeOut, 4);

		static int BasicSize = ScreenSize.x / 100;

		//按钮紧凑贴在密码框右侧：右边缘留出与输入框等宽的 UISPACE 间距
		int x = InputBoxX + InputBoxW + BasicSize + UISPACE,
			y = InputBoxY + InputBoxH / 2 + (100 - LoginBoxSize.value) * BasicSize / 40;

		bool IsIn = XMsg::MouseMsg::IsMouseIn(x - BasicSize, y - BasicSize, BasicSize * 2, BasicSize * 2);

		//绘制圆形背景
		const Color BoxBaseFill = Setting::BackColor == Color(30, 30, 30) ? Color(60, 60, 60) : Color(235, 235, 235);
		XGraph::SetFillColor(BoxBaseFill);
		XGraph::CircleShape::FillCircle_WithoutBorder(x, y, BasicSize * LoginBoxSize.value / 100, window);

		//绘制边框：透明度使用文件作用域变量，Enter 每次进入时由 ResetViewState 清零
		int& BorderAlpha = BorderAlpha_Login;
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
			//每次即时构造描边色，避免共享颜色对象被改写后影响其它绘制
			Color BorderColor = Setting::MainColor;
			BorderColor.a = BorderAlpha;
			XGraph::SetColor(BorderColor);
			XGraph::LineShape::SetLineWidth(1);
			XGraph::CircleShape::Circle(x, y, BasicSize * LoginBoxSize.value / 100, window);
		}

		//绘制箭头
		float Scale = BasicSize / 512.0 * LoginBoxSize.value / 100.0;
		XImage::PutScaleImage(img, x, y, Scale, Scale, window, 0.5, 0.5);

		return IsIn && XMsg::MouseMsg::IsMouseDown(VK::MouseLeft);
	}

	/// <summary>
	/// 只比对密码哈希，不加载用户数据。
	/// 沿用 UserData 的存档目录约定（LocalAppData\Gitta\&lt;用户名 SHA-256&gt;\Data.dat），
	/// 直接读取存档中的密码哈希与本次输入的哈希比较，避免 UserData::Load 的目录创建、
	/// 凭据写入与全局状态切换等耗时且有副作用的操作。
	/// </summary>
	/// <param name="passWord">用户输入的密码明文（UTF-8）。</param>
	/// <returns>输入密码与当前登录用户存档中的密码哈希一致返回 true；</returns>
	/// <returns>密码为空、当前未登录、存档缺失或哈希不一致返回 false。</returns>
	static bool CheckPassWord(const std::string& passWord)
	{
		if (passWord.empty()) return false;

		//当前会话必须已有登录用户，否则无从校验
		const wstring& name = UserData::UserName;
		if (name.empty()) return false;

		//直接复用已登录用户的存档目录，省去重复的哈希与目录拼装
		wstring dir = UserData::UserPath;
		if (dir.empty())
		{
			wchar_t buffer[MAX_PATH] = { 0 };
			DWORD len = GetEnvironmentVariableW(L"LOCALAPPDATA", buffer, MAX_PATH);
			if (len == 0 || len >= MAX_PATH) return false;

			string nameHash = Lock::Hash::SHA256Hex(name);
			if (nameHash.empty()) return false;
			//与 UserData::Load 保持一致的展开方式：64 个 hex 字符按字节直接拼接
			dir = wstring(buffer, len) + L"\\Gitta\\" + wstring(nameHash.begin(), nameHash.end());
		}

		//读取存档里保存的密码哈希（UserData 以单行十六进制文本写入 Data.dat）
		string storedHash;
		ifstream passFile(dir + L"\\Data.dat");
		if (!passFile.is_open()) return false;
		passFile >> storedHash;
		passFile.close();

		if (storedHash.empty()) return false;

		//只做一次 SHA-256 计算，远快于 Load 的文件系统操作
		return storedHash == Lock::Hash::SHA256Hex(XString::Convert::utf8_to_wstring(passWord));
	}

	void Enter(RenWin& window)
	{
		if (!IsRequire) return;

		IMAGE back;
		XImage::NewImage(window, back);

		EV2 Bar;

		int BackAlpha = 255;
		const int BACKALPHA = 100;
		const int BACKALPHA_SPEED = 15;

		//界面生命周期状态：等待输入 → 退出淡出
		bool Accepted = false;

		//记录校验通过的那一帧是否按下了回车：回车键的按下状态会持续若干帧，
		//若不识别它，进入退出循环后仍会被判定为「正在输入」而清空输入框并触发错误抖动
		bool AcceptByEnter = false;

		int Clock = 0;

		PassWordInput.clear();
		IsPassWordF = false;

		//每次进入都复位
		ResetViewState();

		//方块动画的初始化标记同样不能是函数内 static，否则第二次调用不会重新设置起始值
		bool BarInit = false;

		//配色按当前主题重新计算，避免用户中途切换深浅色后仍使用旧颜色
		const Color FillColor = Setting::BackColor == Color(30, 30, 30) ? Color(45, 45, 45) : Color(220, 220, 220);

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
			if (!BarInit || XMsg::WindowMsg::IsWindowResize())
			{
				int w = WindowSize.x / 1.5, h = WindowSize.y / 1.5;
				if (!BarInit) Bar.x.SetAnimationStartValue(w * 0.5);
				Bar.x.SetAnimation(w, 15);
				if (!BarInit) Bar.y.SetAnimationStartValue(h * 0.5);
				Bar.y.SetAnimation(h, 15);

				BarInit = true;
			}

			Bar.x.UpdateAnimation(XEase::EaseBasic::easeOut, 4);
			Bar.y.UpdateAnimation(XEase::EaseBasic::easeOut, 4);

			
			XGraph::SetFillColor(FillColor);
			XGraph::SetColor(Setting::MainColor);
			XGraph::LineShape::SetLineWidth(1);

			//缓存方块布局，供绘制函数统一使用，保证它们与背景方块始终对齐
			BarW = int(Bar.x.value);
			BarH = int(Bar.y.value);
			BarX = WindowSize.x / 2 - BarW / 2;
			BarY = WindowSize.y / 2 - BarH / 2;
			XGraph::RectangleShape::FillRoundRect(BarX, BarY, BarW, BarH, ROUNDSIZE, window);

			//抖动动画每帧推进一次；剩余帧为 0 时偏移恒为 0，输入框保持静止
			ShakeOffset = ShakeAnimation(2.5f);

			//内容绘制：标题 → 密码输入框 → 右侧登录按钮（输入框必须先绘制，按钮才能拿到对齐基准）
			DrawTitle(window, Clock);
			DrawInputBox(window, Clock);
			bool LoginClicked = DrawLoginButton(window, Clock);

			//回车与点击按钮共用同一条确认路径
			bool EnterPressed = XMsg::KeyMsg::IsKeyDown(VK::Enter);
			if (EnterPressed || LoginClicked)
			{
				//只做密码哈希比对，不调用 UserData::Load：
				//Load 会重建目录、读写凭据文件并切换全局用户状态，耗时明显且带副作用，
				//而当前会话的用户早已登录，这里只需要确认密码是否匹配
				if (CheckPassWord(PassWordInput))
				{
					Accepted = true;
					AcceptByEnter = EnterPressed;
					break;
				}

				//校验失败：重置抖动帧计数触发动画，红色描边随后渐隐
				ShakeTotalFrame = 15;
				ShakeRemainFrame = ShakeTotalFrame;
				PassWordInput.clear();
			}

			if (XMsg::MouseMsg::IsMouseDown(VK::MouseLeft) && !XMsg::MouseMsg::IsMouseIn(BarX, BarY, BarW, BarH))
			{
				break;
			}
		}

		//确认成功后写回明文，供业务方通过 GetPassWord() 读取
		if (Accepted)
		{
			PassWord = PassWordInput;
			IsPassWordF = true;
			IsRequire = false;
		}

		//输入缓冲用完即清，避免明文长期驻留
		PassWordInput.clear();

		IsRequire = false;
	}
}
