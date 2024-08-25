#include "RenderPassManager.h"

#include "Logger.h"
#include "MaterialManager.h"
#include "MeshManager.h"
#include "SceneManager.h"
#include "Time.h"

#include "../Components/Camera.h"
#include "../Components/DirectionalLight.h"
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
	CreateAtmosphere();
	CreateTransparent();
}

void RenderPassManager::CreateGBuffer()
{
	RenderPass::ClearDepth clearDepth{};
	clearDepth.clearDepth = 1.0f;
	clearDepth.clearStencil = 0;

	glm::vec4 clearColor = { 0.4f, 0.4f, 0.4f, 1.0f };
	glm::vec4 clearNormal = { 0.0f, 0.0f, 0.0f, 0.0f };
	glm::vec4 clearPosition = { 0.0f, 0.0f, 0.0f, 0.0f };
	glm::vec4 clearShading = { 0.0f, 0.0f, 0.0f, 0.0f };

	RenderPass::AttachmentDescription color{};
	color.format = Format::R8G8B8A8_SRGB;
	color.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;

	RenderPass::AttachmentDescription normal{};
	normal.format = Format::R16G16B16A16_SFLOAT;
	normal.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;

	RenderPass::AttachmentDescription position{};
	position.format = Format::R16G16B16A16_SFLOAT;
	position.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;

	RenderPass::AttachmentDescription shading{};
	shading.format = Format::R8G8B8A8_SRGB;
	shading.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;

	RenderPass::AttachmentDescription depth{};
	depth.format = Format::D32_SFLOAT;
	depth.layout = Texture::Layout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	RenderPass::CreateInfo createInfo{};
	createInfo.type = GBuffer;
	createInfo.clearColors = { clearColor, clearNormal, clearPosition, clearShading };
	createInfo.clearDepths = { clearDepth };
	createInfo.attachmentDescriptions = { color, normal, position, shading, depth };

	createInfo.renderCallback = [](const RenderPass::RenderCallbackInfo& renderInfo)
	{
		const std::string renderPassName = renderInfo.submitInfo.renderPass->GetType();

		{
			std::shared_ptr<Mesh> cubeMesh = MeshManager::GetInstance().LoadMesh("Meshes/Cube.mesh");
			std::shared_ptr<BaseMaterial> skyBoxBaseMaterial = MaterialManager::GetInstance().LoadBaseMaterial("Materials/SkyBox.basemat");

			const std::shared_ptr<Pipeline> pipeline = skyBoxBaseMaterial->GetPipeline(renderPassName);
			if (pipeline)
			{
				std::shared_ptr<UniformWriter> uniformWriter = skyBoxBaseMaterial->GetUniformWriter(renderPassName);
				uniformWriter->WriteTexture("SkyBox", renderInfo.renderer->GetRenderPassFrameBuffer(Atmosphere)->GetAttachment(0));

				std::vector<std::shared_ptr<UniformWriter>> uniformWriters = GetUniformWriters(pipeline, skyBoxBaseMaterial, nullptr, renderInfo);

				for (const auto& uniformWriter : uniformWriters)
				{
					uniformWriter->Flush();

					for (const auto& [location, buffer] : uniformWriter->GetBuffersByLocation())
					{
						buffer->Flush();
					}
				}

				renderInfo.renderer->Render(
					cubeMesh,
					pipeline,
					nullptr,
					0,
					1,
					uniformWriters,
					renderInfo.submitInfo);
			}
		}

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

			if (!r3d.mesh || !r3d.material || !r3d.material->IsPipelineEnabled(renderPassName))
			{
				continue;
			}

			const std::shared_ptr<Pipeline> pipeline = r3d.material->GetBaseMaterial()->GetPipeline(renderPassName);
			if (!pipeline)
			{
				continue;
			}

			materialMeshGameObjects[r3d.material->GetBaseMaterial()][r3d.material][r3d.mesh].emplace_back(entity);

			renderableCount++;
		}

		std::shared_ptr<Buffer> instanceBuffer = renderInfo.renderer->GetBuffer("InstanceBuffer");
		if ((renderableCount != 0 && !instanceBuffer) || (instanceBuffer && renderableCount != 0 && instanceBuffer->GetInstanceCount() != renderableCount))
		{
			instanceBuffer = Buffer::Create(
				sizeof(InstanceData),
				renderableCount,
				Buffer::Usage::VERTEX_BUFFER,
				Buffer::MemoryType::CPU);

			renderInfo.renderer->SetBuffer("InstanceBuffer", instanceBuffer);
		}

		std::vector<InstanceData> instanceDatas;

		// Render all base materials -> materials -> meshes | put gameobjects into the instance buffer.
		for (const auto& [baseMaterial, meshesByMaterial] : materialMeshGameObjects)
		{
			const std::shared_ptr<Pipeline> pipeline = baseMaterial->GetPipeline(renderPassName);

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

					std::vector<std::shared_ptr<UniformWriter>> uniformWriters = GetUniformWriters(pipeline, baseMaterial, material, renderInfo);

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

		const std::string renderPassName = renderInfo.submitInfo.renderPass->GetType();

		const std::shared_ptr<BaseMaterial> baseMaterial = MaterialManager::GetInstance().LoadBaseMaterial("Materials/Deferred.basemat");
		const std::shared_ptr<Pipeline> pipeline = baseMaterial->GetPipeline(renderPassName);
		if (!pipeline)
		{
			return;
		}

		std::shared_ptr<UniformWriter> renderUniformWriter = renderInfo.renderer->GetUniformWriter(renderPassName);
		if (!renderUniformWriter)
		{
			std::shared_ptr<UniformLayout> renderUniformLayout =
				pipeline->GetUniformLayout(*pipeline->GetDescriptorSetIndexByType(Pipeline::DescriptorSetIndexType::RENDERER, renderPassName));
			renderUniformWriter = UniformWriter::Create(renderUniformLayout);
			renderInfo.renderer->SetUniformWriter(renderPassName, renderUniformWriter);

			for (const auto& binding : renderUniformLayout->GetBindings())
			{
				if (binding.buffer && binding.buffer->name == "Lights")
				{
					const std::shared_ptr<Buffer> buffer = Buffer::Create(
						binding.buffer->size,
						1,
						Buffer::Usage::UNIFORM_BUFFER,
						Buffer::MemoryType::CPU);

					renderInfo.renderer->SetBuffer("Lights", buffer);
					renderUniformWriter->WriteBuffer(binding.buffer->name, buffer);
					renderUniformWriter->Flush();

					break;
				}
			}
		}

		const std::shared_ptr<FrameBuffer> frameBuffer = renderInfo.renderer->GetRenderPassFrameBuffer(GBuffer);

		renderUniformWriter->WriteTexture("albedoTexture", frameBuffer->GetAttachment(0));
		renderUniformWriter->WriteTexture("normalTexture", frameBuffer->GetAttachment(1));
		renderUniformWriter->WriteTexture("positionTexture", frameBuffer->GetAttachment(2));
		renderUniformWriter->WriteTexture("shadingTexture", frameBuffer->GetAttachment(3));

		const std::shared_ptr<Buffer> lightsBuffer = renderInfo.renderer->GetBuffer("Lights");

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

			glm::vec3 lightPosition = transform.GetPosition();
			WriterBufferHelper::WriteToBuffer(baseMaterial.get(), lightsBuffer, "Lights", valueNamePrefix + ".position", lightPosition);
			WriterBufferHelper::WriteToBuffer(baseMaterial.get(), lightsBuffer, "Lights", valueNamePrefix + ".color", pl.color);
			WriterBufferHelper::WriteToBuffer(baseMaterial.get(), lightsBuffer, "Lights", valueNamePrefix + ".linear", pl.linear);
			WriterBufferHelper::WriteToBuffer(baseMaterial.get(), lightsBuffer, "Lights", valueNamePrefix + ".quadratic", pl.quadratic);
			WriterBufferHelper::WriteToBuffer(baseMaterial.get(), lightsBuffer, "Lights", valueNamePrefix + ".constant", pl.constant);

			lightIndex++;
		}

		int pointLightsCount = pointLightView.size();
		WriterBufferHelper::WriteToBuffer(baseMaterial.get(), lightsBuffer, "Lights", "pointLightsCount", pointLightsCount);

		auto directionalLightView = renderInfo.scene->GetRegistry().view<DirectionalLight>();
		if (!directionalLightView.empty())
		{
			const entt::entity& entity = directionalLightView.back();
			DirectionalLight& dl = renderInfo.scene->GetRegistry().get<DirectionalLight>(entity);
			const Transform& transform = renderInfo.scene->GetRegistry().get<Transform>(entity);

			WriterBufferHelper::WriteToBuffer(baseMaterial.get(), lightsBuffer, "Lights", "directionalLight.color", dl.color);
			WriterBufferHelper::WriteToBuffer(baseMaterial.get(), lightsBuffer, "Lights", "directionalLight.intensity", dl.intensity);

			const glm::vec3 direction = transform.GetForward();
			WriterBufferHelper::WriteToBuffer(baseMaterial.get(), lightsBuffer, "Lights", "directionalLight.direction", direction);

			int hasDirectionalLight = 1;
			WriterBufferHelper::WriteToBuffer(baseMaterial.get(), lightsBuffer, "Lights", "hasDirectionalLight", hasDirectionalLight);
		}
		else
		{
			int hasDirectionalLight = 0;
			WriterBufferHelper::WriteToBuffer(baseMaterial.get(), lightsBuffer, "Lights", "hasDirectionalLight", hasDirectionalLight);
		}


		std::vector<std::shared_ptr<UniformWriter>> uniformWriters = GetUniformWriters(pipeline, baseMaterial, nullptr, renderInfo);

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

void RenderPassManager::CreateAtmosphere()
{
	RenderPass::ClearDepth clearDepth{};
	clearDepth.clearDepth = 1.0f;
	clearDepth.clearStencil = 0;

	glm::vec4 clearColor = { 0.0f, 0.0f, 0.0f, 0.0f };

	RenderPass::AttachmentDescription color{};
	color.format = Format::R16G16B16A16_SFLOAT;
	color.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;
	color.size = { 256, 256 };
	color.isCubeMap = true;
	
	RenderPass::CreateInfo createInfo{};
	createInfo.type = Atmosphere;
	createInfo.clearColors = { clearColor };
	createInfo.clearDepths = { clearDepth };
	createInfo.attachmentDescriptions = { color };

	const std::string globalBufferName = "GlobalBuffer";
	createInfo.renderCallback = [globalBufferName](const RenderPass::RenderCallbackInfo& renderInfo)
	{
		const std::shared_ptr<BaseMaterial> reflectionBaseMaterial = MaterialManager::GetInstance().LoadBaseMaterial("Materials/DefaultReflection.basemat");
		if (!renderInfo.renderer->GetBuffer(globalBufferName))
		{
			const std::shared_ptr<Pipeline> pipeline = reflectionBaseMaterial->GetPipeline(DefaultReflection);

			std::optional<uint32_t> descriptorSet = pipeline->GetDescriptorSetIndexByType(Pipeline::DescriptorSetIndexType::RENDERER, DefaultReflection);
			const std::shared_ptr<UniformLayout> uniformLayout = pipeline->GetUniformLayout(*descriptorSet);
			const std::shared_ptr<UniformWriter> uniformWriter = UniformWriter::Create(uniformLayout);

			const std::shared_ptr<Buffer> buffer = Buffer::Create(
				uniformLayout->GetBindingByName(globalBufferName)->buffer->size,
				1,
				Buffer::Usage::UNIFORM_BUFFER,
				Buffer::MemoryType::CPU);

			uniformWriter->WriteBuffer(globalBufferName, buffer);
			renderInfo.renderer->SetBuffer(globalBufferName, buffer);
			renderInfo.renderer->SetUniformWriter(DefaultReflection, uniformWriter);
		}

		const glm::mat4 viewProjectionMat4 = renderInfo.submitInfo.projection * renderInfo.camera->GetComponent<Camera>().GetViewMat4();
		WriterBufferHelper::WriteToBuffer(
			reflectionBaseMaterial.get(),
			renderInfo.renderer->GetBuffer(globalBufferName),
			globalBufferName,
			"camera.viewProjectionMat4",
			viewProjectionMat4);

		WriterBufferHelper::WriteToBuffer(
			reflectionBaseMaterial.get(),
			renderInfo.renderer->GetBuffer(globalBufferName),
			globalBufferName,
			"camera.projectionMat4",
			renderInfo.submitInfo.projection);

		const glm::mat4 inverseRotationMat4 = glm::inverse(renderInfo.camera->GetComponent<Transform>().GetRotationMat4());
		WriterBufferHelper::WriteToBuffer(
			reflectionBaseMaterial.get(),
			renderInfo.renderer->GetBuffer(globalBufferName),
			globalBufferName,
			"camera.inverseRotationMat4",
			inverseRotationMat4);

		const glm::vec3 cameraPosition = renderInfo.camera->GetComponent<Transform>().GetPosition();
		WriterBufferHelper::WriteToBuffer(
			reflectionBaseMaterial.get(),
			renderInfo.renderer->GetBuffer(globalBufferName),
			globalBufferName,
			"camera.position",
			cameraPosition);

		const float time = Time::GetTime();
		WriterBufferHelper::WriteToBuffer(
			reflectionBaseMaterial.get(),
			renderInfo.renderer->GetBuffer(globalBufferName),
			globalBufferName,
			"camera.time",
			time);

		const std::shared_ptr<Mesh> plane = MeshManager::GetInstance().LoadMesh("Meshes/Plane.mesh");
		if (!plane)
		{
			return;
		}

		const std::string renderPassName = renderInfo.submitInfo.renderPass->GetType();

		const std::shared_ptr<BaseMaterial> baseMaterial = MaterialManager::GetInstance().LoadBaseMaterial("Materials/Atmosphere.basemat");
		const std::shared_ptr<Pipeline> pipeline = baseMaterial->GetPipeline(renderPassName);
		if (!pipeline)
		{
			return;
		}

		auto directionalLightView = renderInfo.scene->GetRegistry().view<DirectionalLight>();
		if (!directionalLightView.empty())
		{
			const entt::entity& entity = directionalLightView.back();
			DirectionalLight& dl = renderInfo.scene->GetRegistry().get<DirectionalLight>(entity);
			const Transform& transform = renderInfo.scene->GetRegistry().get<Transform>(entity);

			baseMaterial->WriteToBuffer("AtmosphereBuffer", "directionalLight.color", dl.color);
			baseMaterial->WriteToBuffer("AtmosphereBuffer", "directionalLight.intensity", dl.intensity);

			const glm::vec3 direction = transform.GetForward();
			baseMaterial->WriteToBuffer("AtmosphereBuffer", "directionalLight.direction", direction);

			int hasDirectionalLight = 1;
			baseMaterial->WriteToBuffer("AtmosphereBuffer", "hasDirectionalLight", hasDirectionalLight);
		}
		else
		{
			int hasDirectionalLight = 0;
			baseMaterial->WriteToBuffer("AtmosphereBuffer", "hasDirectionalLight", hasDirectionalLight);
		}

		const glm::vec2 faceSize = renderInfo.renderer->GetRenderPassFrameBuffer(renderPassName)->GetSize();
		baseMaterial->WriteToBuffer("AtmosphereBuffer", "faceSize", faceSize);

		std::vector<std::shared_ptr<UniformWriter>> uniformWriters = GetUniformWriters(pipeline, baseMaterial, nullptr, renderInfo);

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

void RenderPassManager::CreateTransparent()
{
	RenderPass::ClearDepth clearDepth{};
	clearDepth.clearDepth = 1.0f;
	clearDepth.clearStencil = 0;

	glm::vec4 clearColor = { 1.0f, 1.0f, 1.0f, 1.0f };

	RenderPass::AttachmentDescription color{};
	color.format = Format::R8G8B8A8_SRGB;
	color.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;
	color.load = RenderPass::Load::LOAD;
	color.getFrameBufferCallback = [](Renderer* renderer, uint32_t& index)
	{
		index = 0;
		return renderer->GetRenderPassFrameBuffer(Deferred);
	};

	RenderPass::AttachmentDescription depth{};
	depth.format = Format::D32_SFLOAT;
	depth.layout = Texture::Layout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depth.load = RenderPass::Load::LOAD;
	depth.store = RenderPass::Store::NONE;
	depth.getFrameBufferCallback = [](Renderer* renderer, uint32_t& index)
	{
		index = 4;
		return renderer->GetRenderPassFrameBuffer(GBuffer);
	};

	RenderPass::CreateInfo createInfo{};
	createInfo.type = Transparent;
	createInfo.clearDepths = { clearDepth };
	createInfo.clearColors = { clearColor };
	createInfo.attachmentDescriptions = { color, depth };

	createInfo.renderCallback = [](const RenderPass::RenderCallbackInfo& renderInfo)
	{
		const std::string renderPassName = renderInfo.submitInfo.renderPass->GetType();

		struct RenderData
		{
			Renderer3D r3d;
			glm::mat4 transformMat4;
			glm::mat3 inversetransformMat3;
			glm::vec3 position;
		};

		std::vector<RenderData> renderDatas;

		size_t renderableCount = 0;
		const std::shared_ptr<Scene> scene = renderInfo.scene;
		entt::registry& registry = scene->GetRegistry();
		const auto r3dView = registry.view<Renderer3D>();
		for (const entt::entity& entity : r3dView)
		{
			Renderer3D& r3d = registry.get<Renderer3D>(entity);

			if (!r3d.mesh || !r3d.material || !r3d.material->IsPipelineEnabled(renderPassName))
			{
				continue;
			}

			const std::shared_ptr<Pipeline> pipeline = r3d.material->GetBaseMaterial()->GetPipeline(renderPassName);
			if (!pipeline)
			{
				continue;
			}

			Transform& transform = registry.get<Transform>(entity);

			RenderData renderData;
			renderData.r3d = r3d;
			renderData.position = transform.GetPosition();
			renderData.transformMat4 = transform.GetTransform();
			renderData.inversetransformMat3 = transform.GetInverseTransform();
			renderDatas.emplace_back(renderData);

			renderableCount++;
		}

		const glm::vec3 cameraPosition = renderInfo.camera->GetComponent<Transform>().GetPosition();

		auto isFurther = [cameraPosition](const RenderData& a, const RenderData& b)
		{
			float distance2A = glm::length2(cameraPosition - a.position);
			float distance2B = glm::length2(cameraPosition - b.position);

			return distance2A > distance2B;
		};

		std::sort(renderDatas.begin(), renderDatas.end(), isFurther);

		std::shared_ptr<Buffer> instanceBuffer = renderInfo.renderer->GetBuffer("InstanceBufferTransparent");
		if ((renderableCount != 0 && !instanceBuffer) || (instanceBuffer && renderableCount != 0 && instanceBuffer->GetInstanceCount() != renderableCount))
		{
			instanceBuffer = Buffer::Create(
				sizeof(InstanceData),
				renderableCount,
				Buffer::Usage::VERTEX_BUFFER,
				Buffer::MemoryType::CPU);

			renderInfo.renderer->SetBuffer("InstanceBufferTransparent", instanceBuffer);
		}

		std::vector<InstanceData> instanceDatas;

		// Render all base materials -> materials -> meshes | put gameobjects into the instance buffer.
		for (const auto& renderData : renderDatas)
		{
			const std::shared_ptr<Pipeline> pipeline = renderData.r3d.material->GetBaseMaterial()->GetPipeline(renderPassName);

			const size_t instanceDataOffset = instanceDatas.size();

			InstanceData data{};
			data.transform = renderData.transformMat4;
			data.inverseTransform = glm::transpose(renderData.inversetransformMat3);
			instanceDatas.emplace_back(data);


			std::vector<std::shared_ptr<UniformWriter>> uniformWriters = GetUniformWriters(
				pipeline,
				renderData.r3d.material->GetBaseMaterial(),
				renderData.r3d.material,
				renderInfo);

			for (const auto& uniformWriter : uniformWriters)
			{
				uniformWriter->Flush();

				for (const auto& [location, buffer] : uniformWriter->GetBuffersByLocation())
				{
					buffer->Flush();
				}
			}

			renderInfo.renderer->Render(
				renderData.r3d.mesh,
				pipeline,
				instanceBuffer,
				instanceDataOffset * instanceBuffer->GetInstanceSize(),
				1,
				uniformWriters,
				renderInfo.submitInfo);
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

std::vector<std::shared_ptr<UniformWriter>> Pengine::RenderPassManager::GetUniformWriters(
	std::shared_ptr<Pipeline> pipeline,
	std::shared_ptr<class BaseMaterial> baseMaterial,
	std::shared_ptr<class Material> material,
	const RenderPass::RenderCallbackInfo& renderInfo)
{
	std::vector<std::shared_ptr<UniformWriter>> uniformWriters;
	for (const auto& [set, location] : pipeline->GetSortedDescriptorSets())
	{
		switch (location.first)
		{
		case Pipeline::DescriptorSetIndexType::RENDERER:
			uniformWriters.emplace_back(renderInfo.renderer->GetUniformWriter(location.second));
			break;
		case Pipeline::DescriptorSetIndexType::RENDERPASS:
			uniformWriters.emplace_back(RenderPassManager::GetInstance().GetRenderPass(location.second)->GetUniformWriter());
			break;
		case Pipeline::DescriptorSetIndexType::BASE_MATERIAL:
			uniformWriters.emplace_back(baseMaterial->GetUniformWriter(location.second));
			break;
		case Pipeline::DescriptorSetIndexType::MATERIAL:
			uniformWriters.emplace_back(material->GetUniformWriter(location.second));
			break;
		default:
			break;
		}
	}

	return uniformWriters;
}
