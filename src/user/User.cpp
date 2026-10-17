#include "User.h"
#include "Shared.h"
#include "main/Lang.h"

NOXS; NOSTD;

namespace UserData
{
	//主题颜色
	Color MainColor = Color(210,180,255);

	//背景颜色
	Color BackColor;
	//语言
	Language Lang = ZH;

	bool CanEnableDWM = true;

	void Read()
	{
		//拼接用户数据文件位置
		wstring UserDataPath = ExePath + L"\\User\\UserData.ini";

		//读取用户数据
		ifstream UserData(UserDataPath);
		if (UserData.is_open())
		{
			//解析器

			while (1)
			{
				string Data;

				UserData >> Data;
				if (Data.empty()) break;

				//主题
				if (Data == "<Theme>")
				{
					UserData >> Data;
					if (Data == "Dark") BackColor = Color(30, 30, 30);
					else
					{
						BackColor = Color(240, 240, 240);
						MainColor = Color(180, 0, 255);
					}
				}
				//语言
				if (Data == "<Lang>")
				{
					UserData >> Data;

					if (Data == "ZH") Lang = ZH;
					else if (Data == "EN") Lang = EN;
					else  if (Data == "JA") Lang = JA;
					else if (Data == "KO") Lang = KO;
					else if (Data == "FR") Lang = FR;
					else if (Data == "DE") Lang = DE;
					else if (Data == "RU") Lang = RU;
					else Lang = ZH;
				}
				//DWM
				if (Data == "<DWM>")
				{
					UserData >> Data;

					if (Data == "Enable") CanEnableDWM = true;
					else CanEnableDWM = false;
				}
			}

			UserData.close();
		}
		else
		{
			UserData.close();

			//创建目录
			XFile::CreateDirectory(ExePath + L"\\User");

			//初始化用户数据
			Init();
		}
	}

	void Init()
	{
		//拼接用户数据文件位置
		wstring UserDataPath = ExePath + L"\\User\\UserData.ini";

		ofstream InitUserData(UserDataPath);
		if (InitUserData.is_open())
		{
			InitUserData << "<Theme> Black" << endl;
			InitUserData << "<Lang> ZH" << endl;
			InitUserData << "<DWM> Enable" << endl;
		}
		else
		{
			MessageBoxW(NULL, Translate::Translate(L"在初始化用户数据时发生致命错误",Lang).c_str(), L"User Data Init Error", MB_OK + MB_SYSTEMMODAL + 16);
		}
	}
}