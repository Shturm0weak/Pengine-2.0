#pragma once

#include "Core.h"

namespace Pengine
{

	class Viewport;

	class PENGINE_API ViewportManager
	{
	public:
		ViewportManager(const ViewportManager&) = delete;
		ViewportManager& operator=(const ViewportManager&) = delete;

		std::shared_ptr<Viewport> Create(const std::string& name, const glm::ivec2& size);

		std::shared_ptr<Viewport> GetViewport(const std::string& name) const;

		bool Destroy(const std::shared_ptr<Viewport>& viewport);

		std::unordered_map<std::string, std::shared_ptr<Viewport>> GetViewports() const { return m_Viewports; }

		void ShutDown();

		ViewportManager() = default;
		~ViewportManager();

	private:
		std::unordered_map<std::string, std::shared_ptr<Viewport>> m_Viewports;
	};

}
