#pragma once

#include "Core.h"
#include "Application.h"

namespace Pengine
{

	class PENGINE_API EntryPoint
	{
	public:
		EntryPoint(Application* application);

		void Run();

	private:
		Application* m_Application = nullptr;
	};

}