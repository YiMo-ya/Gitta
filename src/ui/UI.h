#pragma once
#include "Xs/Xs.h"

namespace UI
{
	class Rect
	{
		int BorderAlpha = 0;

		int x, y, w, h;

		bool IsInUI = false;
	public:

		bool EnableBorder;

		Color BackColor, BorderColor;

		//绘制
		void Draw(RenderTarget& target, int x, int y, int w, int h, int r);

		//是否在UI里面
		bool IsIn();

		//是否被点击
		bool IsClick();
	};

	/// <summary>
	/// 矩形文本输入框组件，支持鼠标点选、光标定位、UTF-8 文本编辑与 IME 输入。
	/// 使用方式：设置位置、尺寸与提示文本后调用对应的 Draw 重载，组件内部自行处理焦点与键盘事件。
	/// </summary>
	class InPutRectUI
	{
		/// <summary>光标闪烁计时器，累加到阈值后重置并刷新 IME 位置。</summary>
		int CurClock = 0;
		/// <summary>数字模式下编辑中的临时字符串。</summary>
		std::string numstr;
		/// <summary>光标横向像素位置的缓动动画，用于平滑移动光标。</summary>
		xs::EaseValue CurLocalX;
		/// <summary>光标当前的透明度，用于闪烁效果。</summary>
		int CurAlpha = 0;

		/// <summary>光标停在第几个 UTF-8 字符之后。</summary>
		size_t CaretPos = 0;
		/// <summary>光标相对输入框起点的像素位置，供 IME 定位使用。</summary>
		float  CaretPixelX = 0.f;

	public:
		/// <summary>输入框左上角坐标。</summary>
		int x = 0, y = 0;
		/// <summary>输入框宽高。</summary>
		int w = 0, h = 0;
		/// <summary>文本绘制原点（通常为左内边距处、垂直居中位置）。</summary>
		int fx = 0, fy = 0;
		/// <summary>绘制原点相对输入框的比例偏移，用于居中排版。</summary>
		float CenterX = 0.0, CenterY = 0.0;
		/// <summary>当前是否持有输入焦点。</summary>
		bool IsCin = false;
		/// <summary>光标动画速度倍率。</summary>
		int CurSpeed = 1;
		/// <summary>未输入内容时显示的提示文本。</summary>
		std::string tips;

		/// <summary>绘制在文本前面的固定前缀。</summary>
		std::string prefix;
		/// <summary>绘制在文本后面的固定后缀。</summary>
		std::string Suffix;
		/// <summary>数字模式下的取值范围与夹取开关。</summary>
		int nummin = 0, nummax = 0;
		bool EnableClampMin = false;
		bool EnableClampMax = false;

		/// <summary>提示文本颜色。</summary>
		Color TipColor = Color(150, 150, 150);
		/// <summary>正文文本颜色。</summary>
		Color TextColor = Color(255, 255, 255);
		/// <summary>光标颜色。</summary>
		Color CurColor = Color(255, 255, 255);

		/// <summary>
		/// 绘制数字输入框并处理输入逻辑。
		/// </summary>
		/// <param name="dest">目标渲染对象。</param>
		/// <param name="num">被编辑的数字，失焦或回车时写回。</param>
		void Draw(Rt& dest, int& num);

		/// <summary>
		/// 绘制文本输入框并处理输入逻辑。
		/// </summary>
		/// <param name="dest">目标渲染对象。</param>
		/// <param name="str">被编辑的 UTF-8 文本。</param>
		void Draw(Rt& dest, std::string& str);

	private:
		/// <summary>统计 UTF-8 字符串的字符数量。</summary>
		static size_t Utf8CharCount(const std::string& s);
		/// <summary>把「第 n 个字符」换算为字节偏移。</summary>
		static size_t Utf8OffsetByChars(const std::string& s, size_t charCount);
		/// <summary>按字符位置安全截取 UTF-8 子串。</summary>
		static std::string Utf8Substr(const std::string& s, size_t pos, size_t len);

		/// <summary>根据当前光标字符位置重新计算并触发光标像素动画。</summary>
		void SyncCaretPixel(const std::string& str);
		/// <summary>根据鼠标横坐标把光标吸附到最近的字符边界。</summary>
		void SetCaretFromMouse(int mouseX, const std::string& str);
		/// <summary>把数字编辑缓冲区提交到目标变量，并按需夹取范围。</summary>
		void ApplyNum(int& num);
	};
}
