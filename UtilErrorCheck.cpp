#include "SharedPCH.h"

bool UtilErrorCheck::Id(DWORD& id)
{
	bool ret = false;
	if (id < 0) ret = true;
	if (ret)
	{
		LOGW(L"ERROR", dfLOG_LEVEL_ERROR,L"Wrong Id : %d",id);
		
	}
	return ret;
}
