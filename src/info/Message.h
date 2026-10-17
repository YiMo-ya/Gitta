#pragma once
#include "Xs/Xs.h"

enum IconType
{
	ICOTYPE_DEBUG = 0,
	ICOTYPE_INFO = 1,
	ICOTYPE_QUESTION = 2,
	ICOTYPE_ERROR = 3,
	ICOTYPE_SUCCESS = 4,
	ICOTYPE_WARNING = 5
};

namespace Message
{
	//0无声，1默认声音，2警告声音，3错误声音
	int ShowMessage(std::wstring text, std::wstring title = L"",
		IconType ico = ICOTYPE_INFO,
		std::vector<std::wstring> button = { L"确定" },
		int audio = 1, std::wstring WindowTitle = L"NULL");
}