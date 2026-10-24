#pragma once
#include "src/Shared.h"

namespace UserData
{
	NOXS;NOSTD;

	/// <summary>
	/// 当前已加载的用户名。
	/// 为空表示尚未调用 Load，此时所有用户数据接口都不生效。
	/// </summary>
	extern wstring UserName;

	/// <summary>
	/// 当前用户数据的存储目录。
	/// 形如 %LOCALAPPDATA%\Gitta\&lt;用户名 SHA-256 十六进制&gt;，Load 成功后才会被赋值。
	/// </summary>
	extern wstring UserPath;

	/// <summary>
	/// 加载指定用户的数据。
	/// 用户名按 SHA-256 哈希后映射到 LocalAppData\Gitta 下的独立子目录，
	/// 因此本机多个用户名的存档互不干扰；目录或存档文件不存在时会自动创建。
	/// </summary>
	/// <param name="name">登录用户名，不能为空。</param>
	/// <returns>加载成功返回 true；用户名为空或目录创建失败返回 false。</returns>
	bool Load(const wstring& name);

	/// <summary>
	/// 判断当前会话是否已登录有效用户。
	/// 只有 Load 成功且存档目录仍然存在时才返回 true；这是业务逻辑判断"当前是谁"的唯一依据。
	/// </summary>
	/// <returns>已登录且存档目录可用返回 true，否则返回 false。</returns>
	bool IsUserLogin();

	/// <summary>
	/// 判断本机磁盘上是否已存在任一用户的存档（不要求当前已登录）。
	/// 用于决定是否需要引导用户去注册/登录，而非判断当前会话身份。
	/// </summary>
	/// <returns>LocalAppData\Gitta 下存在至少一个用户存档文件夹返回 true，否则返回 false。</returns>
	bool HasUser();
}
