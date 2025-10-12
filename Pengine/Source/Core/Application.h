#pragma once

#include "Core.h"

namespace Pengine
{

	class PENGINE_API Application
	{
	public:

		virtual ~Application() = default;

		virtual void OnPreStart() {}

		virtual void OnStart() {}

		virtual void OnUpdate() {}

		virtual void OnImGuiUpdate() {}

		virtual void OnClose() {}
	};

}