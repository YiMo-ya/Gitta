#include "UI.h"

NOXS; NOSTD;

//矩形UI边框渐变速度
const int RECT_BORDER_ALPHA_SPEED = 15;

//工具函数
namespace ToolFc
{
	//渐变
	void ChangeInt(int& Value, const int& Min, const int& Max, const int& Speed)
	{
		Value += Speed;

		Value = max(Min, min(Max, Value));
	}

}

namespace UI
{

	//矩形
#pragma region MyRegion

	void Rect::Draw(RenderTarget& target,int x,int y,int w,int h,int r)
	{
		IsInUI = XMsg::MouseMsg::IsMouseIn(x, y, w, h);

		//绘制边框
		if (EnableBorder)
		{
			if (IsInUI) ToolFc::ChangeInt(BorderAlpha, 0, 255, RECT_BORDER_ALPHA_SPEED);
			else ToolFc::ChangeInt(BorderAlpha, 0, 255, -RECT_BORDER_ALPHA_SPEED);

			if (BorderAlpha > 0)
			{
				XGraph::SetColor(BorderColor);
				if(r > 0) XGraph::RectangleShape::RoundRect(x, y, w, h, r, target);
				else XGraph::RectangleShape::Rect(x, y, w, h, target);
			}
		}

		//绘制背景
		XGraph::SetFillColor(BackColor);
		if (r > 0) XGraph::RectangleShape::FillRoundRect_WithoutBorder(x, y, w, h, r, target);
		else XGraph::RectangleShape::FillRect_WithoutBorder(x, y, w, h, target);
	}

	bool Rect::IsIn()
	{
		return IsInUI;
	}

	bool Rect::IsClick()
	{
		return XMsg::MouseMsg::IsMouseDown(VK::MouseLeft) && IsInUI;
	}

#pragma endregion

	//输入框
#pragma region MyRegion

	/// <summary>
	/// 返回 UTF-8 首字节所引导的字符占用字节数。
	/// </summary>
	/// <param name="b">UTF-8 序列的首字节。</param>
	/// <returns>该字符占用的字节数，非法首字节按单字节处理。</returns>
	static size_t Utf8SeqLen(unsigned char b)
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
			i += Utf8SeqLen((unsigned char)s[i]);
			++count;
		}
		return count;
	}

	size_t InPutRectUI::Utf8OffsetByChars(const std::string& s, size_t charCount)
	{
		size_t bytePos = 0, chars = 0;
		while (bytePos < s.size() && chars < charCount)
		{
			bytePos += Utf8SeqLen((unsigned char)s[bytePos]);
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

	void InPutRectUI::SyncCaretPixel(const std::string& str)
	{
		// 光标像素位置 = 前缀宽度 + 光标前文本宽度；
		// 漏算 prefix 会让有前缀的输入框光标整体左移
		CaretPixelX = XText::GetFontWidth(prefix) +
			XText::GetFontWidth(Utf8Substr(str, 0, CaretPos));
		CurLocalX.value = CurLocalX.end;
		CurLocalX.SetAnimation(CaretPixelX, 10 / CurSpeed);
	}

	void InPutRectUI::SetCaretFromMouse(int mouseX, const std::string& str)
	{
		// 统一坐标系：鼠标点相对 fx 的偏移与「前缀宽度 + 光标前文本宽度」直接比较，
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
			ToolFc::ChangeInt(CurAlpha, 0, 255, 100 * CurSpeed);
		else
			ToolFc::ChangeInt(CurAlpha, 0, 255, -50 * CurSpeed);

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
					// 数字框只接受数字字符，过滤掉其它输入
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
			ToolFc::ChangeInt(CurAlpha, 0, 255, 100 * CurSpeed);
		else
			ToolFc::ChangeInt(CurAlpha, 0, 255, -50 * CurSpeed);

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