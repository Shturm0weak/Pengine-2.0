#pragma once

#include "Core.h"

namespace Pengine
{

	class PENGINE_API ClayScriptManager
	{
	public:
		static ClayScriptManager& GetInstance();

		ClayScriptManager(const ClayScriptManager&) = delete;
		ClayScriptManager& operator=(const ClayScriptManager&) = delete;

		std::unordered_map<std::string, std::function<void(class Canvas* canvas, std::shared_ptr<class Entity>)>> scriptsByName;

	private:
		ClayScriptManager() = default;
		~ClayScriptManager() = default;
	};

}
