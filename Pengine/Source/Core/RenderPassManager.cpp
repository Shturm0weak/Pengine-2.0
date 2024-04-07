#include "RenderPassManager.h"

#include "Logger.h"
#include "MaterialManager.h"
#include "MeshManager.h"
#include "SceneManager.h"

#include "../Components/Camera.h"
#include "../Components/PointLight.h"
#include "../Components/Renderer3D.h"
#include "../Components/Transform.h"
#include "../Graphics/Renderer.h"

using namespace Pengine;

RenderPassManager& RenderPassManager::GetInstance()
{
	static RenderPassManager renderPassManager;
	return renderPassManager;
}

std::shared_ptr<RenderPass> RenderPassManager::Create(const RenderPass::CreateInfo& createInfo)
{
	std::shared_ptr<RenderPass> renderPass = RenderPass::Create(createInfo);
	m_RenderPassesByType.emplace(createInfo.type, renderPass);

	return renderPass;
}

std::shared_ptr<RenderPass> RenderPassManager::GetRenderPass(const std::string& type) const
{
	if (const auto renderPassByName = m_RenderPassesByType.find(type);
		renderPassByName != m_RenderPassesByType.end())
	{
		return renderPassByName->second;
	}

	FATAL_ERROR(type + " id of render pass doesn't exist!");

	return nullptr;
}

void RenderPassManager::ShutDown()
{
	m_RenderPassesByType.clear();
}

RenderPassManager::RenderPassManager()
{
	CreateDefaultReflection();
	CreateGBuffer();
	CreateDeferred();
}

void RenderPassManager::CreateGBuffer()
{
	RenderPass::ClearDepth clearDepth{};
	clearDepth.clearDepth = 1.0f;
	clearDepth.clearStencil = 0;

	glm::vec4 clearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
	glm::vec4 clearNormal = { 0.0f, 0.0f, 0.0f, 0.0f };
	glm::vec4 clearPosition = { 0.0f, 0.0f, 0.0f, 0.0f };

	RenderPass::AttachmentDescription color{};
	color.format = Format::B8G8R8A8_SRGB;
	color.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;

	RenderPass::AttachmentDescription normal{};
	normal.format = Format::R16G16B16A16_SFLOAT;
	normal.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;

	RenderPass::AttachmentDescription position{};
	position.format = Format::R16G16B16A16_SFLOAT;
	position.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;

	RenderPass::AttachmentDescription depth{};
	depth.format = Format::D32_SFLOAT;
	depth.layout = Texture::Layout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	RenderPass::CreateInfo createInfo{};
	createInfo.type = GBuffer;
	createInfo.clearColors = { clearColor, clearNormal, clearPosition };
	createInfo.clearDepths = { clearDepth };
	createInfo.attachmentDescriptions = { color, normal, position, depth };

	struct InstanceData
	{
		glm::mat4 transform;
		glm::mat3 inverseTransform;
	};

	createInfo.buffersByName["InstanceBuffer"] = nullptr;

	const std::string globalBufferName = "GlobalBuffer";
	createInfo.createCallback = [globalBufferName](RenderPass& renderPass)
	{
		const std::shared_ptr<BaseMaterial> baseMaterial = MaterialManager::GetInstance().LoadBaseMaterial("Materials/DefaultReflection.basemat");
		const std::shared_ptr<Pipeline> pipeline = baseMaterial->GetPipeline(DefaultReflection);

		std::optional<uint32_t> descriptorSet = pipeline->GetDescriptorSetIndexByType(Pipeline::DescriptorSetIndexType::RENDERPASS);
		const std::shared_ptr<UniformLayout> uniformLayout = pipeline->GetUniformLayout(*descriptorSet);
		const std::shared_ptr<UniformWriter> uniformWriter = UniformWriter::Create(uniformLayout);

		const std::shared_ptr<Buffer> buffer = Buffer::Create(
			uniformLayout->GetBindingByName(globalBufferName)->buffer->size,
			1,
			Buffer::Usage::UNIFORM_BUFFER,
			Buffer::MemoryType::CPU);

		renderPass.SetBuffer(globalBufferName, buffer);
		renderPass.SetUniformWriter(uniformWriter);
		renderPass.GetUniformWriter()->WriteBuffer(globalBufferName, buffer);
	};

	createInfo.renderCallback = [globalBufferName](const RenderPass::RenderCallbackInfo& renderInfo)
	{
		const std::shared_ptr<BaseMaterial> reflectionBaseMaterial = MaterialManager::GetInstance().GetBaseMaterial("Materials/DefaultReflection.basemat");
		WriterBufferHelper::WriteToBuffer(
			reflectionBaseMaterial.get(),
			renderInfo.submitInfo.renderPass->GetBuffer(globalBufferName),
			globalBufferName,
			"camera.viewProjectionMat4",
			renderInfo.camera->GetComponent<Camera>().GetViewProjectionMat4());

		using EntitiesByMesh = std::unordered_map<std::shared_ptr<Mesh>, std::vector<entt::entity>>;
		using MeshesByMaterial = std::unordered_map<std::shared_ptr<Material>, EntitiesByMesh>;
		using MaterialByBaseMaterial = std::unordered_map<std::shared_ptr<BaseMaterial>, MeshesByMaterial>;

		MaterialByBaseMaterial materialMeshGameObjects;

		size_t renderableCount = 0;
		const std::shared_ptr<Scene> scene = renderInfo.scene;
		entt::registry& registry = scene->GetRegistry();
		const auto r3dView = registry.view<Renderer3D>();
		for (const entt::entity& entity : r3dView)
		{
			Renderer3D& r3d = registry.get<Renderer3D>(entity);

			if (!r3d.mesh || !r3d.material)
			{
				continue;
			}

			materialMeshGameObjects[r3d.material->GetBaseMaterial()][r3d.material][r3d.mesh].emplace_back(entity);

			renderableCount++;
		}

		std::shared_ptr<Buffer> instanceBuffer = renderInfo.submitInfo.renderPass->GetBuffer("InstanceBuffer");
		if ((renderableCount != 0 && !instanceBuffer) || (instanceBuffer && renderableCount != 0 && instanceBuffer->GetInstanceCount() != renderableCount))
		{
			instanceBuffer = Buffer::Create(
				sizeof(InstanceData),
				renderableCount,
				Buffer::Usage::VERTEX_BUFFER,
				Buffer::MemoryType::CPU);

			renderInfo.submitInfo.renderPass->SetBuffer("InstanceBuffer", instanceBuffer);
		}

		std::vector<InstanceData> instanceDatas;

		// Render all base materials -> materials -> meshes | put gameobjects into the instance buffer.
		for (const auto& [baseMaterial, meshesByMaterial] : materialMeshGameObjects)
		{
			const std::shared_ptr<Pipeline> pipeline = baseMaterial->GetPipeline(renderInfo.submitInfo.renderPass->GetType());
			if (!pipeline)
			{
				continue;
			}

			for (const auto& [material, gameObjectsByMeshes] : meshesByMaterial)
			{
				for (const auto& [mesh, entities] : gameObjectsByMeshes)
				{
					const size_t instanceDataOffset = instanceDatas.size();

					for (const entt::entity& entity : entities)
					{
						InstanceData data{};
						Transform& transform = registry.get<Transform>(entity);
						data.transform = transform.GetTransform();
						data.inverseTransform = glm::transpose(transform.GetInverseTransform());
						instanceDatas.emplace_back(data);
					}

					std::vector<std::shared_ptr<UniformWriter>> uniformWriters;
					if (pipeline->GetDescriptorSetIndexByType(Pipeline::DescriptorSetIndexType::RENDERPASS))
					{
						uniformWriters.emplace_back(renderInfo.submitInfo.renderPass->GetUniformWriter());
					}
					if (pipeline->GetDescriptorSetIndexByType(Pipeline::DescriptorSetIndexType::BASE_MATERIAL))
					{
						const std::shared_ptr<UniformWriter> baseMaterialUniformWriter = baseMaterial->GetUniformWriter(
							renderInfo.submitInfo.renderPass->GetType());
						uniformWriters.emplace_back(baseMaterialUniformWriter);
					}
					if (pipeline->GetDescriptorSetIndexByType(Pipeline::DescriptorSetIndexType::MATERIAL))
					{
						uniformWriters.emplace_back(material->GetUniformWriter(renderInfo.submitInfo.renderPass->GetType()));
					}

					for (const auto& uniformWriter : uniformWriters)
					{
						uniformWriter->Flush();

						for (const auto& [location, buffer] : uniformWriter->GetBuffersByLocation())
						{
							buffer->Flush();
						}
					}

					renderInfo.renderer->Render(
						mesh,
						pipeline,
						instanceBuffer,
						instanceDataOffset * instanceBuffer->GetInstanceSize(),
						entities.size(),
						uniformWriters,
						renderInfo.submitInfo);
				}
			}
		}

		// Because these are all just commands and will be rendered later we can write the instance buffer
		// just once when all instance data is collected.
		if (instanceBuffer && !instanceDatas.empty())
		{
			instanceBuffer->WriteToBuffer(instanceDatas.data(), instanceDatas.size() * sizeof(InstanceData));
		}
	};

	Create(createInfo);
}

void RenderPassManager::CreateDeferred()
{
	RenderPass::ClearDepth clearDepth{};
	clearDepth.clearDepth = 1.0f;
	clearDepth.clearStencil = 0;

	glm::vec4 clearColor = { 1.0f, 1.0f, 1.0f, 1.0f };

	RenderPass::AttachmentDescription color{};
	color.format = Format::R8G8B8A8_SRGB;
	color.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;

	RenderPass::CreateInfo createInfo{};
	createInfo.type = Deferred;
	createInfo.clearColors = { clearColor };
	createInfo.clearDepths = { clearDepth };
	createInfo.attachmentDescriptions = { color };

	const std::shared_ptr<Mesh> planeMesh = nullptr;

	createInfo.renderCallback = [](const RenderPass::RenderCallbackInfo& renderInfo)
	{
		const std::shared_ptr<Mesh> plane = MeshManager::GetInstance().LoadMesh("Meshes/Plane.mesh");
		if (!plane)
		{
			return;
		}

		const std::shared_ptr<BaseMaterial> baseMaterial = MaterialManager::GetInstance().LoadBaseMaterial("Materials/Deferred.basemat");
		const std::shared_ptr<Pipeline> pipeline = baseMaterial->GetPipeline(renderInfo.submitInfo.renderPass->GetType());
		if (!pipeline)
		{
			return;
		}

		const std::shared_ptr<UniformWriter> baseMaterialUniformWriter = baseMaterial->GetUniformWriter(
			renderInfo.submitInfo.renderPass->GetType());

		const std::shared_ptr<FrameBuffer> frameBuffer = renderInfo.renderer->GetRenderPassFrameBuffer(GBuffer);

		baseMaterialUniformWriter->WriteTexture("albedoTexture", frameBuffer->GetAttachment(0));
		baseMaterialUniformWriter->WriteTexture("normalTexture", frameBuffer->GetAttachment(1));
		baseMaterialUniformWriter->WriteTexture("positionTexture", frameBuffer->GetAttachment(2));

		auto pointLightView = renderInfo.scene->GetRegistry().view<PointLight>();
		uint32_t lightIndex = 0;
		for (const entt::entity& entity : pointLightView)
		{
			if (lightIndex == 32)
			{
				break;
			}

			PointLight& pl = renderInfo.scene->GetRegistry().get<PointLight>(entity);
			const Transform& transform = renderInfo.scene->GetRegistry().get<Transform>(entity);

			const std::string valueNamePrefix = "pointLights[" + std::to_string(lightIndex) + "]";

			baseMaterial->WriteToBuffer("Lights", valueNamePrefix + ".color", pl.color);
			glm::vec3 lightPosition = transform.GetPosition();
			baseMaterial->WriteToBuffer("Lights", valueNamePrefix + ".position", lightPosition);
			baseMaterial->WriteToBuffer("Lights", valueNamePrefix + ".linear", pl.linear);
			baseMaterial->WriteToBuffer("Lights", valueNamePrefix + ".quadratic", pl.quadratic);
			baseMaterial->WriteToBuffer("Lights", valueNamePrefix + ".constant", pl.constant);

			lightIndex++;
		}

		int pointLightsCount = pointLightView.size();
		baseMaterial->WriteToBuffer("Lights", "pointLightsCount", pointLightsCount);

		std::vector<std::shared_ptr<UniformWriter>> uniformWriters;
		if (pipeline->GetDescriptorSetIndexByType(Pipeline::DescriptorSetIndexType::RENDERPASS))
		{
			uniformWriters.emplace_back(renderInfo.submitInfo.renderPass->GetUniformWriter());
		}
		if (pipeline->GetDescriptorSetIndexByType(Pipeline::DescriptorSetIndexType::BASE_MATERIAL))
		{
			uniformWriters.emplace_back(baseMaterialUniformWriter);
		}

		for (const auto& uniformWriter : uniformWriters)
		{
			uniformWriter->Flush();

			for (const auto& [location, buffer] : uniformWriter->GetBuffersByLocation())
			{
				buffer->Flush();
			}
		}

		renderInfo.renderer->Render(
			plane,
			pipeline,
			nullptr,
			0,
			1,
			uniformWriters,
			renderInfo.submitInfo);
	};

	Create(createInfo);
}

void RenderPassManager::CreateDefaultReflection()
{
	RenderPass::ClearDepth clearDepth{};
	clearDepth.clearDepth = 1.0f;
	clearDepth.clearStencil = 0;

	glm::vec4 clearColor = { 1.0f, 1.0f, 1.0f, 1.0f };

	RenderPass::AttachmentDescription color{};
	color.format = Format::R8G8B8A8_SRGB;
	color.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;

	RenderPass::CreateInfo createInfo{};
	createInfo.type = DefaultReflection;
	createInfo.clearColors = { clearColor };
	createInfo.clearDepths = { clearDepth };
	createInfo.attachmentDescriptions = { color };

	Create(createInfo);
}
