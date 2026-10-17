#pragma once
#include "Xs/Xs.h"

namespace Project
{
	namespace Struct
	{
		struct ProjectConfig
		{
			//项目路径
			std::wstring ProjectPath;
			//项目名称
			std::wstring ProjectName;
			//项目画布大小
			Vector2i ProjectSize;
			//项目帧率
			int ProjectFps;
		};
	}

	//项目配置
	extern Struct::ProjectConfig Config;
}