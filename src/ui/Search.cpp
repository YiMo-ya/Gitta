#include "Search.h"
#include "SideBar.h"
#include "Shared.h"
#include "user/User.h"

namespace UI
{
	NOXS; NOSTD;

	static void ChangeInt(int& value, int speed, int minnum, int maxnum)
	{
		value += speed;
		value = min(value, maxnum);
		value = max(value, minnum);
	}

	//输入框
	class InPutRectUI
	{
		int CurClock = 0;
		std::string numstr;
		xs::EaseValue CurLocalX;
		int CurAlpha = 0;

		size_t CaretPos = 0;          // 光标在第几个 UTF‑8 字符后
		float  CaretPixelX = 0.f;     // 光标像素位置

	public:
		int x = 0, y = 0, w = 0, h = 0;
		int fx = 0, fy = 0;
		float CenterX = 0.0, CenterY = 0.0;
		bool IsCin = false;
		int CurSpeed = 1;
		std::string tips;

		std::string prefix;
		std::string Suffix;
		int nummin = 0, nummax = 0;
		bool EnableClampMin = false;
		bool EnableClampMax = false;

		Color TipColor = Color(150, 150, 150);
		Color TextColor = Color(255, 255, 255);
		Color CurColor = Color(255, 255, 255);

		void Draw(Rt& dest, int& num);
		void Draw(Rt& dest, std::string& str);

		// ====== 新增工具函数 ======
	private:
		static size_t Utf8CharCount(const std::string& s);
		static size_t Utf8OffsetByChars(const std::string& s, size_t charCount);
		static std::string Utf8Substr(const std::string& s, size_t pos, size_t len);

		void SyncCaretPixel(const std::string& str);
		void SetCaretFromMouse(int mouseX, const std::string& str);
		void ApplyNum(int& num);
	};

	//UTF8 工具
#pragma region MyRegion

	static size_t utf8_seq_len(unsigned char b)
	{
		if ((b & 0x80) == 0x00) return 1;
		if ((b & 0xE0) == 0xC0) return 2;
		if ((b & 0xF0) == 0xE0) return 3;
		if ((b & 0xF8) == 0xF0) return 4;
		return 1;
	}

	size_t InPutRectUI::Utf8CharCount(const std::string& s)
	{
		size_t count = 0, i = 0;
		while (i < s.size())
		{
			i += utf8_seq_len((unsigned char)s[i]);
			++count;
		}
		return count;
	}

	size_t InPutRectUI::Utf8OffsetByChars(const std::string& s, size_t charCount)
	{
		size_t bytePos = 0, chars = 0;
		while (bytePos < s.size() && chars < charCount)
		{
			bytePos += utf8_seq_len((unsigned char)s[bytePos]);
			++chars;
		}
		return bytePos;
	}

	std::string InPutRectUI::Utf8Substr(const std::string& s, size_t pos, size_t len)
	{
		size_t start = Utf8OffsetByChars(s, pos);
		size_t end = Utf8OffsetByChars(s, pos + len);
		return s.substr(start, end - start);
	}

#pragma endregion

	//文字输入框
#pragma region MyRegion



	void InPutRectUI::SyncCaretPixel(const std::string& str)
	{
		// 光标像素位置 = 前缀宽度 + 光标前文本宽度；
		// 之前漏算 prefix，导致有前缀时光标整体左移
		CaretPixelX = XText::GetFontWidth(prefix) +
			XText::GetFontWidth(Utf8Substr(str, 0, CaretPos));
		CurLocalX.value = CurLocalX.end;
		CurLocalX.SetAnimation(CaretPixelX, 10 / CurSpeed);
	}

	void InPutRectUI::SetCaretFromMouse(int mouseX, const std::string& str)
	{
		// 统一坐标系：鼠标点相对 fx 的偏移，与「前缀宽度 + 光标前文本宽度」直接比较，
		// 避免扣除再叠加前缀宽度带来的二次误差
		const float PrefixW = XText::GetFontWidth(prefix);
		const float localX = (float)(mouseX - fx);
		size_t bestPos = 0;
		float bestDist = FLT_MAX;

		size_t total = Utf8CharCount(str);
		for (size_t i = 0; i <= total; ++i)
		{
			// 候选光标位置（含前缀）与鼠标位置同为 fx 基准坐标
			float px = PrefixW + XText::GetFontWidth(Utf8Substr(str, 0, i));
			float dist = std::fabs(px - localX);
			if (dist < bestDist)
			{
				bestDist = dist;
				bestPos = i;
			}
		}
		CaretPos = bestPos;
		SyncCaretPixel(str);
	}

	void InPutRectUI::ApplyNum(int& num)
	{
		if (numstr.empty())
			return;

		try
		{
			int temp = std::stoi(numstr);

			if (EnableClampMin && temp < nummin)
				temp = nummin;

			if (EnableClampMax && temp > nummax)
				temp = nummax;

			num = temp;
			numstr.clear();
		}
		catch (...)
		{
			// 非法输入，回滚
			num = 0;
			numstr.clear();
		}
	}

	static void utf8_pop_back(std::string& str)
	{
		if (str.empty())
			return;

		size_t pos = str.size();

		// 最多回退 4 个字节（UTF‑8 最长编码）
		for (int i = 0; i < 4 && pos > 0; ++i)
		{
			unsigned char b = str[pos - 1];

			// 单字节字符
			if ((b & 0x80) == 0x00)
			{
				--pos;
				break;
			}

			// 多字节首字节
			if ((b & 0xE0) == 0xC0 ||  // 2 字节
				(b & 0xF0) == 0xE0 ||  // 3 字节
				(b & 0xF8) == 0xF0)    // 4 字节
			{
				--pos;
				break;
			}

			--pos;
		}

		str.erase(pos);
	}

	static int BarWidth = XSystem::Info::GetScreenSize().x * 0.001;

	//字符
	void InPutRectUI::Draw(Rt& dest, std::string& str)
	{
		int TempX = x - w * CenterX, TempY = y - h * CenterY;

		CurLocalX.UpdateAnimation(XEase::EaseBasic::easeOut, 7);

		// === 鼠标点击 ===
		if (XMsg::MouseMsg::IsMouseDown(VK::MouseLeft))
		{
			if (XMsg::MouseMsg::IsMouseIn(TempX, TempY, w, h))
			{
				IsCin = true;
				SetCaretFromMouse(XMsg::MouseMsg::GetMousePosWindow().x, str);
			}
			else
			{
				IsCin = false;
				CaretPos = Utf8CharCount(str);
				SyncCaretPixel(str);
				CurClock = 30;
			}
		}

		if (IsCin)
		{
			// IME 位置
			if (CurClock < 30) CurClock += CurSpeed;
			else
			{
				CurClock = 0;
				XMsg::KeyMsg::SetIMEPos(fx + CaretPixelX, y);
			}

			// === 键盘 ← → ===
			if (XMsg::KeyMsg::IsKeyDown(VK_LEFT) && CaretPos > 0)
			{
				--CaretPos;
				SyncCaretPixel(str);
			}
			if (XMsg::KeyMsg::IsKeyDown(VK_RIGHT) &&
				CaretPos < Utf8CharCount(str))
			{
				++CaretPos;
				SyncCaretPixel(str);
			}

			std::string cinstr = XMsg::KeyMsg::GetInPut();
			if (!cinstr.empty())
			{
				if (cinstr == "/b")
				{
					if (CaretPos > 0)
					{
						std::string before = Utf8Substr(str, 0, CaretPos - 1);
						std::string after = Utf8Substr(str, CaretPos, Utf8CharCount(str));
						str = before + after;
						--CaretPos;
						SyncCaretPixel(str);
					}
				}
				else if (cinstr == "/n")
				{
					IsCin = false;
					CaretPos = Utf8CharCount(str);
					SyncCaretPixel(str);
					CurClock = 30;
				}
				else
				{
					std::string before = Utf8Substr(str, 0, CaretPos);
					std::string after = Utf8Substr(str, CaretPos, Utf8CharCount(str));
					str = before + cinstr + after;

					CaretPos += Utf8CharCount(cinstr);

					SyncCaretPixel(str);
				}
			}

			if (XMsg::KeyMsg::Keystate(VK::Enter))
			{
				IsCin = false;
				CaretPos = Utf8CharCount(str);
				SyncCaretPixel(str);
				CurClock = 30;
			}
		}
		else
		{
			CaretPos = Utf8CharCount(str);
			SyncCaretPixel(str);
		}

		// 光标闪烁
		if (IsCin && (CurClock < 15 || CurLocalX.IsAnimation()))
			ChangeInt(CurAlpha, 100 * CurSpeed, 0, 255);
		else
			ChangeInt(CurAlpha, -50 * CurSpeed, 0, 255);

		// 绘制
		if (IsCin || !str.empty())
		{
			XText::SetFontColor(XColor::AlphaColor(CurColor, CurAlpha));
			XText::Xyprintf(fx + CurLocalX.value + BarWidth, fy, "I", dest);
			XText::SetFontColor(TextColor);
			XText::Xyprintf(fx, fy, prefix + str + Suffix, dest);
		}
		else
		{
			XText::SetFontColor(TipColor);
			XText::Xyprintf(fx, fy, tips, dest);
		}
	}

	//数字
	void InPutRectUI::Draw(Rt& dest, int& num)
	{
		int TempX = x - w * CenterX, TempY = y - h * CenterY;
		CurLocalX.UpdateAnimation(XEase::EaseBasic::easeOut, 7);

		if (XMsg::MouseMsg::IsMouseDown(VK::MouseLeft))
		{
			if (XMsg::MouseMsg::IsMouseIn(TempX, TempY, w, h))
			{
				if (!IsCin)
				{
					IsCin = true;
					numstr = std::to_string(num);
					CaretPos = Utf8CharCount(numstr);
					SyncCaretPixel(numstr);
				}
			}
			else
			{
				IsCin = false;
				CaretPos = Utf8CharCount(numstr);
				SyncCaretPixel(numstr);

				ApplyNum(num);

				CurClock = 30;
			}
		}

		if (IsCin)
		{
			if (CurClock < 30) CurClock += CurSpeed;
			else
			{
				CurClock = 0;
				XMsg::KeyMsg::SetIMEPos(fx + CaretPixelX, y);
			}

			if (XMsg::KeyMsg::IsKeyDown(VK_LEFT) && CaretPos > 0)
			{
				--CaretPos;
				SyncCaretPixel(numstr);
			}
			if (XMsg::KeyMsg::IsKeyDown(VK_RIGHT) &&
				CaretPos < Utf8CharCount(numstr))
			{
				++CaretPos;
				SyncCaretPixel(numstr);
			}

			std::string cinstr = XMsg::KeyMsg::GetInPut();
			if (!cinstr.empty())
			{
				if (cinstr == "/b")
				{
					if (CaretPos > 0)
					{
						std::string before = Utf8Substr(numstr, 0, CaretPos - 1);
						std::string after = Utf8Substr(numstr, CaretPos, Utf8CharCount(numstr));
						numstr = before + after;
						--CaretPos;
						SyncCaretPixel(numstr);
					}
				}
				else if (cinstr == "/n")
				{
					IsCin = false;
					ApplyNum(num);
					CaretPos = Utf8CharCount(numstr);
					SyncCaretPixel(numstr);
					CurClock = 30;
				}
				else if (cinstr != " " && cinstr != "/n")
				{
					std::string digit = XMsg::KeyMsg::GetInPutNumber("");
					if (!digit.empty())
					{
						std::string before = Utf8Substr(numstr, 0, CaretPos);
						std::string after = Utf8Substr(numstr, CaretPos, Utf8CharCount(numstr));
						numstr = before + digit + after;
						++CaretPos;
						SyncCaretPixel(numstr);
					}
				}
			}

			if (XMsg::KeyMsg::Keystate(VK::Enter))
			{
				IsCin = false;
				ApplyNum(num);
				CaretPos = Utf8CharCount(numstr);
				SyncCaretPixel(numstr);
				CurClock = 30;
			}
		}
		else
		{
			CaretPos = Utf8CharCount(std::to_string(num));
			SyncCaretPixel(std::to_string(num));
		}

		if (IsCin && (CurClock < 15 || CurLocalX.IsAnimation()))
			ChangeInt(CurAlpha, 100 * CurSpeed, 0, 255);
		else
			ChangeInt(CurAlpha, -50 * CurSpeed, 0, 255);

		if (IsCin)
		{
			XText::SetFontColor(XColor::AlphaColor(CurColor, CurAlpha));
			XText::Xyprintf(fx + CurLocalX.value + BarWidth, fy, "I", dest);
			XText::SetFontColor(TextColor);
			XText::Xyprintf(fx, fy, prefix + numstr + Suffix, dest);
		}
		else
		{
			XText::SetFontColor(XColor::AlphaColor(CurColor, CurAlpha));
			XText::Xyprintf(fx + CurLocalX.value + BarWidth, fy, "I", dest);
			XText::SetFontColor(TextColor);
			XText::Xyprintf(fx, fy, prefix + std::to_string(num) + Suffix, dest);
		}
	}
#pragma endregion

}

namespace Search
{
	NOXS; NOSTD;

	int SearchBoxH;

	void Draw(RenWin& window)
	{
		static string SearchText;

		//位置
		int x = SideBar::Level1::w;
		static int y = ScreenSize.y * 0.01;
		int w = WindowSize.x / 2;

		static UI::InPutRectUI ui;
		ui.x = x, ui.y = y, ui.w = w, ui.h = SearchBoxH;
		ui.fx = x + UISPACE, ui.fy = y + SearchBoxH / 2;
		ui.tips = "> 键入以搜索";
		ui.prefix = "> ";

		static bool init = false;
		if (!init)
		{
			SearchBoxH = ScreenSize.y  * 0.04;

			init = true;
		}

		//绘制底色
		static Color FillColor = UserData::BackColor == Color(30, 30, 30) ? Color(20, 20, 20, 200) : Color(255, 255, 255, 200);
		static Color BorderColor = UserData::BackColor == Color(30, 30, 30) ? Color(100, 100, 100, 255) : Color(200, 200, 200, 255);
		static int lw = ScreenSize.x / 1920.0;
		static int BorderAlpha;
		XGraph::SetFillColor(FillColor);
		XGraph::SetColor(BorderColor);
		XGraph::LineShape::SetLineWidth(lw);
		XGraph::RectangleShape::FillRoundRect(x, y, w, SearchBoxH, ROUNDSIZE, window);
		if (BorderAlpha > 0)
		{
			static Color color = UserData::MainColor;
			color.a = BorderAlpha;
			XGraph::SetColor(color);
			XGraph::RectangleShape::RoundRect(x, y, w, SearchBoxH, ROUNDSIZE, window);
		}

		static int SPEED = 50;
		if (XMsg::MouseMsg::IsMouseIn(x, y, w, SearchBoxH) || ui.IsCin)
		{
			if (BorderAlpha + SPEED < 255) BorderAlpha += SPEED;
			else BorderAlpha = 255;
		}
		else
		{
			if (BorderAlpha - SPEED > 0) BorderAlpha -= SPEED;
			else BorderAlpha = 0;
		}

		//输入逻辑
		ui.Draw(window, SearchText);
	}
}