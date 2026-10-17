#pragma once
#include "Xs/Xs.h"

namespace BottomBar
{
	extern int h;

	//绘制
	void Draw(RenWin& window);

	/// <summary>
	/// 设置显示文本
	/// </summary>
	/// <param name="text">要显示的文本</param>
	void SetText(const std::wstring& text);

	/// <summary>
	/// 设置显示方式
	/// </summary>
	/// <param name="type">normal/process/loading/error/success/stop</param>
	void SetShowType(const std::wstring& type);
}