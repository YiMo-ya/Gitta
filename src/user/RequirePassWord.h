#pragma once
#include "src/Shared.h"

namespace RequirePassWord
{
	NOXS; NOSTD;

	void RequirePassWord();

	bool IsPassWordFinish();

	void Enter(RenWin& window);
}