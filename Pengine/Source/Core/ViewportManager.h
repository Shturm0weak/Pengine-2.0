#pragma once

#include "Core.h"
#include "Viewport.h"

namespace Pengine
{

	class PENGINE_API ViewportManager
	{
	public:
		static ViewportManager& GetInstance();

		std::shared_ptr<Viewport> Create(const std::string& name, const glm::ivec2& size);

		std::shared_ptr<Viewport> GetViewport(const std::string& name);

		bool Destroy(std::shared_ptr<Viewport> viewport);

		std::unordered_map<std::string, std::shared_ptr<Viewport>> GetViewports() const { return m_Viewports; }

		void ShutDown();

	private:
		ViewportManager() = default;
		~ViewportManager();
		ViewportManager(const ViewportManager&) = delete;
		ViewportManager& operator=(const ViewportManager&) = delete;

		std::unordered_map<std::string, std::shared_ptr<Viewport>> m_Viewports;
	};

}