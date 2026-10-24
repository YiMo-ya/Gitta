#include "UserData.h"
#include "lock/Lock.h"
#include <ShlObj.h>


namespace UserData
{
	NOXS; NOSTD;

	wstring UserName;
	wstring UserPath;

	/// <summary>
	/// 取本机 LocalAppData 目录（%LOCALAPPDATA%）。
	/// 直接从环境变量读取即可，避免依赖 COM 初始化，同时兼容重定向的漫游配置。
	/// </summary>
	/// <returns>目录绝对路径；读取失败时返回空串。</returns>
	static wstring GetLocalAppDataPath()
	{
		wchar_t buffer[MAX_PATH] = { 0 };
		DWORD len = GetEnvironmentVariableW(L"LOCALAPPDATA", buffer, MAX_PATH);
		if (len == 0 || len >= MAX_PATH) return wstring();
		return wstring(buffer, len);
	}

	/// <summary>
	/// 复位用户状态，保证任何失败路径都不会留下"半加载"的残留数据。
	/// </summary>
	static void Reset()
	{
		UserName.clear();
		UserPath.clear();
	}

	bool Load(const wstring& name)
	{
		//先复位，避免上一次的用户名在失败时仍被 IsUser 采信
		Reset();

		if (name.empty()) return false;

		wstring localAppData = GetLocalAppDataPath();
		if (localAppData.empty()) return false;

		//用用户名哈希作为目录名：不同用户名的存档互不覆盖，且不会把明文用户名暴露在磁盘路径上
		string hash = Lock::Hash::SHA256Hex(name);
		if (hash.empty()) return false;

		wstring basePath = localAppData + L"\\Gitta";
		wstring userPath = basePath + L"\\" + wstring(hash.begin(), hash.end());

		//首次登录时逐级补齐目录
		XFile::CreateDirectory(basePath);
		XFile::CreateDirectory(userPath);

		//目录没真正建出来说明磁盘或权限有问题，此时不认为加载成功
		if (GetFileAttributesW(userPath.c_str()) == INVALID_FILE_ATTRIBUTES) return false;
		if (!(GetFileAttributesW(userPath.c_str()) & FILE_ATTRIBUTE_DIRECTORY)) return false;

		UserName = name;
		UserPath = userPath;
		return true;
	}

	bool IsUserLogin()
	{
		//没有调用过 Load（或调用失败）时，两个成员均为空，直接判定为未登录
		if (UserName.empty() || UserPath.empty()) return false;

		//存档目录被外部删除后同样视为未登录，避免后续写入失败
		DWORD attr = GetFileAttributesW(UserPath.c_str());
		if (attr == INVALID_FILE_ATTRIBUTES) return false;
		return (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
	}

	bool HasUser()
	{
		wstring localAppData = GetLocalAppDataPath();
		if (localAppData.empty()) return false;

		wstring basePath = localAppData + L"\\Gitta";

		//枚举 Gitta 目录下的子项，只要出现一个目录就说明本机已存在用户存档
		WIN32_FIND_DATAW findData;
		HANDLE find = FindFirstFileW((basePath + L"\\*").c_str(), &findData);
		if (find == INVALID_HANDLE_VALUE) return false;

		bool found = false;
		do
		{
			//跳过 . 与 .. 这两个伪目录项
			if (wcscmp(findData.cFileName, L".") == 0 || wcscmp(findData.cFileName, L"..") == 0) continue;
			if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				found = true;
				break;
			}
		} while (FindNextFileW(find, &findData));

		FindClose(find);
		return found;
	}
}
