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
	bool Load(const wstring& name,const wstring& password);

	/// <summary>
	/// 初始化指定用户的数据（带密码校验）。
	/// 在 Load 完成目录映射的基础上，额外用密码参与目录定位，
	/// 使得同一用户名配合不同密码会落到不同存档目录，避免仅凭用户名即可访问他人存档。
	/// </summary>
	/// <param name="UserName">登录用户名，不能为空。</param>
	/// <param name="PassWord">登录密码，不能为空。</param>
	/// <returns>初始化成功返回 true；用户名或密码为空、或目录创建失败返回 false。</returns>
	bool InitUserData(const wstring& UserName, const wstring& PassWord);

	/// <summary>
	/// 判断当前会话是否已登录有效用户。
	/// 只有 Load 成功且存档目录仍然存在时才返回 true；这是业务逻辑判断"当前是谁"的唯一依据。
	/// </summary>
	/// <returns>已登录且存档目录可用返回 true，否则返回 false。</returns>
	bool IsUserLogin();

	/// <summary>
	/// 判断指定用户名在本机是否已注册（存在用户存档）。
	/// 按与 Load/InitUserData 相同的哈希规则定位存档目录，并检查其中的密码哈希文件 Data.dat 是否存在；
	/// 不依赖当前登录状态，也不会加载或切换用户数据、不校验密码。
	/// </summary>
	/// <param name="UserName">待检查的用户名，不能为空；区分大小写。</param>
	/// <returns>该用户名已注册返回 true；用户名为空、LocalAppData 不可用或存档文件不存在返回 false。</returns>
	bool IsUser(wstring UserName);

	/// <summary>
	/// 判断本机磁盘上是否已存在任一用户的存档（不要求当前已登录）。
	/// 用于决定是否需要引导用户去注册/登录，而非判断当前会话身份。
	/// </summary>
	/// <returns>LocalAppData\Gitta 下存在至少一个用户存档文件夹返回 true，否则返回 false。</returns>
	bool HasUser();

	/// <summary>
	/// 自动登录，仅限于本地具有上次的凭据
	/// </summary>
	/// <returns></returns>
	bool AutoLogin();
}
