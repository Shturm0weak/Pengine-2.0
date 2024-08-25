#pragma once

#include "Core.h"
#include "../Graphics/RenderPass.h"

namespace Pengine
{

	// TODO: move somewhere.
	const std::vector<std::string> renderPassesOrder =
	{
		Atmosphere,
		GBuffer,
		Deferred,
		Transparent
	};

	class PENGINE_API RenderPassManager
	{
	public:
		static RenderPassManager& GetInstance();

		RenderPassManager(const RenderPassManager&) = delete;
		RenderPassManager& operator=(const RenderPassManager&) = delete;

		std::shared_ptr<RenderPass> Create(const RenderPass::CreateInfo& createInfo);

		std::shared_ptr<RenderPass> GetRenderPass(const std::string& type) const;

		size_t GetRenderPassesCount() const { return m_RenderPassesByType.size(); }

		void ShutDown();

	private:

		struct InstanceData
		{
			glm::mat4 transform;
			glm::mat3 inverseTransform;
		};

		RenderPassManager();
		~RenderPassManager() = default;

		void CreateGBuffer();

		void CreateDeferred();

		void CreateDefaultReflection();

		void CreateAtmosphere();

		void CreateTransparent();

		std::unordered_map<std::string, std::shared_ptr<RenderPass>> m_RenderPassesByType;
	};

}