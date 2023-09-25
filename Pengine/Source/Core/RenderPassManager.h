#pragma once

#include "Core.h"
#include "../Graphics/RenderPass.h"

namespace Pengine
{

	// TODO: move somewhere.
	const std::vector<std::string> renderPassesOrder =
	{
		GBuffer,
		Deferred,
	};

	class PENGINE_API RenderPassManager
	{
	public:
		static RenderPassManager& GetInstance();

		std::shared_ptr<RenderPass> Create(const RenderPass::CreateInfo& createInfo);

		std::shared_ptr<RenderPass> GetRenderPass(const std::string& type) const;

		size_t GetRenderPassesCount() const { return m_RenderPassesByType.size(); }

		void ShutDown();

	private:
		RenderPassManager();
		~RenderPassManager() = default;
		RenderPassManager(const RenderPassManager&) = delete;
		RenderPassManager& operator=(const RenderPassManager&) = delete;

		std::unordered_map<std::string, std::shared_ptr<RenderPass>> m_RenderPassesByType;

		void CreateGBuffer();

		void CreateDeferred();
	};

}