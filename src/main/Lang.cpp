#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

#include "Lang.h"

#include <fstream>
#include <sstream>
#include <codecvt>
#include <unordered_map>
#include <vector>
#include <string>

using WStringList = std::vector<std::wstring>;

static std::unordered_map<std::wstring, WStringList> g_I18NTable;
static bool g_I18NLoaded = false;

static std::wstring ReadUTF8File(const std::wstring& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return {};
    char bom[3];
    file.read(bom, 3);
    bool hasBOM = (file.gcount() == 3 && (unsigned char)bom[0] == 0xEF && (unsigned char)bom[1] == 0xBB && (unsigned char)bom[2] == 0xBF);
    if (!hasBOM) file.seekg(0);
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    return conv.from_bytes(content);
}

static void LoadAllLangFiles(const std::wstring& langDir) {
    const wchar_t* files[] = { L"zh.ini", L"en.ini", L"jp.ini", L"kr.ini", L"fr.ini", L"de.ini", L"ru.ini" };
    for (int i = 0; i < 7; i++) {
        std::wstring content = ReadUTF8File(langDir + L"/" + files[i]);
        if (content.empty()) continue;
        std::wstringstream ss(content);
        std::wstring line;
        while (std::getline(ss, line)) {
            if (line.empty() || line[0] == L'#') continue;
            size_t eq = line.find(L'=');
            if (eq == std::wstring::npos) continue;
            std::wstring key = line.substr(0, eq);
            std::wstring val = line.substr(eq + 1);
            while (!key.empty() && (key.back() == L' ' || key.back() == L'\t')) key.pop_back();
            while (!val.empty() && (val.front() == L' ' || val.front() == L'\t')) val.erase(0, 1);
            auto it = g_I18NTable.find(key);
            if (it != g_I18NTable.end()) {
                if (i >= (int)it->second.size()) it->second.resize(i + 1);
                it->second[i] = val;
            }
            else {
                WStringList list; list.resize(i + 1); list[i] = val;
                g_I18NTable[key] = list;
            }
        }
    }
    g_I18NLoaded = true;
}

namespace Translate {
    const std::wstring Translate(const std::wstring& chineseText, Language lang) {
        static const std::wstring empty;
        if (!g_I18NLoaded) LoadAllLangFiles(L"./Lang");
        auto it = g_I18NTable.find(chineseText);
        if (it == g_I18NTable.end()) return chineseText;
        int index = static_cast<int>(lang);
        if (index < 0 || index >= (int)it->second.size()) return chineseText;
        const std::wstring& result = it->second[index];
        if (result.empty()) return chineseText;
        return result;
    }
}