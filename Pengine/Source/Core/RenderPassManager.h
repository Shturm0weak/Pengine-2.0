#pragma once

#include "Core.h"
#include "LineRenderer.h"
#include "SSAORenderer.h"
#include "CSMRenderer.h"

#include "../Graphics/ComputePass.h"
#include "../Graphics/RenderPass.h"
#include "../Graphics/Pipeline.h"

namespace Pengine
{

	class PENGINE_API RenderPassManager
	{
	public:
		static RenderPassManager& GetInstance();

		RenderPassManager(const RenderPassManager&) = delete;
		RenderPassManager& operator=(const RenderPassManager&) = delete;

		std::shared_ptr<RenderPass> CreateRenderPass(const RenderPass::CreateInfo& createInfo);

		std::shared_ptr<ComputePass> CreateComputePass(const ComputePass::CreateInfo& createInfo);

		std::shared_ptr<Pass> GetPass(const std::string& name) const;

		std::shared_ptr<RenderPass> GetRenderPass(const std::string& name) const;

		std::shared_ptr<ComputePass> GetComputePass(const std::string& name) const;

		size_t GetPassCount() const { return m_PassesByName.size(); }

		void ShutDown();

		static std::vector<std::shared_ptr<class UniformWriter>> GetUniformWriters(
			std::shared_ptr<class Pipeline> pipeline,
			std::shared_ptr<class BaseMaterial> baseMaterial,
			std::shared_ptr<class Material> material,
			const RenderPass::RenderCallbackInfo& renderInfo);

		static void PrepareUniformsPerViewportBeforeDraw(const RenderPass::RenderCallbackInfo& renderInfo);

		std::shared_ptr<Texture> ScaleTexture(
			std::shared_ptr<Texture> sourceTexture,
			const glm::ivec2& dstSize);

	private:

		bool m_IsRayTracingEnabled = false;

		struct EntitiesByMesh
		{
			std::unordered_map<std::shared_ptr<class Mesh>, std::vector<entt::entity>> instanced;
			std::vector<std::pair<std::shared_ptr<class Mesh>, entt::entity>> single;
		};

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

		static void GetVertexBuffers(
			std::shared_ptr<class Pipeline> pipeline,
			std::shared_ptr<class Mesh> mesh,
			std::vector<std::shared_ptr<class Buffer>>& vertexBuffers,
			std::vector<size_t>& vertexBufferOffsets);

		void CreateRayTracing();

		void CreateZPrePass();

		void CreateGBuffer();

		void CreateDeferred();

		void CreateDefaultReflection();

		void CreateAtmosphere();

		void CreateTransparent();

		void CreateFinal();

		void CreateCSM();

		void CreateBloom();

		void CreateSSR();

		void CreateSSRBlur();

		void CreateSSAO();

		void CreateSSAOBlur();

		void CreateUI();

		static void FlushUniformWriters(const std::vector<std::shared_ptr<class UniformWriter>>& uniformWriters);

		static void WriteRenderViews(
			std::shared_ptr<class RenderView> cameraRenderView,
			std::shared_ptr<class RenderView> sceneRenderView,
			std::shared_ptr<class Pipeline> pipeline,
			std::shared_ptr<class UniformWriter> uniformWriter);

		static std::shared_ptr<class UniformWriter> GetOrCreateUniformWriter(
			std::shared_ptr<class RenderView> renderView,
			std::shared_ptr<class Pipeline> pipeline,
			Pipeline::DescriptorSetIndexType descriptorSetIndexType,
			const std::string& uniformWriterName,
			const std::string& uniformWriterIndexByName = {});

		static std::shared_ptr<class UniformWriter> GetOrCreateRendererUniformWriter(
			std::shared_ptr<class RenderView> renderView,
			std::shared_ptr<class Pipeline> pipeline,
			const std::string& uniformWriterName,
			const std::string& uniformWriterIndexByName = {});

		static std::shared_ptr<class Buffer> GetOrCreateRenderBuffer(
			std::shared_ptr<class RenderView> renderView,
			std::shared_ptr<class UniformWriter> uniformWriter,
			const std::string& bufferName,
			const std::string& setBufferName = {});

		static void UpdateSkeletalAnimator(
			class SkeletalAnimator* skeletalAnimator,
			std::shared_ptr<class BaseMaterial> baseMaterial,
			std::shared_ptr<class Pipeline> pipeline);

		std::unordered_map<std::string, std::shared_ptr<Pass>> m_PassesByName;
	};

}
