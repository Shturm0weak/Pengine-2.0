#include "ClayScriptManager.h"

using namespace Pengine;

ClayScriptManager& ClayScriptManager::GetInstance()
{
	static ClayScriptManager clayScriptManager;
	return clayScriptManager;
}
