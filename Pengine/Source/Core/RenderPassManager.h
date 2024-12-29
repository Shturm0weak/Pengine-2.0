#pragma once

#include "Core.h"
#include "LineRenderer.h"
#include "SSAORenderer.h"
#include "CSMRenderer.h"

#include "../Graphics/RenderPass.h"

namespace Pengine
{

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

		static void PrepareUniformsPerViewportBeforeDraw(const RenderPass::RenderCallbackInfo& renderInfo);

	private:
		using EntitiesByMesh = std::unordered_map<std::shared_ptr<class Mesh>, std::vector<entt::entity>>;
		using MeshesByMaterial = std::unordered_map<std::shared_ptr<class Material>, EntitiesByMesh>;
		using RenderableEntities = std::unordered_map<std::shared_ptr<class BaseMaterial>, MeshesByMaterial>;

		struct RenderableData : public CustomData
		{
			RenderableEntities renderableEntities;
			size_t renderableCount = 0;
		};

		struct InstanceData
		{
			glm::mat4 transform;
			glm::mat3 inverseTransform;
		};

		RenderPassManager();
		~RenderPassManager() = default;

		static std::vector<std::shared_ptr<class Buffer>> GetVertexBuffers(
			std::shared_ptr<class Pipeline> pipeline,
			std::shared_ptr<class Mesh> mesh);

		void CreateZPrePass();

		void CreateGBuffer();

		void CreateDeferred();

		void CreateDefaultReflection();

		void CreateAtmosphere();

		void CreateTransparent();

		void CreateFinal();

		void CreateSSAO();

		void CreateSSAOBlur();

		void CreateCSM();

		void CreateBloom();

		std::unordered_map<std::string, std::shared_ptr<RenderPass>> m_RenderPassesByType;
	};

}