#include "Menu.h"
#include "Shared.h"
#include "info/Message.h"
#include "main/Lang.h"
#include "user/User.h"
#include "main/Debug.h"
#include "SideBar.h"

//左上角菜单

namespace Menu
{
	NOXS; NOSTD;

	static RenderTexture rt;
	//脏标记
	static bool NeedReRraw = false;

	//子项0为该项名称例如文件(F)，从1开始为子项内容
	static vector<vector<wstring>> SideBarName;

	//按钮文字距离
	static int FontSpace;

	//展开标记，-1表示没有
	static int ExpMenuIndex = -1;
	static int ExpIndexTemp = -1;
	static EV2 ExpRectSize;
	static EV2 ExpRectPos;
	static EV ExpRectAlpha;
	static EV ExpChildTextAlpha;

	//单一子项大小
	static int ChildSize;

	//子菜单阴影
	static IMAGE Shadow;

	int h;

	//辅助函数
	namespace
	{
		bool ParseText(
			const std::wstring& text,
			std::wstring& outPrefix,   // xx
			std::wstring& outSuffix    // (x)
		)
		{
			outPrefix.clear();
			outSuffix.clear();

			if (text.empty())
				return false;

			// 查找最后一个 '('
			size_t pos = text.rfind(L'(');
			if (pos == std::wstring::npos)
				return false;

			// 必须以 ')' 结尾
			if (text.back() != L')')
				return false;

			// 至少要有 "x(x)"
			if (pos == 0 || pos + 2 > text.size())
				return false;

			outPrefix = text.substr(0, pos);
			outSuffix = text.substr(pos);

			return true;
		}
	}

	//初始化
	void Init()
	{
		//文字距离初始化
		FontSpace = ScreenSize.x * 0.04;

		//首次启动刷新
		NeedReRraw = true;

		//配置解释器
		wstring SideBarConfigPath = ExePath + L"\\Config\\Menu\\Config.ini";

		std::ifstream Config(SideBarConfigPath);
		if (!Config.is_open())
		{
			Message::ShowMessage(
				Translate::Translate(L"加载Gitta配置失败，重新安装或许能够解决问题。", UserData::Lang),
				Translate::Translate(L"加载配置失败", UserData::Lang),
				ICOTYPE_ERROR,
				{ L"确定" },
				3,
				L"Gitta"
			);
			exit(0x02);
		}

		std::string line;
		while (std::getline(Config, line))
		{
			// 跳过空行和 { }
			if (line.empty() || line == "{" || line == "}")
				continue;

			// 去掉行首行尾空格
			line.erase(0, line.find_first_not_of(" \t"));
			line.erase(line.find_last_not_of(" \t") + 1);

			if (line.rfind("name=", 0) == 0)
			{
				std::string nameValue = line.substr(5);
				if (nameValue.empty())
					continue;

				// 去掉 nameValue 首尾空格
				nameValue.erase(0, nameValue.find_first_not_of(" \t"));
				nameValue.erase(nameValue.find_last_not_of(" \t") + 1);

				wstring Value = XString::Convert::utf8_to_wstring(nameValue);
				wstring NameA, KeyA;
				ParseText(Value, NameA, KeyA);

				SideBarName.emplace_back();
				SideBarName.back().push_back(
					Translate::Translate(
						NameA,
						UserData::Lang
					) + KeyA
				);
			}
			else
			{
				if (!SideBarName.empty() && !line.empty())
				{
					SideBarName.back().push_back(
						Translate::Translate(
							XString::Convert::utf8_to_wstring(line),
							UserData::Lang
						)
					);
				}
			}
		}

		//单一子项大小
		ChildSize = ScreenSize.y * 0.03;

		//子项阴影
		XImage::NewImage(Shadow, ExePath + L"\\Image\\Menu\\Shadow.dll");
	}

	//显示菜单子项
	void DrawChild(RenWin& window)
	{
		//总大小
		static Vector2i AllSize = { int(ScreenSize.x * 0.1),int(ScreenSize.y * 0.03) };

		//更新动画
		ExpRectSize.x.UpdateAnimation(XEase::EaseBasic::easeOut, 4);
		ExpRectSize.y.UpdateAnimation(XEase::EaseBasic::easeOut, 4);
		ExpRectPos.x.UpdateAnimation(XEase::EaseBasic::easeOut, 4);
		ExpRectPos.y.UpdateAnimation(XEase::EaseBasic::easeOut, 4);
		ExpRectAlpha.UpdateAnimation(XEase::EaseBasic::easeOut, 4);
		ExpChildTextAlpha.UpdateAnimation(XEase::EaseBasic::easeOut, 4);

		//刷新大小
		if (ExpMenuIndex != ExpIndexTemp)
		{
			ExpIndexTemp = ExpMenuIndex;

			//显示
			if(ExpMenuIndex > -1)
			{
				//计算画布新的大小
				AllSize.y = int((SideBarName[ExpMenuIndex].size() - 1) * ChildSize);

				//设置动画
				ExpRectSize.x.SetAnimationStartValue(AllSize.x * 0.9);
				ExpRectSize.x.SetAnimation(AllSize.x, 10);

				ExpRectSize.y.SetAnimationStartValue(AllSize.y * 0.9);
				ExpRectSize.y.SetAnimation(AllSize.y, 10);

				//如果之前显示了的，就直接平滑过来
				if(ExpRectAlpha.value > 0)
				{
					ExpRectPos.x.SetAnimation(FontSpace * 0.3 + FontSpace * ExpMenuIndex, 10);
					ExpRectPos.y.SetAnimation(ScreenSize.y * 0.03, 10);
				}
				//显示
				else
				{
					ExpRectPos.x.SetAnimationStartValue(FontSpace * ExpMenuIndex);
					ExpRectPos.x.SetAnimation(FontSpace * 0.3 + FontSpace * ExpMenuIndex, 10);
					ExpRectPos.y.SetAnimationStartValue(ScreenSize.y * 0.02);
					ExpRectPos.y.SetAnimation(ScreenSize.y * 0.03, 10);
				}

				ExpRectAlpha.SetAnimationStartValue(100);
				ExpRectAlpha.SetAnimation(240, 10);
				ExpChildTextAlpha.SetAnimationStartValue(0);
				ExpChildTextAlpha.SetAnimation(255, 10);
			}
			//隐藏
			else
			{
				ExpRectAlpha.SetAnimation(0, 10);

				ExpRectSize.x.SetAnimation(AllSize.x * 0.9, 10);
				ExpRectSize.y.SetAnimation(AllSize.y * 0.9, 10);
			}
		}

		//不显示
		if (ExpRectAlpha.value < 0) return;

		//绘制阴影
		Shadow.color.a = ExpRectAlpha.value / 2;
		static float ShadowScale = 0.2;
		float RectScaleX = ExpRectSize.x.value * (1 + ShadowScale) / 512.0, RectScaleY = ExpRectSize.y.value * (1 + ShadowScale) / 512.0;
		XImage::PutScaleImage(Shadow,
			ExpRectPos.x.value - ExpRectSize.x.value * ShadowScale / 2,
			ExpRectPos.y.value - ExpRectSize.y.value * ShadowScale / 2,
			RectScaleX, RectScaleY, window);

		//绘制底色
		static Color BackFillColor = UserData::BackColor == Color(30, 30, 30) ? Color(50, 50, 50) : Color(250, 250, 250);
		BackFillColor.a = ExpRectAlpha.value;
		XGraph::SetFillColor(BackFillColor);
		static int ChildRectY = ScreenSize.y * 0.03;
		XGraph::RectangleShape::FillRoundRect_WithoutBorder(
			ExpRectPos.x.value, ExpRectPos.y.value, ExpRectSize.x.value, ExpRectSize.y.value, ROUNDSIZE, window);

		//绘制文本
		if(ExpMenuIndex > -1)
		{
			for (int i = 1; i < SideBarName[ExpMenuIndex].size(); i++)
			{
				//绘制选中底色
				if (XMsg::MouseMsg::IsMouseIn(ExpRectPos.x.value, ExpRectPos.y.value + ChildSize * (i - 1), ExpRectSize.x.value, ChildSize))
				{
					static Color FillColor = UserData::BackColor == Color(30, 30, 30) ? Color(100, 100, 100) : Color(220, 220, 220);
					XGraph::SetFillColor(FillColor);
					XGraph::RectangleShape::FillRoundRect_WithoutBorder(
						ExpRectPos.x.value, ExpRectPos.y.value + ChildSize * (i - 1), ExpRectSize.x.value, ChildSize, ROUNDSIZE, window);
				}

				static int FontSize = FONTSIZE * 0.9;

				XText::SetFontConfig(XColor::AlphaColor(FONTCOLOR, ExpChildTextAlpha.value), FontSize);
				XText::SetFontAdjust(ADJUST_LEFT, ADJUST_CENTER);

				static int LeftSpace = FontSpace * 0.2;

				XText::Xyprintf(
					ExpRectPos.x.value + LeftSpace,
					ExpRectPos.y.value - ChildSize * 0.5 + ChildSize * i,
					SideBarName[ExpMenuIndex][i], window);
			}
		}
	}

	//显示菜单
	void Draw(RenWin& window)
	{
		//初始化
		static bool IsInit = false;
		if (!IsInit)
		{
			Init();

			IsInit = true;
		}

		//静态菜单大小
		static int Size = SideBarName.size();

		//渲染
		static int w = ScreenSize.x * 0.3;
		
		if (NeedReRraw || XMsg::WindowMsg::IsWindowResize())
		{
			if (rt.resize({ unsigned int(w),unsigned int(h) }))
			{
				NeedReRraw = false;

				rt.clear(Color::Transparent);

				//调试
				Debug::DrawDebugRect(rt, 0, 0, w, h);

				static int FontSize = FONTSIZE;

				XText::SetFontConfig(FONTCOLOR, FontSize);
				XText::SetFontAdjust(ADJUST_CENTER, ADJUST_CENTER);

				for (int i = 0; i < Size; i++)
				{
					//选中底色
					if (ExpMenuIndex == i)
					{
						static Color FillColor = UserData::BackColor == Color(30, 30, 30) ? Color(70, 70, 70, 220) : Color(200, 200, 200, 220);
						XGraph::SetFillColor(FillColor);
						XGraph::RectangleShape::FillRoundRect_WithoutBorder(FontSpace * i, h * 0.2, FontSpace, h * 0.6, ROUNDSIZE, window);
					}

					wstring name = SideBarName[i][0];
					XText::Xyprintf(FontSpace / 2 + FontSpace * i, h / 2, name, rt);

					//调试
					Debug::DrawDebugRect(rt, FontSpace * i, 0, FontSpace, h);
				}

				rt.display();
			}
		}

		Sprite s(rt.getTexture());
		s.setPosition({ 0.0,0.0 });
		window.draw(s);

		//绘制子项
		DrawChild(window);
	}

	//菜单消息
	void Msg(RenWin& window)
	{
		//静态菜单大小
		static int Size = SideBarName.size();
		if(Size < 1) Size = SideBarName.size();

		//鼠标
		for (int i = 0; i < Size; i++)
		{
			//当没有展开时，点击才展开
			if (ExpMenuIndex == -1)
			{
				if (XMsg::MouseMsg::IsMouseDown(VK::MouseLeft))
				{
					if (XMsg::MouseMsg::IsMouseIn(FontSpace * i, 0, FontSpace, h))
					{
						ExpMenuIndex = i;
						NeedReRraw = true;
					}
				}
			}
			//否则移动到对应项直接自动展开
			else
			{
				if (XMsg::MouseMsg::IsMouseIn(FontSpace * i, 0, FontSpace, h))
				{
					ExpMenuIndex = i;
					NeedReRraw = true;
				}
			}
		}
		if (XMsg::KeyMsg::IsKeyDown(VK::Alt))
		{
			//文件
			if (XMsg::KeyMsg::IsKeyDown(VK::F))
			{
				if (ExpMenuIndex != 0) ExpMenuIndex = 0;
				else ExpMenuIndex = -1;
			}
			//编辑
			if (XMsg::KeyMsg::IsKeyDown(VK::E))
			{
				if (ExpMenuIndex != 1) ExpMenuIndex = 1;
				else ExpMenuIndex = -1;
			}
			//项目
			if (XMsg::KeyMsg::IsKeyDown(VK::P))
			{
				if (ExpMenuIndex != 2) ExpMenuIndex = 2;
				else ExpMenuIndex = -1;
			}
			//工具
			if (XMsg::KeyMsg::IsKeyDown(VK::T))
			{
				if (ExpMenuIndex != 3) ExpMenuIndex = 3;
				else ExpMenuIndex = -1;
			}
			//帮助
			if (XMsg::KeyMsg::IsKeyDown(VK::H))
			{
				if (ExpMenuIndex != 4) ExpMenuIndex = 4;
				else ExpMenuIndex = -1;
			}
			//关于
			if (XMsg::KeyMsg::IsKeyDown(VK::A))
			{

			}
		}

		//菜单子项
		if (ExpMenuIndex > -1)
		{
			if (XMsg::MouseMsg::IsMouseDown(VK::MouseLeft))
			{
				//在菜单内->操作
				if (XMsg::MouseMsg::IsMouseIn(
					FontSpace * ExpMenuIndex, 0, ExpRectSize.x.value + FontSpace * 0.3, ExpRectSize.y.value + h))
				{

				}

				if (ExpIndexTemp == ExpMenuIndex)
				{
					ExpMenuIndex = -1;
				}
			}

			//子项展开时要清除所有消息
			XMsg::ClearMsg();
		}


	}
}