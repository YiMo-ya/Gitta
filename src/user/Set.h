#pragma once
#include "Xs/Xs.h"
#include "main/Lang.h"

namespace Setting
{
	//初始化用户数据
	void Init();

	void Read();

	//导出主题颜色
	extern Color MainColor;

	//导出背景颜色
	extern Color BackColor;

	//导出语言
	extern Language Lang;
}