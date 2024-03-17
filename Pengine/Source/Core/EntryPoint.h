#pragma once

#include "Core.h"
#include "Application.h"

namespace Pengine
{

	class PENGINE_API EntryPoint
	{
	public:
		explicit EntryPoint(Application* application);

		void Run() const;

	private:
		Application* m_Application = nullptr;
	};

}