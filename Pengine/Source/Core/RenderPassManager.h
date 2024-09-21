#pragma once

#include "Core.h"
#include "LineRenderer.h"
#include "SSAORenderer.h"
#include "CSMRenderer.h"

#include "../Graphics/RenderPass.h"

namespace Pengine
{

	// TODO: move somewhere.
	const std::vector<std::string> renderPassesOrder =
	{
		Atmosphere,
		GBuffer,
		CSM,
		SSAO,
		SSAOBlur,
		Deferred,
		Transparent,
		Final,
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

		static std::vector<std::shared_ptr<class UniformWriter>> GetUniformWriters(
			std::shared_ptr<class Pipeline> pipeline,
			std::shared_ptr<class BaseMaterial> baseMaterial,
			std::shared_ptr<class Material> material,
			const RenderPass::RenderCallbackInfo& renderInfo);

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

		void CreateFinal();

		void CreateSSAO();

		void CreateSSAOBlur();

		void CreateCSM();

		std::unordered_map<std::string, std::shared_ptr<RenderPass>> m_RenderPassesByType;

		LineRenderer m_LineRenderer;
		SSAORenderer m_SSAORenderer;
		std::map<std::wstring, CSMRenderer> m_CSMRenderersByCSMSetting;
	};

}