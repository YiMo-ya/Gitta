#pragma once

#include <string>

//语言翻译文件

enum Language
{
    ZH = 0, //中文
    EN = 1, //英文
    JA = 2, //日文
    KO = 3, //韩文
    FR = 4, //法语
    DE = 5, //德语
    RU = 6 //俄语
};

namespace Translate
{
    //翻译
    const std::wstring Translate(
        const std::wstring& chineseText,
        Language lang);
}