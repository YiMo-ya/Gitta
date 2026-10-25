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

	bool Load(const wstring& name,const wstring& password)
	{
		//先复位，避免上一次的用户名在失败时仍被 IsUser 采信
		Reset();

		if (name.empty()) return false;

		wstring localAppData = GetLocalAppDataPath();
		if (localAppData.empty()) return false;

		//用用户名哈希作为目录名：不同用户名的存档互不覆盖，且不会把明文用户名暴露在磁盘路径上
		string UserNameHash = Lock::Hash::SHA256Hex(name);
		if (UserNameHash.empty()) return false;

		wstring basePath = localAppData + L"\\Gitta";
		wstring userPath = basePath + L"\\" + XString::Convert::utf8_to_wstring(UserNameHash);

		//首次登录时逐级补齐目录
		XFile::CreateDirectory(basePath);
		XFile::CreateDirectory(userPath);

		//目录没真正建出来说明磁盘或权限有问题，此时不认为加载成功
		if (GetFileAttributesW(userPath.c_str()) == INVALID_FILE_ATTRIBUTES) return false;
		if (!(GetFileAttributesW(userPath.c_str()) & FILE_ATTRIBUTE_DIRECTORY)) return false;

		//检查用户的密码

		//计算本次哈希
		string PassHashNow = Lock::Hash::SHA256Hex(password);
		//读取用户配置的哈希
		string UserPassHash;
		ifstream UserPassword(userPath + L"\\Data.dat");
		UserPassword >> UserPassHash;
		UserPassword.close();

		if (UserPassHash != PassHashNow) return false;

		//保存本次用户的登录凭据
		if (!XFile::Exists(L"User"))
		{
			XFile::CreateDirectory(L"User");
		}

		ofstream LoginTemp(ExePath + L"\\User\\Data.dat");
		//凭据写入三行：用户名哈希（拼存档目录）、密码哈希（校验）、明文用户名（自动登录后回填 UserName）
		LoginTemp << UserNameHash << endl << UserPassHash << endl << XString::Convert::wstring_to_utf8(name);
		LoginTemp.close();

		UserName = name;
		UserPath = userPath;
		return true;
	}

	/// <summary>
	/// 读取本地「上次登录凭据」文件（ExePath\User\Data.dat）。
	/// 文件格式为三行：第一行用户名 SHA-256，第二行密码 SHA-256，第三行明文用户名（UTF-8）。
	/// </summary>
	/// <param name="userNameHash">输出：上次登录用户名的哈希。</param>
	/// <param name="passHash">输出：上次登录密码的哈希。</param>
	/// <param name="userName">输出：上次登录的明文用户名（UTF-16）。</param>
	/// <returns>凭据完整且格式正确返回 true，否则返回 false。</returns>
	static bool ReadLoginCredential(string& userNameHash, string& passHash, wstring& userName)
	{
		userNameHash.clear();
		passHash.clear();
		userName.clear();

		wstring credPath = ExePath + L"\\User\\Data.dat";

		//文件不存在说明从没成功登录过，这是首次启动的正常情况，不算错误
		ifstream LoginFile(credPath);
		if (!LoginFile.is_open()) return false;

		//逐行读取：任一行缺失都视为凭据不完整，交由调用方回落到密码登录
		string nameLine, passLine, rawNameLine;
		getline(LoginFile, nameLine);
		getline(LoginFile, passLine);
		//第三行是明文用户名（UTF-8），必须一并读出，否则 userName 恒为空会让自动登录必然失败
		getline(LoginFile, rawNameLine);
		LoginFile.close();

		//去掉 Windows 换行残留的 \r，以及首尾空白
		auto Trim = [](string& s)
		{
			while (!s.empty() && (s.back() == '\r' || s.back() == '\n' || s.back() == ' ' || s.back() == '\t')) s.pop_back();
			size_t pos = 0;
			while (pos < s.size() && (s[pos] == ' ' || s[pos] == '\t')) pos++;
			if (pos > 0) s.erase(0, pos);
		};
		Trim(nameLine);
		Trim(passLine);
		Trim(rawNameLine);

		//哈希固定为 64 位十六进制，长度不符说明文件被篡改或写坏了
		if (nameLine.size() != 64 || passLine.size() != 64) return false;

		userNameHash = nameLine;
		passHash = passLine;
		//明文用户名为空说明是旧版两行格式的凭据，此时无法回填 UserName，按无效处理
		if (rawNameLine.empty()) return false;
		userName = XString::Convert::utf8_to_wstring(rawNameLine);
		if (userName.empty()) return false;
		//末行可能没有换行结尾，这里补一次收尾 Trim 已在上面完成；
		//明文用户名写成哈希前，必须与第一行的哈希双向校验，防止文件被改写后冒用他人存档
		if (Lock::Hash::SHA256Hex(userName) != userNameHash) return false;
		return true;
	}

	bool AutoLogin()
	{
		//先复位，避免上次残留的用户名被 IsUserLogin 误采信
		Reset();

		string userNameHash, passHash;
		wstring userName;
		if (!ReadLoginCredential(userNameHash, passHash, userName)) return false;

		wstring localAppData = GetLocalAppDataPath();
		if (localAppData.empty()) return false;

		//凭据里存的是用户名哈希，目录名正好也是该哈希，可直接拼出存档目录。
		//注意：必须与 Load 用完全相同的展开方式，否则同一用户会定位到不存在的目录而导致自动登录必然失败
		wstring userPath = localAppData + L"\\Gitta\\" + wstring(userNameHash.begin(), userNameHash.end());

		DWORD attr = GetFileAttributesW(userPath.c_str());
		if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY)) return false;

		//用存档目录里的密码哈希与凭据比对，等价于一次"无密码登录"；
		//这样即使用户在别处改过密码，旧凭据也会自动失效，不会绕过校验
		string storedHash;
		ifstream UserPassword(userPath + L"\\Data.dat");
		if (!UserPassword.is_open()) return false;
		UserPassword >> storedHash;
		UserPassword.close();

		if (storedHash.empty() || storedHash != passHash) return false;

		//二次校验：凭据中的明文用户名必须与哈希一致，防止凭据被篡改后冒用他人存档
		if (Lock::Hash::SHA256Hex(userName) != userNameHash) return false;

		//凭据有效：恢复登录状态，此时 UserName 与 UserPath 与一次真实 Load 完全等价
		UserName = userName;
		UserPath = userPath;
		return true;
	}

	bool InitUserData(const wstring& name, const wstring& passWord)
	{
		//先复位，避免上一次的用户名在失败时仍被 IsUserLogin 采信
		Reset();

		if (name.empty() || passWord.empty()) return false;

		wstring localAppData = GetLocalAppDataPath();
		if (localAppData.empty()) return false;

		//计算用户名hash
		string UserNameHash = Lock::Hash::SHA256Hex(name);
		if (UserNameHash.empty()) return false;

		wstring basePath = localAppData + L"\\Gitta";
		//与 Load 保持一致：把 64 个 hex 字符按字节直接展开，避免同一用户名落到不同目录
		wstring userPath = basePath + L"\\" + wstring(UserNameHash.begin(), UserNameHash.end());

		//首次登录时逐级补齐目录
		XFile::CreateDirectory(basePath);
		XFile::CreateDirectory(userPath);

		//目录没真正建出来说明磁盘或权限有问题，此时不认为初始化成功
		DWORD attr = GetFileAttributesW(userPath.c_str());
		if (attr == INVALID_FILE_ATTRIBUTES) return false;
		if (!(attr & FILE_ATTRIBUTE_DIRECTORY)) return false;

		//存入
		string UserPassHash = Lock::Hash::SHA256Hex(passWord);
		ofstream UserPassWordHash(userPath + L"\\Data.dat");
		UserPassWordHash << UserPassHash << endl; //存入密码hash
		UserPassWordHash.close();

		//保存本次用户的登录凭据
		if (!XFile::Exists(L"User"))
		{
			XFile::CreateDirectory(L"User");
		}

		ofstream LoginTemp(ExePath + L"\\User\\Data.dat");
		//凭据写入三行：用户名哈希（拼存档目录）、密码哈希（校验）、明文用户名（自动登录后回填 UserName）
		LoginTemp << UserNameHash << endl << UserPassHash << endl << XString::Convert::wstring_to_utf8(name);
		LoginTemp.close();

		// 显式限定命名空间成员，避免与同名参数混淆导致自我赋值
		UserData::UserName = name;
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

	bool IsUser(wstring UserName)
	{
		//只做存在性检查，不依赖当前登录状态，因此不读也不改全局 UserName/UserPath
		if (UserName.empty()) return false;

		wstring localAppData = GetLocalAppDataPath();
		if (localAppData.empty()) return false;

		//沿用与 Load/InitUserData 完全一致的目录映射规则，保证查询目标与注册落盘位置一致
		string hash = Lock::Hash::SHA256Hex(UserName);
		if (hash.empty()) return false;

		//同上：目录名展开方式必须与 Load/InitUserData 完全相同
		wstring userPath = localAppData + L"\\Gitta\\" + wstring(hash.begin(), hash.end());

		//注册时只建目录还不算有效账号，必须已写入密码哈希文件才算真正注册过
		wstring dataPath = userPath + L"\\Data.dat";

		//用文件存在性判断，避免打开文件造成句柄占用；INVALID_FILE_ATTRIBUTES 即不存在
		DWORD attr = GetFileAttributesW(dataPath.c_str());
		if (attr == INVALID_FILE_ATTRIBUTES) return false;

		//同名目录可能是外部误建，排除掉目录同名的异常情况
		return (attr & FILE_ATTRIBUTE_DIRECTORY) == 0;
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