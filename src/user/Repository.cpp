#include "Repository.h"

namespace Repository
{
	NOXS; NOSTD;

	bool FinishLoad;

	vector< RepositoryStruct > Repositorires;

	//加载用户仓库
	void LoadUserRepository(wstring UserPath)
	{
		//历遍仓库
		vector<Path> Repos = XFile::ListDirectories(UserPath);

		//历遍仓库中的分支
		for (int r = 0; r < Repos.size(); r++)
		{
			Repositorires.emplace_back();

			wstring RepositoryPath = UserPath + L"\\" + Repos[r].wstring();

			//读取信息
			string temp;
			ifstream RepositoryRead(RepositoryPath + L"\\Data.dat");
			RepositoryRead >> temp; Repositorires[r].Name = XString::Convert::utf8_to_wstring(temp);
			RepositoryRead >> temp; Repositorires[r].Discription = XString::Convert::utf8_to_wstring(temp);
			RepositoryRead.close();

			//历遍分支
			vector<Path> BranchPaths = XFile::ListDirectories(UserPath + L"\\" + Repos[r].wstring());
			for (int b = 0; b < BranchPaths.size(); b++)
			{
				Repositorires[r].Branches.emplace_back();

				wstring BranchPath = UserPath + L"\\" + Repos[r].wstring() + L"\\" + BranchPaths[b].wstring();

				//读取信息
				string temp;
				ifstream BranchRead(BranchPath + L"\\Data.dat");
				BranchRead >> temp; Repositorires[r].Branches[b].Name = XString::Convert::utf8_to_wstring(temp);
				BranchRead.close();

				//历遍版本
				vector<Path> Versions = XFile::ListDirectories(UserPath + L"\\" + Repos[r].wstring() + L"\\" + BranchPaths[b].wstring());
				for (int v = 0; v < Versions.size(); v++)
				{
					Repositorires[r].Branches[b].Versions.emplace_back();

					//拼接版本分支
					wstring VerPath = UserPath + L"\\" + Repos[r].wstring() + L"\\" + BranchPaths[b].wstring() + L"\\" + Versions[v].wstring();

					string temp;

					//读取信息
					ifstream VerRead(VerPath + L"\\Data.dat");
					VerRead >> temp; Repositorires[r].Branches[b].Versions[v].Name = XString::Convert::utf8_to_wstring(temp);
					VerRead >> temp; Repositorires[r].Branches[b].Versions[v].Discription = XString::Convert::utf8_to_wstring(temp);
					VerRead.close();

					//读取文件
					Repositorires[r].Branches[b].Versions[v].Files = XFile::ListDirectories(VerPath + L"\\Files");
				}

			}

		}

		FinishLoad = true;
	}

	//创建用户仓库（返回空串表示成功，非空串为错误描述）
	wstring CreateUserRepository(wstring UserHash, wstring RepositoryName, vector< wstring > BranchNames)
	{
		//仓库名称不能为空，否则无法定位目录
		if (RepositoryName.empty())
		{
			return L"仓库名称不能为空";
		}

		//仓库根目录：用户哈希目录下的仓库名目录
		wstring RepositoryPath = UserHash + L"\\" + RepositoryName;

		//同名仓库已存在时不重复创建，避免覆盖已有版本数据
		if (std::filesystem::exists(RepositoryPath))
		{
			return L"仓库已存在：" + RepositoryName;
		}

		try
		{
			std::filesystem::create_directories(RepositoryPath);

			//仓库信息文件：首行为名称，次行为描述（描述留空，由后续功能补充）
			{
				ofstream RepositoryWrite(RepositoryPath + L"\\Data.dat");
				if (!RepositoryWrite.is_open())
				{
					return L"仓库信息文件写入失败：" + RepositoryPath + L"\\Data.dat";
				}
				RepositoryWrite << XString::Convert::wstring_to_utf8(RepositoryName) << endl;
				RepositoryWrite << XString::Convert::wstring_to_utf8(L"") << endl;
				RepositoryWrite.close();
			}

			//按传入顺序创建多个分支
			for (int b = 0; b < BranchNames.size(); b++)
			{
				//空分支名会生成非法路径，直接跳过
				if (BranchNames[b].empty())
				{
					continue;
				}

				wstring BranchPath = RepositoryPath + L"\\" + BranchNames[b];
				std::filesystem::create_directories(BranchPath);

				//分支信息文件：首行为分支名称
				ofstream BranchWrite(BranchPath + L"\\Data.dat");
				if (!BranchWrite.is_open())
				{
					return L"分支信息文件写入失败：" + BranchPath + L"\\Data.dat";
				}
				BranchWrite << XString::Convert::wstring_to_utf8(BranchNames[b]) << endl;
				BranchWrite.close();
			}
		}
		catch (const std::filesystem::filesystem_error& e)
		{
			//磁盘权限或路径非法等文件系统异常，把系统给出的原因透传给调用方
			return L"文件系统操作失败：" + XString::Convert::utf8_to_wstring(e.what());
		}

		return L"";
	}
}