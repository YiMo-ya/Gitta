#pragma once
#include "src/Shared.h"

//版本
struct VersionStruct
{
	//版本名称
	std::wstring Name;
	//版本描述
	std::wstring Discription;
	//版本文件列表
	std::vector<std::filesystem::path> Files;
};

//分支
struct BranchStruct
{
	//分支名称
	std::wstring Name;

	std::vector< VersionStruct> Versions;
};

//仓库
struct RepositoryStruct
{
	//版本名称
	std::wstring Name;

	//版本描述
	std::wstring Discription;

	std::vector< BranchStruct > Branches;
};

namespace Repository
{
	NOSTD;

	extern bool FinishLoad;

	extern vector< RepositoryStruct > Repositorires;

	/// <summary>
	/// 加载用户的仓库
	/// </summary>
	/// <param name="UserHash">用户NameHash</param>
	void LoadUserRepository(wstring UserHash);

	/// <summary>
	/// 创建用户的仓库
	/// </summary>
	/// <param name="UserHash">用户NameHash</param>
	/// <param name="RepositoryName">仓库名称</param>
	/// <param name="BranchNames">分支名称列表，允许一次创建多个分支</param>
	/// <returns>返回空字符串表示创建成功；返回非空字符串为失败原因描述（如名称为空、仓库已存在、文件写入失败、文件系统异常）</returns>
	wstring CreateUserRepository(wstring UserHash, wstring RepositoryName, vector< wstring > BranchNames);

	
	
}