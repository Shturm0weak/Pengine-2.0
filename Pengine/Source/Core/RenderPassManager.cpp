#include "RenderPassManager.h"

#include "Logger.h"
#include "MaterialManager.h"
#include "MeshManager.h"
#include "SceneManager.h"
#include "TextureManager.h"
#include "Time.h"
#include "FrustumCulling.h"

#include "../Components/Camera.h"
#include "../Components/DirectionalLight.h"
#include "../Components/PointLight.h"
#include "../Components/Renderer3D.h"
#include "../Components/Transform.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/RenderTarget.h"
#include "../EventSystem/EventSystem.h"
#include "../EventSystem/NextFrameEvent.h"

#include "../Core/ViewportManager.h"
#include "../Core/Viewport.h"

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

	FATAL_ERROR(type + " id of render pass doesn't exist, please create render pass!");

	return nullptr;
}

void RenderPassManager::ShutDown()
{
	m_RenderPassesByType.clear();
	m_LineRenderer.ShutDown();
	m_SSAORenderer.ShutDown();
}

std::vector<std::shared_ptr<UniformWriter>> RenderPassManager::GetUniformWriters(
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
			uniformWriters.emplace_back(renderInfo.renderTarget->GetUniformWriter(location.second));
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

void RenderPassManager::PrepareUniformsPerViewportBeforeDraw(const RenderPass::RenderCallbackInfo& renderInfo)
{
	const std::string globalBufferName = "GlobalBuffer";

	std::shared_ptr<Buffer> globalBuffer = renderInfo.renderTarget->GetBuffer(globalBufferName);
	const std::shared_ptr<BaseMaterial> reflectionBaseMaterial = MaterialManager::GetInstance().LoadBaseMaterial("Materials\\DefaultReflection.basemat");
	if (!renderInfo.renderTarget->GetBuffer(globalBufferName))
	{
		const std::shared_ptr<Pipeline> pipeline = reflectionBaseMaterial->GetPipeline(DefaultReflection);

		std::optional<uint32_t> descriptorSet = pipeline->GetDescriptorSetIndexByType(Pipeline::DescriptorSetIndexType::RENDERER, DefaultReflection);
		const std::shared_ptr<UniformLayout> uniformLayout = pipeline->GetUniformLayout(*descriptorSet);
		const std::shared_ptr<UniformWriter> uniformWriter = UniformWriter::Create(uniformLayout);

		globalBuffer = Buffer::Create(
			uniformLayout->GetBindingByName(globalBufferName)->buffer->size,
			1,
			Buffer::Usage::UNIFORM_BUFFER,
			Buffer::MemoryType::CPU);

		uniformWriter->WriteBuffer(globalBufferName, globalBuffer);
		renderInfo.renderTarget->SetBuffer(globalBufferName, globalBuffer);
		renderInfo.renderTarget->SetUniformWriter(DefaultReflection, uniformWriter);
	}

	const Camera& camera = renderInfo.camera->GetComponent<Camera>();
	const Transform& cameraTransform = renderInfo.camera->GetComponent<Transform>();
	const glm::mat4 viewProjectionMat4 = renderInfo.projection * camera.GetViewMat4();
	reflectionBaseMaterial->WriteToBuffer(
		globalBuffer,
		globalBufferName,
		"camera.viewProjectionMat4",
		viewProjectionMat4);

	const glm::mat4 viewMat4 = camera.GetViewMat4();
	reflectionBaseMaterial->WriteToBuffer(
		globalBuffer,
		globalBufferName,
		"camera.viewMat4",
		viewMat4);

	const glm::mat4 inverseViewMat4 = glm::inverse(camera.GetViewMat4());
	reflectionBaseMaterial->WriteToBuffer(
		globalBuffer,
		globalBufferName,
		"camera.inverseViewMat4",
		inverseViewMat4);

	reflectionBaseMaterial->WriteToBuffer(
		globalBuffer,
		globalBufferName,
		"camera.projectionMat4",
		renderInfo.projection);

	const glm::mat4 inverseRotationMat4 = glm::inverse(cameraTransform.GetRotationMat4());
	reflectionBaseMaterial->WriteToBuffer(
		globalBuffer,
		globalBufferName,
		"camera.inverseRotationMat4",
		inverseRotationMat4);

	const float time = Time::GetTime();
	reflectionBaseMaterial->WriteToBuffer(
		globalBuffer,
		globalBufferName,
		"camera.time",
		time);

	const float zNear = camera.GetZNear();
	reflectionBaseMaterial->WriteToBuffer(
		globalBuffer,
		globalBufferName,
		"camera.zNear",
		zNear);

	const float zFar = camera.GetZFar();
	reflectionBaseMaterial->WriteToBuffer(
		globalBuffer,
		globalBufferName,
		"camera.zFar",
		zFar);

	const glm::vec2 viewportSize = renderInfo.viewportSize;
	reflectionBaseMaterial->WriteToBuffer(
		renderInfo.renderTarget->GetBuffer(globalBufferName),
		globalBufferName,
		"camera.viewportSize",
		viewportSize);

	const float aspectRation = viewportSize.x / viewportSize.y;
	reflectionBaseMaterial->WriteToBuffer(
		renderInfo.renderTarget->GetBuffer(globalBufferName),
		globalBufferName,
		"camera.aspectRatio",
		aspectRation);

	const float tanHalfFOV = tanf(camera.GetFov() / 2.0f);
	reflectionBaseMaterial->WriteToBuffer(
		renderInfo.renderTarget->GetBuffer(globalBufferName),
		globalBufferName,
		"camera.tanHalfFOV",
		tanHalfFOV);
}

RenderPassManager::RenderPassManager()
{
	CreateDefaultReflection();
	CreateZPrePass();
	CreateGBuffer();
	CreateDeferred();
	CreateAtmosphere();
	CreateTransparent();
	CreateFinal();
	CreateSSAO();
	CreateSSAOBlur();
	CreateCSM();
	CreateBloom();
}

std::vector<std::shared_ptr<Buffer>> RenderPassManager::GetVertexBuffers(
	std::shared_ptr<Pipeline> pipeline,
	std::shared_ptr<Mesh> mesh)
{
	const auto& bindingDescriptions = pipeline->GetCreateInfo().bindingDescriptions;

	std::vector<std::shared_ptr<Buffer>> vertexBuffers;
	for (size_t i = 0; i < bindingDescriptions.size(); i++)
	{
		// NOTE: Consider that InputRate::Instance is always the last one.
		if (bindingDescriptions[i].inputRate == Pipeline::InputRate::VERTEX)
		{
			vertexBuffers.emplace_back(mesh->GetVertexBuffer(i));
		}
	}

	return vertexBuffers;
}

void RenderPassManager::CreateZPrePass()
{
	RenderPass::ClearDepth clearDepth{};
	clearDepth.clearDepth = 1.0f;
	clearDepth.clearStencil = 0;

	RenderPass::AttachmentDescription depth{};
	depth.format = Format::D32_SFLOAT;
	depth.layout = Texture::Layout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	Texture::SamplerCreateInfo depthSamplerCreateInfo{};
	depthSamplerCreateInfo.addressMode = Texture::SamplerCreateInfo::AddressMode::CLAMP_TO_BORDER;
	depthSamplerCreateInfo.borderColor = Texture::SamplerCreateInfo::BorderColor::FLOAT_OPAQUE_WHITE;

	depth.samplerCreateInfo = depthSamplerCreateInfo;

	RenderPass::CreateInfo createInfo{};
	createInfo.type = ZPrePass;
	createInfo.clearColors = { };
	createInfo.clearDepths = { clearDepth };
	createInfo.attachmentDescriptions = { depth };
	createInfo.resizeWithViewport = true;
	createInfo.resizeViewportScale = { 1.0f, 1.0f };

	createInfo.renderCallback = [this](const RenderPass::RenderCallbackInfo& renderInfo)
	{
		const std::string renderPassName = renderInfo.renderPass->GetType();

		using EntitiesByMesh = std::unordered_map<std::shared_ptr<Mesh>, std::vector<entt::entity>>;
		using MeshesByMaterial = std::unordered_map<std::shared_ptr<Material>, EntitiesByMesh>;
		using RenderableEntities = std::unordered_map<std::shared_ptr<BaseMaterial>, MeshesByMaterial>;

		RenderableEntities renderableEntities;

		size_t renderableCount = 0;
		const std::shared_ptr<Scene> scene = renderInfo.scene;
		const entt::registry& registry = scene->GetRegistry();
		const Camera& camera = renderInfo.camera->GetComponent<Camera>();
		const glm::mat4 viewProjectionMat4 = renderInfo.projection * camera.GetViewMat4();
		const auto r3dView = registry.view<Renderer3D>();
		for (const entt::entity& entity : r3dView)
		{
			const Renderer3D& r3d = registry.get<Renderer3D>(entity);
			const Transform& transform = registry.get<Transform>(entity);
			if (!transform.GetEntity()->IsEnabled())
			{
				continue;
			}

			// NOTE: Use GBuffer name for now, maybe in the future need to change.
			if (!r3d.mesh || !r3d.material || !r3d.material->IsPipelineEnabled(GBuffer))
			{
				continue;
			}

			const std::shared_ptr<Pipeline> pipeline = r3d.material->GetBaseMaterial()->GetPipeline(renderPassName);
			if (!pipeline)
			{
				continue;
			}

			const glm::mat4& transformMat4 = transform.GetTransform();
			const BoundingBox& box = r3d.mesh->GetBoundingBox();
			bool isInFrustum = FrustumCulling::CullBoundingBox(viewProjectionMat4, transformMat4, box.min, box.max, camera.GetZNear());
			if (!isInFrustum)
			{
				continue;
			}

			if (scene->GetSettings().m_DrawBoundingBoxes)
			{
				const glm::vec3 color = glm::vec3(0.0f, 1.0f, 0.0f);

				scene->GetVisualizer().DrawBox(box.min, box.max, color, transformMat4);
			}

			renderableEntities[r3d.material->GetBaseMaterial()][r3d.material][r3d.mesh].emplace_back(entity);

			renderableCount++;
		}

		std::shared_ptr<Buffer> instanceBuffer = renderInfo.renderTarget->GetBuffer("InstanceBufferZPrePass");
		if ((renderableCount != 0 && !instanceBuffer) || (instanceBuffer && renderableCount != 0 && instanceBuffer->GetInstanceCount() != renderableCount))
		{
			instanceBuffer = Buffer::Create(
				sizeof(glm::mat4),
				renderableCount,
				Buffer::Usage::VERTEX_BUFFER,
				Buffer::MemoryType::CPU);

			renderInfo.renderTarget->SetBuffer("InstanceBufferZPrePass", instanceBuffer);
		}

		std::vector<glm::mat4> instanceDatas;

		const std::shared_ptr<FrameBuffer> frameBuffer = renderInfo.renderTarget->GetFrameBuffer(renderPassName);

		RenderPass::SubmitInfo submitInfo{};
		submitInfo.frame = renderInfo.frame;
		submitInfo.renderPass = renderInfo.renderPass;
		submitInfo.frameBuffer = frameBuffer;
		renderInfo.renderer->BeginRenderPass(submitInfo);

		// Render all base materials -> materials -> meshes | put gameobjects into the instance buffer.
		for (const auto& [baseMaterial, meshesByMaterial] : renderableEntities)
		{
			const std::shared_ptr<Pipeline> pipeline = baseMaterial->GetPipeline(renderPassName);

			for (const auto& [material, gameObjectsByMeshes] : meshesByMaterial)
			{
				std::vector<std::shared_ptr<UniformWriter>> uniformWriters = GetUniformWriters(pipeline, baseMaterial, material, renderInfo);
				for (const auto& uniformWriter : uniformWriters)
				{
					uniformWriter->Flush();

					for (const auto& [location, buffer] : uniformWriter->GetBuffersByLocation())
					{
						buffer->Flush();
					}
				}

				for (const auto& [mesh, entities] : gameObjectsByMeshes)
				{
					const size_t instanceDataOffset = instanceDatas.size();

					for (const entt::entity& entity : entities)
					{
						const Transform& transform = registry.get<Transform>(entity);
						instanceDatas.emplace_back(transform.GetTransform());
					}

					renderInfo.renderer->Render(
						GetVertexBuffers(pipeline, mesh),
						mesh->GetIndexBuffer(),
						mesh->GetIndexCount(),
						pipeline,
						instanceBuffer,
						instanceDataOffset * instanceBuffer->GetInstanceSize(),
						entities.size(),
						uniformWriters,
						renderInfo.frame);
				}
			}
		}

		struct RenderableData
		{
			RenderableEntities renderableEntities;
			size_t renderableCount = 0;
		};
		RenderableData* renderableData = (RenderableData*)renderInfo.renderTarget->GetCustomData("RenderableData");
		if (!renderableData)
		{
			renderableData = new RenderableData();
			renderInfo.renderTarget->SetCustomData("RenderableData", renderableData);
		}
		renderableData->renderableEntities = std::move(renderableEntities);
		renderableData->renderableCount = renderableCount;

		// Because these are all just commands and will be rendered later we can write the instance buffer
		// just once when all instance data is collected.
		if (instanceBuffer && !instanceDatas.empty())
		{
			instanceBuffer->WriteToBuffer(instanceDatas.data(), instanceDatas.size() * sizeof(glm::mat4));
		}

		renderInfo.renderer->EndRenderPass(submitInfo);
	};

	Create(createInfo);
}

void RenderPassManager::CreateGBuffer()
{
	RenderPass::ClearDepth clearDepth{};
	clearDepth.clearDepth = 1.0f;
	clearDepth.clearStencil = 0;

	glm::vec4 clearColor = { 0.4f, 0.4f, 0.4f, 1.0f };
	glm::vec4 clearNormal = { 0.0f, 0.0f, 0.0f, 0.0f };
	glm::vec4 clearShading = { 0.0f, 0.0f, 0.0f, 0.0f };
	glm::vec4 clearEmissive = { 0.0f, 0.0f, 0.0f, 0.0f };

	RenderPass::AttachmentDescription color{};
	color.format = Format::B10G11R11_UFLOAT_PACK32;
	color.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;
	color.load = RenderPass::Load::LOAD;
	color.store = RenderPass::Store::STORE;

	RenderPass::AttachmentDescription normal{};
	normal.format = Format::R32G32B32A32_SFLOAT;
	normal.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;
	normal.load = RenderPass::Load::LOAD;
	normal.store = RenderPass::Store::STORE;

	RenderPass::AttachmentDescription shading{};
	shading.format = Format::R8G8B8A8_SRGB;
	shading.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;
	shading.load = RenderPass::Load::LOAD;
	shading.store = RenderPass::Store::STORE;

	RenderPass::AttachmentDescription emissive{};
	emissive.format = Format::R16G16B16A16_SFLOAT;
	emissive.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;
	emissive.load = RenderPass::Load::LOAD;
	emissive.store = RenderPass::Store::STORE;

	Texture::SamplerCreateInfo emissiveSamplerCreateInfo{};
	emissiveSamplerCreateInfo.addressMode = Texture::SamplerCreateInfo::AddressMode::CLAMP_TO_BORDER;
	emissiveSamplerCreateInfo.borderColor = Texture::SamplerCreateInfo::BorderColor::FLOAT_OPAQUE_BLACK;
	emissiveSamplerCreateInfo.maxAnisotropy = 1.0f;

	emissive.samplerCreateInfo = emissiveSamplerCreateInfo;

	RenderPass::AttachmentDescription depth{};
	depth.format = Format::D32_SFLOAT;
	depth.layout = Texture::Layout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depth.load = RenderPass::Load::LOAD;
	depth.store = RenderPass::Store::NONE;
	depth.getFrameBufferCallback = [](RenderTarget* renderTarget, uint32_t& index)
	{
		index = 0;
		return renderTarget->GetFrameBuffer(ZPrePass);
	};

	Texture::SamplerCreateInfo depthSamplerCreateInfo{};
	depthSamplerCreateInfo.addressMode = Texture::SamplerCreateInfo::AddressMode::CLAMP_TO_BORDER;
	depthSamplerCreateInfo.borderColor = Texture::SamplerCreateInfo::BorderColor::FLOAT_OPAQUE_WHITE;

	depth.samplerCreateInfo = depthSamplerCreateInfo;

	RenderPass::CreateInfo createInfo{};
	createInfo.type = GBuffer;
	createInfo.clearColors = { clearColor, clearNormal, clearShading, clearEmissive };
	createInfo.clearDepths = { clearDepth };
	createInfo.attachmentDescriptions = { color, normal, shading, emissive, depth };
	createInfo.resizeWithViewport = true;
	createInfo.resizeViewportScale = { 1.0f, 1.0f };

	createInfo.renderCallback = [this](const RenderPass::RenderCallbackInfo& renderInfo)
	{
		const std::string renderPassName = renderInfo.renderPass->GetType();
		
		using EntitiesByMesh = std::unordered_map<std::shared_ptr<Mesh>, std::vector<entt::entity>>;
		using MeshesByMaterial = std::unordered_map<std::shared_ptr<Material>, EntitiesByMesh>;
		using RenderableEntities = std::unordered_map<std::shared_ptr<BaseMaterial>, MeshesByMaterial>;

		struct RenderableData
		{
			RenderableEntities renderableEntities;
			size_t renderableCount = 0;
		};
		RenderableData* renderableData = (RenderableData*)renderInfo.renderTarget->GetCustomData("RenderableData");

		const size_t renderableCount = renderableData->renderableCount;
		const std::shared_ptr<Scene> scene = renderInfo.scene;
		const entt::registry& registry = scene->GetRegistry();

		std::shared_ptr<Buffer> instanceBuffer = renderInfo.renderTarget->GetBuffer("InstanceBuffer");
		if ((renderableCount != 0 && !instanceBuffer) || (instanceBuffer && renderableCount != 0 && instanceBuffer->GetInstanceCount() != renderableCount))
		{
			instanceBuffer = Buffer::Create(
				sizeof(InstanceData),
				renderableCount,
				Buffer::Usage::VERTEX_BUFFER,
				Buffer::MemoryType::CPU);

			renderInfo.renderTarget->SetBuffer("InstanceBuffer", instanceBuffer);
		}

		std::vector<InstanceData> instanceDatas;

		const std::shared_ptr<FrameBuffer> frameBuffer = renderInfo.renderTarget->GetFrameBuffer(renderPassName);

		RenderPass::SubmitInfo submitInfo{};
		submitInfo.frame = renderInfo.frame;
		submitInfo.renderPass = renderInfo.renderPass;
		submitInfo.frameBuffer = frameBuffer;
		renderInfo.renderer->BeginRenderPass(submitInfo);

		// Render all base materials -> materials -> meshes | put gameobjects into the instance buffer.
		for (const auto& [baseMaterial, meshesByMaterial] : renderableData->renderableEntities)
		{
			const std::shared_ptr<Pipeline> pipeline = baseMaterial->GetPipeline(renderPassName);

			for (const auto& [material, gameObjectsByMeshes] : meshesByMaterial)
			{
				std::vector<std::shared_ptr<UniformWriter>> uniformWriters = GetUniformWriters(pipeline, baseMaterial, material, renderInfo);
				// Already updated in ZPrePass.
				/*for (const auto& uniformWriter : uniformWriters)
				{
					uniformWriter->Flush();

					for (const auto& [location, buffer] : uniformWriter->GetBuffersByLocation())
					{
						buffer->Flush();
					}
				}*/

				for (const auto& [mesh, entities] : gameObjectsByMeshes)
				{
					const size_t instanceDataOffset = instanceDatas.size();

					for (const entt::entity& entity : entities)
					{
						InstanceData data{};
						const Transform& transform = registry.get<Transform>(entity);
						data.transform = transform.GetTransform();
						data.inverseTransform = glm::transpose(transform.GetInverseTransform());
						instanceDatas.emplace_back(data);
					}

					renderInfo.renderer->Render(
						GetVertexBuffers(pipeline, mesh),
						mesh->GetIndexBuffer(),
						mesh->GetIndexCount(),
						pipeline,
						instanceBuffer,
						instanceDataOffset * instanceBuffer->GetInstanceSize(),
						entities.size(),
						uniformWriters,
						renderInfo.frame);
				}
			}
		}

		// NOTE: Clear after the draw when not it is needed. If not clear, the crash will appear when closing the application.
		renderableData->renderableCount = 0;
		renderableData->renderableEntities.clear();

		// Because these are all just commands and will be rendered later we can write the instance buffer
		// just once when all instance data is collected.
		if (instanceBuffer && !instanceDatas.empty())
		{
			instanceBuffer->WriteToBuffer(instanceDatas.data(), instanceDatas.size() * sizeof(InstanceData));
		}

		m_LineRenderer.Render(renderInfo);

		// Render SkyBox.
		{
			std::shared_ptr<Mesh> cubeMesh = MeshManager::GetInstance().LoadMesh("SkyBoxCube");
			std::shared_ptr<BaseMaterial> skyBoxBaseMaterial = MaterialManager::GetInstance().LoadBaseMaterial("Materials\\SkyBox.basemat");

			const std::shared_ptr<Pipeline> pipeline = skyBoxBaseMaterial->GetPipeline(renderPassName);
			if (pipeline)
			{
				std::vector<std::shared_ptr<UniformWriter>> uniformWriters = GetUniformWriters(pipeline, skyBoxBaseMaterial, nullptr, renderInfo);

				renderInfo.renderer->Render(
					GetVertexBuffers(pipeline, cubeMesh),
					cubeMesh->GetIndexBuffer(),
					cubeMesh->GetIndexCount(),
					pipeline,
					nullptr,
					0,
					1,
					uniformWriters,
					renderInfo.frame);
			}
		}

		renderInfo.renderer->EndRenderPass(submitInfo);
	};

	Create(createInfo);
}

void RenderPassManager::CreateDeferred()
{
	glm::vec4 clearColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	glm::vec4 clearEmissive = { 0.0f, 0.0f, 0.0f, 0.0f };

	RenderPass::AttachmentDescription color{};
	color.format = Format::B10G11R11_UFLOAT_PACK32;
	color.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;
	color.load = RenderPass::Load::LOAD;
	color.store = RenderPass::Store::STORE;

	RenderPass::AttachmentDescription emissive{};
	emissive.format = Format::R16G16B16A16_SFLOAT;
	emissive.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;
	emissive.load = RenderPass::Load::LOAD;
	emissive.store = RenderPass::Store::STORE;
	emissive.getFrameBufferCallback = [](RenderTarget* renderTarget, uint32_t& index)
	{
		index = 3;
		return renderTarget->GetFrameBuffer(GBuffer);
	};

	Texture::SamplerCreateInfo samplerCreateInfo{};
	samplerCreateInfo.addressMode = Texture::SamplerCreateInfo::AddressMode::CLAMP_TO_BORDER;
	samplerCreateInfo.borderColor = Texture::SamplerCreateInfo::BorderColor::FLOAT_OPAQUE_BLACK;
	samplerCreateInfo.maxAnisotropy = 1.0f;

	emissive.samplerCreateInfo = samplerCreateInfo;

	RenderPass::CreateInfo createInfo{};
	createInfo.type = Deferred;
	createInfo.clearColors = { clearColor, clearEmissive };
	createInfo.attachmentDescriptions = { color, emissive };
	createInfo.resizeWithViewport = true;
	createInfo.resizeViewportScale = { 1.0f, 1.0f };

	const std::shared_ptr<Mesh> planeMesh = nullptr;

	createInfo.renderCallback = [this](const RenderPass::RenderCallbackInfo& renderInfo)
	{
		const std::shared_ptr<Mesh> plane = MeshManager::GetInstance().LoadMesh("FullScreenQuad");
		if (!plane)
		{
			return;
		}

		const std::string renderPassName = renderInfo.renderPass->GetType();

		const std::shared_ptr<BaseMaterial> baseMaterial = MaterialManager::GetInstance().LoadBaseMaterial("Materials\\Deferred.basemat");
		const std::shared_ptr<Pipeline> pipeline = baseMaterial->GetPipeline(renderPassName);
		if (!pipeline)
		{
			return;
		}

		std::shared_ptr<UniformWriter> renderUniformWriter = renderInfo.renderTarget->GetUniformWriter(renderPassName);
		if (!renderUniformWriter)
		{
			std::shared_ptr<UniformLayout> renderUniformLayout =
				pipeline->GetUniformLayout(*pipeline->GetDescriptorSetIndexByType(Pipeline::DescriptorSetIndexType::RENDERER, renderPassName));
			renderUniformWriter = UniformWriter::Create(renderUniformLayout);
			renderInfo.renderTarget->SetUniformWriter(renderPassName, renderUniformWriter);

			for (const auto& binding : renderUniformLayout->GetBindings())
			{
				if (binding.buffer && binding.buffer->name == "Lights")
				{
					const std::shared_ptr<Buffer> buffer = Buffer::Create(
						binding.buffer->size,
						1,
						Buffer::Usage::UNIFORM_BUFFER,
						Buffer::MemoryType::CPU);

					renderInfo.renderTarget->SetBuffer("Lights", buffer);
					renderUniformWriter->WriteBuffer(binding.buffer->name, buffer);
					renderUniformWriter->Flush();

					break;
				}
			}
		}

		for (const auto& [name, renderTargetInfo] : pipeline->GetCreateInfo().uniformInfo.renderTargetsByName)
		{
			const std::shared_ptr<FrameBuffer> frameBuffer = renderInfo.renderTarget->GetFrameBuffer(renderTargetInfo.renderPassName);
			if (frameBuffer)
			{
				renderUniformWriter->WriteTexture(name, frameBuffer->GetAttachment(renderTargetInfo.attachmentIndex));
			}
			else if (!renderTargetInfo.renderTargetDefault.empty())
			{
				renderUniformWriter->WriteTexture(name, TextureManager::GetInstance().GetTexture(renderTargetInfo.renderTargetDefault));
			}
			else
			{
				renderUniformWriter->WriteTexture(name, TextureManager::GetInstance().GetWhite());
			}
		}

		const Camera& camera = renderInfo.camera->GetComponent<Camera>();

		const std::shared_ptr<Buffer> lightsBuffer = renderInfo.renderTarget->GetBuffer("Lights");

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

			// View Space!
			glm::vec3 lightPosition = camera.GetViewMat4() * glm::vec4(transform.GetPosition(), 1.0f);
			baseMaterial->WriteToBuffer(lightsBuffer, "Lights", valueNamePrefix + ".position", lightPosition);
			baseMaterial->WriteToBuffer(lightsBuffer, "Lights", valueNamePrefix + ".color", pl.color);
			baseMaterial->WriteToBuffer(lightsBuffer, "Lights", valueNamePrefix + ".linear", pl.linear);
			baseMaterial->WriteToBuffer(lightsBuffer, "Lights", valueNamePrefix + ".quadratic", pl.quadratic);
			baseMaterial->WriteToBuffer(lightsBuffer, "Lights", valueNamePrefix + ".constant", pl.constant);

			lightIndex++;
		}

		int pointLightsCount = pointLightView.size();
		baseMaterial->WriteToBuffer(lightsBuffer, "Lights", "pointLightsCount", pointLightsCount);

		const GraphicsSettings& graphicsSettings = renderInfo.scene->GetGraphicsSettings();
		baseMaterial->WriteToBuffer(lightsBuffer, "Lights", "brightnessThreshold", graphicsSettings.bloom.brightnessThreshold);

		auto directionalLightView = renderInfo.scene->GetRegistry().view<DirectionalLight>();
		if (!directionalLightView.empty())
		{
			const entt::entity& entity = directionalLightView.back();
			DirectionalLight& dl = renderInfo.scene->GetRegistry().get<DirectionalLight>(entity);
			const Transform& transform = renderInfo.scene->GetRegistry().get<Transform>(entity);

			baseMaterial->WriteToBuffer(lightsBuffer, "Lights", "directionalLight.color", dl.color);
			baseMaterial->WriteToBuffer(lightsBuffer, "Lights", "directionalLight.intensity", dl.intensity);
			baseMaterial->WriteToBuffer(lightsBuffer, "Lights", "directionalLight.ambient", dl.ambient);

			// View Space!
			const glm::vec3 direction = glm::normalize(glm::mat3(camera.GetViewMat4()) * transform.GetForward());
			baseMaterial->WriteToBuffer(lightsBuffer, "Lights", "directionalLight.direction", direction);

			const int hasDirectionalLight = 1;
			baseMaterial->WriteToBuffer(lightsBuffer, "Lights", "hasDirectionalLight", hasDirectionalLight);

			const GraphicsSettings::Shadows& shadowSettings = renderInfo.scene->GetGraphicsSettings().shadows;

			const int isEnabled = shadowSettings.isEnabled;
			baseMaterial->WriteToBuffer(lightsBuffer, "Lights", "csm.isEnabled", isEnabled);

			CSMRenderer& csmRenderer = m_CSMRenderersByCSMSetting[renderInfo.scene->GetGraphicsSettings().GetFilepath().wstring()];
			if (shadowSettings.isEnabled && !csmRenderer.GetLightSpaceMatrices().empty())
			{
				std::vector<glm::vec4> shadowCascadeLevels;
				for (const float& distance : csmRenderer.GetDistances())
				{
					shadowCascadeLevels.emplace_back(glm::vec4(distance));
				}

				baseMaterial->WriteToBuffer(lightsBuffer, "Lights", "csm.lightSpaceMatrices", *csmRenderer.GetLightSpaceMatrices().data());

				baseMaterial->WriteToBuffer(lightsBuffer, "Lights", "csm.distances", *shadowCascadeLevels.data());

				const int cascadeCount = csmRenderer.GetLightSpaceMatrices().size();
				baseMaterial->WriteToBuffer(lightsBuffer, "Lights", "csm.cascadeCount", cascadeCount);

				baseMaterial->WriteToBuffer(lightsBuffer, "Lights", "csm.fogFactor", shadowSettings.fogFactor);
				baseMaterial->WriteToBuffer(lightsBuffer, "Lights", "csm.maxDistance", shadowSettings.maxDistance);
				baseMaterial->WriteToBuffer(lightsBuffer, "Lights", "csm.pcfRange", shadowSettings.pcfRange);

				const int pcfEnabled = shadowSettings.pcfEnabled;
				baseMaterial->WriteToBuffer(lightsBuffer, "Lights", "csm.pcfEnabled", pcfEnabled);

				const int visualize = shadowSettings.visualize;
				baseMaterial->WriteToBuffer(lightsBuffer, "Lights", "csm.visualize", visualize);

				std::vector<glm::vec4> biases;
				for (const float& bias : shadowSettings.biases)
				{
					biases.emplace_back(glm::vec4(bias));
				}
				baseMaterial->WriteToBuffer(lightsBuffer, "Lights", "csm.biases", *biases.data());
			}
		}
		else
		{
			const int hasDirectionalLight = 0;
			baseMaterial->WriteToBuffer(lightsBuffer, "Lights", "hasDirectionalLight", hasDirectionalLight);
			baseMaterial->WriteToBuffer(lightsBuffer, "Lights", "csm.cascadeCount", hasDirectionalLight);
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

		const std::shared_ptr<FrameBuffer> frameBuffer = renderInfo.renderTarget->GetFrameBuffer(renderPassName);
		RenderPass::SubmitInfo submitInfo{};
		submitInfo.frame = renderInfo.frame;
		submitInfo.renderPass = renderInfo.renderPass;
		submitInfo.frameBuffer = frameBuffer;
		renderInfo.renderer->BeginRenderPass(submitInfo);

		renderInfo.renderer->Render(
			GetVertexBuffers(pipeline, plane),
			plane->GetIndexBuffer(),
			plane->GetIndexCount(),
			pipeline,
			nullptr,
			0,
			1,
			uniformWriters,
			renderInfo.frame);

		renderInfo.renderer->EndRenderPass(submitInfo);
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
	glm::vec4 clearColor = { 0.0f, 0.0f, 0.0f, 0.0f };

	RenderPass::AttachmentDescription color{};
	color.format = Format::R16G16B16A16_SFLOAT;
	color.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;
	color.load = RenderPass::Load::LOAD;
	color.store = RenderPass::Store::STORE;
	color.size = { 256, 256 };
	color.isCubeMap = true;
	
	RenderPass::CreateInfo createInfo{};
	createInfo.type = Atmosphere;
	createInfo.clearColors = { clearColor };
	createInfo.attachmentDescriptions = { color };
	createInfo.resizeWithViewport = false;

	createInfo.renderCallback = [this](const RenderPass::RenderCallbackInfo& renderInfo)
	{
		const std::shared_ptr<Mesh> plane = MeshManager::GetInstance().GetMesh("FullScreenQuad");
		if (!plane)
		{
			return;
		}

		const std::string renderPassName = renderInfo.renderPass->GetType();

		const std::shared_ptr<BaseMaterial> baseMaterial = MaterialManager::GetInstance().LoadBaseMaterial("Materials\\Atmosphere.basemat");
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

		const std::shared_ptr<FrameBuffer> frameBuffer = renderInfo.scene->GetRenderTarget()->GetFrameBuffer(Atmosphere);
		const glm::vec2 faceSize = frameBuffer->GetSize();
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
		
		RenderPass::SubmitInfo submitInfo{};
		submitInfo.frame = renderInfo.frame;
		submitInfo.renderPass = renderInfo.renderPass;
		submitInfo.frameBuffer = frameBuffer;
		renderInfo.renderer->BeginRenderPass(submitInfo);

		renderInfo.renderer->Render(
			GetVertexBuffers(pipeline, plane),
			plane->GetIndexBuffer(),
			plane->GetIndexCount(),
			pipeline,
			nullptr,
			0,
			1,
			uniformWriters,
			renderInfo.frame);

		renderInfo.renderer->EndRenderPass(submitInfo);

		// Update SkyBox.
		{
			std::shared_ptr<BaseMaterial> skyBoxBaseMaterial = MaterialManager::GetInstance().LoadBaseMaterial("Materials\\SkyBox.basemat");

			const std::shared_ptr<Pipeline> pipeline = skyBoxBaseMaterial->GetPipeline(GBuffer);
			if (pipeline)
			{
				std::shared_ptr<UniformWriter> uniformWriter = skyBoxBaseMaterial->GetUniformWriter(GBuffer);
				uniformWriter->WriteTexture("SkyBox", frameBuffer->GetAttachment(0));
				uniformWriter->Flush();
			}
		}
	};

	const std::shared_ptr<RenderPass> renderPass = Create(createInfo);
}

void RenderPassManager::CreateTransparent()
{
	RenderPass::ClearDepth clearDepth{};
	clearDepth.clearDepth = 1.0f;
	clearDepth.clearStencil = 0;

	glm::vec4 clearColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	glm::vec4 clearEmissive = { 0.0f, 0.0f, 0.0f, 1.0f };

	RenderPass::AttachmentDescription color{};
	color.format = Format::B10G11R11_UFLOAT_PACK32;
	color.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;
	color.load = RenderPass::Load::LOAD;
	color.store = RenderPass::Store::STORE;
	color.getFrameBufferCallback = [](RenderTarget* renderTarget, uint32_t& index)
	{
		index = 0;
		return renderTarget->GetFrameBuffer(Deferred);
	};

	RenderPass::AttachmentDescription emissive{};
	emissive.format = Format::R16G16B16A16_SFLOAT;
	emissive.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;
	emissive.load = RenderPass::Load::LOAD;
	emissive.store = RenderPass::Store::STORE;
	emissive.getFrameBufferCallback = [](RenderTarget* renderTarget, uint32_t& index)
	{
		index = 3;
		return renderTarget->GetFrameBuffer(GBuffer);
	};

	Texture::SamplerCreateInfo samplerCreateInfo{};
	samplerCreateInfo.addressMode = Texture::SamplerCreateInfo::AddressMode::CLAMP_TO_BORDER;
	samplerCreateInfo.borderColor = Texture::SamplerCreateInfo::BorderColor::FLOAT_OPAQUE_BLACK;
	samplerCreateInfo.maxAnisotropy = 1.0f;

	emissive.samplerCreateInfo = samplerCreateInfo;

	RenderPass::AttachmentDescription depth{};
	depth.format = Format::D32_SFLOAT;
	depth.layout = Texture::Layout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depth.load = RenderPass::Load::LOAD;
	depth.store = RenderPass::Store::NONE;
	depth.getFrameBufferCallback = [](RenderTarget* renderTarget, uint32_t& index)
	{
		index = 0;
		return renderTarget->GetFrameBuffer(ZPrePass);
	};

	RenderPass::CreateInfo createInfo{};
	createInfo.type = Transparent;
	createInfo.clearDepths = { clearDepth };
	createInfo.clearColors = { clearColor, clearEmissive };
	createInfo.attachmentDescriptions = { color, emissive, depth };
	createInfo.resizeWithViewport = true;

	createInfo.renderCallback = [](const RenderPass::RenderCallbackInfo& renderInfo)
	{
		const std::string renderPassName = renderInfo.renderPass->GetType();

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
			Transform& transform = registry.get<Transform>(entity);
			if (!transform.GetEntity()->IsEnabled())
			{
				continue;
			}

			if (!r3d.mesh || !r3d.material || !r3d.material->IsPipelineEnabled(renderPassName))
			{
				continue;
			}

			const std::shared_ptr<Pipeline> pipeline = r3d.material->GetBaseMaterial()->GetPipeline(renderPassName);
			if (!pipeline)
			{
				continue;
			}

			const Camera& camera = renderInfo.camera->GetComponent<Camera>();
			const glm::mat4& transformMat4 = transform.GetTransform();
			const BoundingBox& box = r3d.mesh->GetBoundingBox();
			bool isInFrustum = FrustumCulling::CullBoundingBox(renderInfo.projection * camera.GetViewMat4(), transformMat4, box.min, box.max, camera.GetZNear());
			if (!isInFrustum)
			{
				continue;
			}

			if (scene->GetSettings().m_DrawBoundingBoxes)
			{
				const glm::vec3 color = glm::vec3(0.0f, 1.0f, 0.0f);

				scene->GetVisualizer().DrawBox(box.min, box.max, color, transformMat4);
			}

			RenderData renderData{};
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

		std::shared_ptr<Buffer> instanceBuffer = renderInfo.renderTarget->GetBuffer("InstanceBufferTransparent");
		if ((renderableCount != 0 && !instanceBuffer) || (instanceBuffer && renderableCount != 0 && instanceBuffer->GetInstanceCount() != renderableCount))
		{
			instanceBuffer = Buffer::Create(
				sizeof(InstanceData),
				renderableCount,
				Buffer::Usage::VERTEX_BUFFER,
				Buffer::MemoryType::CPU);

			renderInfo.renderTarget->SetBuffer("InstanceBufferTransparent", instanceBuffer);
		}

		std::vector<InstanceData> instanceDatas;

		const std::shared_ptr<FrameBuffer> frameBuffer = renderInfo.renderTarget->GetFrameBuffer(renderPassName);
		RenderPass::SubmitInfo submitInfo{};
		submitInfo.frame = renderInfo.frame;
		submitInfo.renderPass = renderInfo.renderPass;
		submitInfo.frameBuffer = frameBuffer;
		renderInfo.renderer->BeginRenderPass(submitInfo);

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
				GetVertexBuffers(pipeline, renderData.r3d.mesh),
				renderData.r3d.mesh->GetIndexBuffer(),
				renderData.r3d.mesh->GetIndexCount(),
				pipeline,
				instanceBuffer,
				instanceDataOffset * instanceBuffer->GetInstanceSize(),
				1,
				uniformWriters,
				renderInfo.frame);
		}

		// Because these are all just commands and will be rendered later we can write the instance buffer
		// just once when all instance data is collected.
		if (instanceBuffer && !instanceDatas.empty())
		{
			instanceBuffer->WriteToBuffer(instanceDatas.data(), instanceDatas.size() * sizeof(InstanceData));
		}

		renderInfo.renderer->EndRenderPass(submitInfo);
	};

	Create(createInfo);
}

void RenderPassManager::CreateFinal()
{
	RenderPass::ClearDepth clearDepth{};
	clearDepth.clearDepth = 1.0f;
	clearDepth.clearStencil = 0;

	glm::vec4 clearColor = { 1.0f, 1.0f, 1.0f, 1.0f };

	RenderPass::AttachmentDescription color{};
	color.format = Format::B10G11R11_UFLOAT_PACK32;
	color.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;
	color.load = RenderPass::Load::LOAD;
	color.store = RenderPass::Store::STORE;

	RenderPass::CreateInfo createInfo{};
	createInfo.type = Final;
	createInfo.clearColors = { clearColor };
	createInfo.clearDepths = { clearDepth };
	createInfo.attachmentDescriptions = { color };
	createInfo.resizeWithViewport = true;
	createInfo.resizeViewportScale = { 1.0f, 1.0f };

	const std::shared_ptr<Mesh> planeMesh = nullptr;

	createInfo.renderCallback = [](const RenderPass::RenderCallbackInfo& renderInfo)
	{
		const std::shared_ptr<Mesh> plane = MeshManager::GetInstance().LoadMesh("FullScreenQuad");
		if (!plane)
		{
			return;
		}

		const std::string renderPassName = renderInfo.renderPass->GetType();

		const std::shared_ptr<BaseMaterial> baseMaterial = MaterialManager::GetInstance().LoadBaseMaterial("Materials/Final.basemat");
		const std::shared_ptr<Pipeline> pipeline = baseMaterial->GetPipeline(renderPassName);
		if (!pipeline)
		{
			return;
		}

		const GraphicsSettings& graphicsSettings = renderInfo.scene->GetGraphicsSettings();

		std::shared_ptr<UniformWriter> renderUniformWriter = renderInfo.renderTarget->GetUniformWriter(renderPassName);
		if (!renderUniformWriter)
		{
			std::shared_ptr<UniformLayout> renderUniformLayout =
				pipeline->GetUniformLayout(*pipeline->GetDescriptorSetIndexByType(Pipeline::DescriptorSetIndexType::RENDERER, renderPassName));
			renderUniformWriter = UniformWriter::Create(renderUniformLayout);
			renderInfo.renderTarget->SetUniformWriter(renderPassName, renderUniformWriter);

			for (const auto& binding : renderUniformLayout->GetBindings())
			{
				if (binding.buffer && binding.buffer->name == "PostProcessBuffer")
				{
					const std::shared_ptr<Buffer> buffer = Buffer::Create(
						binding.buffer->size,
						1,
						Buffer::Usage::UNIFORM_BUFFER,
						Buffer::MemoryType::CPU);

					renderInfo.renderTarget->SetBuffer("PostProcessBuffer", buffer);
					renderUniformWriter->WriteBuffer(binding.buffer->name, buffer);
					renderUniformWriter->Flush();

					break;
				}
			}
		}

		// Deferred texture.
		{
			const std::shared_ptr<FrameBuffer> frameBuffer = renderInfo.renderTarget->GetFrameBuffer("Deferred");
			renderUniformWriter->WriteTexture("deferredTexture", frameBuffer->GetAttachment(0));
		}

		// Bloom texture.
		if (graphicsSettings.bloom.isEnabled)
		{
			const std::shared_ptr<FrameBuffer> frameBuffer = renderInfo.renderTarget->GetFrameBuffer("BloomFrameBuffers(0)");
			renderUniformWriter->WriteTexture("bloomTexture", frameBuffer->GetAttachment(0));
		}
		else
		{
			renderUniformWriter->WriteTexture("bloomTexture", TextureManager::GetInstance().GetBlack());
		}

		const glm::vec2 viewportSize = renderInfo.viewportSize;
		const int fxaa = graphicsSettings.postProcess.fxaa;
		const std::shared_ptr<Buffer> postProcessBuffer = renderInfo.renderTarget->GetBuffer("PostProcessBuffer");
		baseMaterial->WriteToBuffer(postProcessBuffer, "PostProcessBuffer", "toneMapperIndex", graphicsSettings.postProcess.toneMapper);
		baseMaterial->WriteToBuffer(postProcessBuffer, "PostProcessBuffer", "gamma", graphicsSettings.postProcess.gamma);
		baseMaterial->WriteToBuffer(postProcessBuffer, "PostProcessBuffer", "viewportSize", viewportSize);
		baseMaterial->WriteToBuffer(postProcessBuffer, "PostProcessBuffer", "fxaa", fxaa);
		postProcessBuffer->Flush();

		std::vector<std::shared_ptr<UniformWriter>> uniformWriters = GetUniformWriters(pipeline, baseMaterial, nullptr, renderInfo);

		for (const auto& uniformWriter : uniformWriters)
		{
			uniformWriter->Flush();

			for (const auto& [location, buffer] : uniformWriter->GetBuffersByLocation())
			{
				buffer->Flush();
			}
		}

		const std::shared_ptr<FrameBuffer> frameBuffer = renderInfo.renderTarget->GetFrameBuffer(renderPassName);
		RenderPass::SubmitInfo submitInfo{};
		submitInfo.frame = renderInfo.frame;
		submitInfo.renderPass = renderInfo.renderPass;
		submitInfo.frameBuffer = frameBuffer;
		renderInfo.renderer->BeginRenderPass(submitInfo);

		renderInfo.renderer->Render(
			GetVertexBuffers(pipeline, plane),
			plane->GetIndexBuffer(),
			plane->GetIndexCount(),
			pipeline,
			nullptr,
			0,
			1,
			uniformWriters,
			renderInfo.frame);

		renderInfo.renderer->EndRenderPass(submitInfo);
	};

	Create(createInfo);
}

void RenderPassManager::CreateSSAO()
{
	glm::vec4 clearColor = { 1.0f, 1.0f, 1.0f, 1.0f };

	RenderPass::AttachmentDescription color{};
	color.format = Format::R8G8B8A8_SRGB;
	color.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;
	color.load = RenderPass::Load::LOAD;
	color.store = RenderPass::Store::STORE;

	RenderPass::CreateInfo createInfo{};
	createInfo.type = SSAO;
	createInfo.clearColors = { clearColor };
	createInfo.attachmentDescriptions = { color };
	createInfo.resizeWithViewport = true;
	createInfo.resizeViewportScale = { 0.75f, 0.75f };

	const std::shared_ptr<Mesh> planeMesh = nullptr;

	createInfo.renderCallback = [this](const RenderPass::RenderCallbackInfo& renderInfo)
	{
		const std::string renderPassName = renderInfo.renderPass->GetType();
		std::shared_ptr<FrameBuffer> frameBuffer = renderInfo.renderTarget->GetFrameBuffer(renderPassName);

		const GraphicsSettings::SSAO& ssaoSettings = renderInfo.scene->GetGraphicsSettings().ssao;
		if (!ssaoSettings.isEnabled)
		{
			renderInfo.renderTarget->DeleteFrameBuffer(renderPassName);

			return;
		}
		else
		{
			if (!frameBuffer)
			{
				frameBuffer = FrameBuffer::Create(renderInfo.renderPass, renderInfo.renderTarget.get(), renderInfo.viewportSize);
				renderInfo.renderTarget->SetFrameBuffer(renderPassName, frameBuffer);
			}
		}

		RenderPass::SubmitInfo submitInfo{};
		submitInfo.frame = renderInfo.frame;
		submitInfo.renderPass = renderInfo.renderPass;
		submitInfo.frameBuffer = frameBuffer;
		renderInfo.renderer->BeginRenderPass(submitInfo);

		const std::shared_ptr<Mesh> plane = MeshManager::GetInstance().LoadMesh("FullScreenQuad");
		if (!plane)
		{
			renderInfo.renderer->EndRenderPass(submitInfo);
			return;
		}

		const std::shared_ptr<BaseMaterial> baseMaterial = MaterialManager::GetInstance().LoadBaseMaterial("Materials\\SSAO.basemat");
		const std::shared_ptr<Pipeline> pipeline = baseMaterial->GetPipeline(renderPassName);
		if (!pipeline)
		{
			renderInfo.renderer->EndRenderPass(submitInfo);
			return;
		}

		m_SSAORenderer.GenerateSamples(ssaoSettings.kernelSize);

		auto callback = [this, ssaoSettings]()
		{
			m_SSAORenderer.GenerateNoiseTexture(ssaoSettings.noiseSize);
		};

		std::shared_ptr<NextFrameEvent> generateNoiseTextureEvent = std::make_shared<NextFrameEvent>(callback, Event::Type::OnNextFrame, this);
		EventSystem::GetInstance().SendEvent(generateNoiseTextureEvent);

		constexpr float resolutionScales[] = { 0.25f, 0.5f, 0.75f, 1.0f };

		if (renderInfo.renderPass->GetResizeViewportScale() != glm::vec2(resolutionScales[ssaoSettings.resolutionScale]))
		{
			auto callback = [resolutionScales, frameBuffer, ssaoSettings, renderInfo]()
			{
				renderInfo.renderPass->SetResizeViewportScale(glm::vec2(resolutionScales[ssaoSettings.resolutionScale]));
				frameBuffer->Resize(renderInfo.viewportSize);
			};

			std::shared_ptr<NextFrameEvent> resizeEvent = std::make_shared<NextFrameEvent>(callback, Event::Type::OnNextFrame, this);
			EventSystem::GetInstance().SendEvent(resizeEvent);
		}

		std::shared_ptr<UniformWriter> renderUniformWriter = renderInfo.renderTarget->GetUniformWriter(renderPassName);
		if (!renderUniformWriter)
		{
			std::shared_ptr<UniformLayout> renderUniformLayout =
				pipeline->GetUniformLayout(*pipeline->GetDescriptorSetIndexByType(Pipeline::DescriptorSetIndexType::RENDERER, renderPassName));
			renderUniformWriter = UniformWriter::Create(renderUniformLayout);
			renderInfo.renderTarget->SetUniformWriter(renderPassName, renderUniformWriter);

			for (const auto& binding : renderUniformLayout->GetBindings())
			{
				if (binding.buffer && binding.buffer->name == "SSAOBuffer")
				{
					const std::shared_ptr<Buffer> buffer = Buffer::Create(
						binding.buffer->size,
						1,
						Buffer::Usage::UNIFORM_BUFFER,
						Buffer::MemoryType::CPU);

					renderInfo.renderTarget->SetBuffer("SSAOBuffer", buffer);
					renderUniformWriter->WriteBuffer(binding.buffer->name, buffer);
					renderUniformWriter->Flush();

					break;
				}
			}
		}

		for (const auto& [name, renderTargetInfo] : pipeline->GetCreateInfo().uniformInfo.renderTargetsByName)
		{
			const std::shared_ptr<FrameBuffer> frameBuffer = renderInfo.renderTarget->GetFrameBuffer(renderTargetInfo.renderPassName);
			renderUniformWriter->WriteTexture(name, frameBuffer->GetAttachment(renderTargetInfo.attachmentIndex));
		}

		const std::shared_ptr<Texture> noiseTexture = m_SSAORenderer.GetNoiseTexture();
		if (!noiseTexture)
		{
			renderInfo.renderer->EndRenderPass(submitInfo);
			return;
		}
		
		renderUniformWriter->WriteTexture("noiseTexture", noiseTexture);

		const std::shared_ptr<Buffer> ssaoBuffer = renderInfo.renderTarget->GetBuffer("SSAOBuffer");
		baseMaterial->WriteToBuffer(ssaoBuffer, "SSAOBuffer", "viewportScale", GetRenderPass(renderPassName)->GetResizeViewportScale());
		baseMaterial->WriteToBuffer(ssaoBuffer, "SSAOBuffer", "kernelSize", ssaoSettings.kernelSize);
		baseMaterial->WriteToBuffer(ssaoBuffer, "SSAOBuffer", "noiseSize", ssaoSettings.noiseSize);
		baseMaterial->WriteToBuffer(ssaoBuffer, "SSAOBuffer", "aoScale", ssaoSettings.aoScale);
		baseMaterial->WriteToBuffer(ssaoBuffer, "SSAOBuffer", "samples", m_SSAORenderer.GetSamples());
		baseMaterial->WriteToBuffer(ssaoBuffer, "SSAOBuffer", "radius", ssaoSettings.radius);
		baseMaterial->WriteToBuffer(ssaoBuffer, "SSAOBuffer", "bias", ssaoSettings.bias);

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
			GetVertexBuffers(pipeline, plane),
			plane->GetIndexBuffer(),
			plane->GetIndexCount(),
			pipeline,
			nullptr,
			0,
			1,
			uniformWriters,
			renderInfo.frame);

		renderInfo.renderer->EndRenderPass(submitInfo);
	};

	Create(createInfo);
}

void RenderPassManager::CreateSSAOBlur()
{
	glm::vec4 clearColor = { 1.0f, 1.0f, 1.0f, 1.0f };

	RenderPass::AttachmentDescription color{};
	color.format = Format::R8G8B8A8_SRGB;
	color.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;
	color.load = RenderPass::Load::LOAD;
	color.store = RenderPass::Store::STORE;

	RenderPass::CreateInfo createInfo{};
	createInfo.type = SSAOBlur;
	createInfo.clearColors = { clearColor };
	createInfo.attachmentDescriptions = { color };
	createInfo.resizeWithViewport = true;
	createInfo.resizeViewportScale = { 0.75f, 0.75f };

	const std::shared_ptr<Mesh> planeMesh = nullptr;

	createInfo.renderCallback = [this](const RenderPass::RenderCallbackInfo& renderInfo)
	{
		const std::string renderPassName = renderInfo.renderPass->GetType();
		std::shared_ptr<FrameBuffer> frameBuffer = renderInfo.renderTarget->GetFrameBuffer(renderPassName);

		const GraphicsSettings::SSAO& ssaoSettings = renderInfo.scene->GetGraphicsSettings().ssao;
		if (!ssaoSettings.isEnabled)
		{
			renderInfo.renderTarget->DeleteFrameBuffer(renderPassName);

			return;
		}
		else
		{
			if (!frameBuffer)
			{
				frameBuffer = FrameBuffer::Create(renderInfo.renderPass, renderInfo.renderTarget.get(), renderInfo.viewportSize);
				renderInfo.renderTarget->SetFrameBuffer(renderPassName, frameBuffer);
			}
		}

		RenderPass::SubmitInfo submitInfo{};
		submitInfo.frame = renderInfo.frame;
		submitInfo.renderPass = renderInfo.renderPass;
		submitInfo.frameBuffer = frameBuffer;
		renderInfo.renderer->BeginRenderPass(submitInfo);

		const std::shared_ptr<Mesh> plane = MeshManager::GetInstance().LoadMesh("FullScreenQuad");
		if (!plane)
		{
			renderInfo.renderer->EndRenderPass(submitInfo);
			return;
		}

		const std::shared_ptr<BaseMaterial> baseMaterial = MaterialManager::GetInstance().LoadBaseMaterial("Materials\\SSAOBlur.basemat");
		const std::shared_ptr<Pipeline> pipeline = baseMaterial->GetPipeline(renderPassName);
		if (!pipeline)
		{
			renderInfo.renderer->EndRenderPass(submitInfo);
			return;
		}

		std::shared_ptr<UniformWriter> renderUniformWriter = renderInfo.renderTarget->GetUniformWriter(renderPassName);
		if (!renderUniformWriter)
		{
			std::shared_ptr<UniformLayout> renderUniformLayout =
				pipeline->GetUniformLayout(*pipeline->GetDescriptorSetIndexByType(Pipeline::DescriptorSetIndexType::RENDERER, renderPassName));
			renderUniformWriter = UniformWriter::Create(renderUniformLayout);
			renderInfo.renderTarget->SetUniformWriter(renderPassName, renderUniformWriter);
		}

		for (const auto& [name, renderTargetInfo] : pipeline->GetCreateInfo().uniformInfo.renderTargetsByName)
		{
			const std::shared_ptr<FrameBuffer> frameBuffer = renderInfo.renderTarget->GetFrameBuffer(renderTargetInfo.renderPassName);
			renderUniformWriter->WriteTexture(name, frameBuffer->GetAttachment(renderTargetInfo.attachmentIndex));
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
			GetVertexBuffers(pipeline, plane),
			plane->GetIndexBuffer(),
			plane->GetIndexCount(),
			pipeline,
			nullptr,
			0,
			1,
			uniformWriters,
			renderInfo.frame);

		renderInfo.renderer->EndRenderPass(submitInfo);
	};

	Create(createInfo);
}

void RenderPassManager::CreateCSM()
{
	RenderPass::ClearDepth clearDepth{};
	clearDepth.clearDepth = 1.0f;
	clearDepth.clearStencil = 0;

	RenderPass::AttachmentDescription depth{};
	depth.format = Format::D32_SFLOAT;
	depth.layout = Texture::Layout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	Texture::SamplerCreateInfo samplerCreateInfo{};
	samplerCreateInfo.filter = Texture::SamplerCreateInfo::Filter::NEAREST;
	samplerCreateInfo.borderColor = Texture::SamplerCreateInfo::BorderColor::FLOAT_OPAQUE_WHITE;
	samplerCreateInfo.addressMode = Texture::SamplerCreateInfo::AddressMode::CLAMP_TO_EDGE;
	samplerCreateInfo.maxAnisotropy = 1.0f;

	depth.samplerCreateInfo = samplerCreateInfo;

	RenderPass::CreateInfo createInfo{};
	createInfo.type = CSM;
	createInfo.clearDepths = { clearDepth };
	createInfo.attachmentDescriptions = { depth };
	createInfo.resizeWithViewport = false;
	createInfo.createFrameBuffer = false;

	const std::shared_ptr<Mesh> planeMesh = nullptr;

	createInfo.renderCallback = [this](const RenderPass::RenderCallbackInfo& renderInfo)
	{
		const std::string renderPassName = renderInfo.renderPass->GetType();

		const GraphicsSettings::Shadows& shadowsSettings = renderInfo.scene->GetGraphicsSettings().shadows;
		if (!shadowsSettings.isEnabled)
		{
			renderInfo.renderTarget->DeleteFrameBuffer(renderPassName);

			return;
		}

		glm::ivec2 resolutions[3] = { { 1024, 1024 }, { 2048, 2048 }, { 4096, 4096 } };

		std::shared_ptr<FrameBuffer> frameBuffer = renderInfo.renderTarget->GetFrameBuffer(renderPassName);
		if (!frameBuffer)
		{
			// NOTE: Maybe should be send as the next frame event, because creating a frame buffer here may cause some problems.
			const std::string renderPassName = renderInfo.renderPass->GetType();
			renderInfo.renderPass->GetAttachmentDescriptions().back().layercount = renderInfo.scene->GetGraphicsSettings().shadows.cascadeCount;
			frameBuffer = FrameBuffer::Create(renderInfo.renderPass, renderInfo.renderTarget.get(), resolutions[shadowsSettings.quality]);

			renderInfo.renderTarget->SetFrameBuffer(renderPassName, frameBuffer);
		}

		// Recreate if quality has been changed.
		if (frameBuffer->GetSize() != resolutions[shadowsSettings.quality])
		{
			auto callback = [frameBuffer, resolution = resolutions[shadowsSettings.quality]]()
			{
				frameBuffer->Resize(resolution);
			};

			std::shared_ptr<NextFrameEvent> event = std::make_shared<NextFrameEvent>(callback, Event::Type::OnNextFrame, this);
			EventSystem::GetInstance().SendEvent(event);
		}

		if (!shadowsSettings.isEnabled)
		{
			return;
		}

		using EntitiesByMesh = std::unordered_map<std::shared_ptr<Mesh>, std::vector<entt::entity>>;
		using MeshesByMaterial = std::unordered_map<std::shared_ptr<Material>, EntitiesByMesh>;
		using MaterialByBaseMaterial = std::unordered_map<std::shared_ptr<BaseMaterial>, MeshesByMaterial>;

		MaterialByBaseMaterial materialMeshGameObjects;

		size_t renderableCount = 0;
		const std::shared_ptr<Scene> scene = renderInfo.scene;
		const Camera& camera = renderInfo.camera->GetComponent<Camera>();
		const entt::registry& registry = scene->GetRegistry();
		const auto r3dView = registry.view<Renderer3D>();

		glm::vec3 lightDirection{};
		auto directionalLightView = renderInfo.scene->GetRegistry().view<DirectionalLight>();
		if (directionalLightView.empty())
		{
			return;
		}

		{
			const entt::entity& entity = directionalLightView.back();
			DirectionalLight& dl = renderInfo.scene->GetRegistry().get<DirectionalLight>(entity);
			const Transform& transform = renderInfo.scene->GetRegistry().get<Transform>(entity);
			lightDirection = transform.GetForward();
		}

		for (const entt::entity& entity : r3dView)
		{
			const Renderer3D& r3d = registry.get<Renderer3D>(entity);
			const Transform& transform = registry.get<Transform>(entity);
			if (!transform.GetEntity()->IsEnabled())
			{
				continue;
			}

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

		struct InstanceDataCSM
		{
			glm::mat4 transform;
		};

		std::shared_ptr<Buffer> instanceBuffer = renderInfo.renderTarget->GetBuffer("InstanceBufferCSM");
		if ((renderableCount != 0 && !instanceBuffer) || (instanceBuffer && renderableCount != 0 && instanceBuffer->GetInstanceCount() != renderableCount))
		{
			instanceBuffer = Buffer::Create(
				sizeof(InstanceDataCSM),
				renderableCount,
				Buffer::Usage::VERTEX_BUFFER,
				Buffer::MemoryType::CPU);

			renderInfo.renderTarget->SetBuffer("InstanceBufferCSM", instanceBuffer);
		}

		std::vector<InstanceDataCSM> instanceDatas;

		bool updatedLightSpaceMatrices = false;

		RenderPass::SubmitInfo submitInfo{};
		submitInfo.frame = renderInfo.frame;
		submitInfo.renderPass = renderInfo.renderPass;
		submitInfo.frameBuffer = frameBuffer;
		renderInfo.renderer->BeginRenderPass(submitInfo);

		// Render all base materials -> materials -> meshes | put gameobjects into the instance buffer.
		for (const auto& [baseMaterial, meshesByMaterial] : materialMeshGameObjects)
		{
			const std::shared_ptr<Pipeline> pipeline = baseMaterial->GetPipeline(renderPassName);

			std::shared_ptr<UniformWriter> renderUniformWriter = renderInfo.renderTarget->GetUniformWriter(renderPassName);
			if (!renderUniformWriter)
			{
				std::shared_ptr<UniformLayout> renderUniformLayout =
					pipeline->GetUniformLayout(*pipeline->GetDescriptorSetIndexByType(Pipeline::DescriptorSetIndexType::RENDERER, renderPassName));
				renderUniformWriter = UniformWriter::Create(renderUniformLayout);
				renderInfo.renderTarget->SetUniformWriter(renderPassName, renderUniformWriter);

				for (const auto& binding : renderUniformLayout->GetBindings())
				{
					if (binding.buffer && binding.buffer->name == "LightSpaceMatrices")
					{
						const std::shared_ptr<Buffer> buffer = Buffer::Create(
							binding.buffer->size,
							1,
							Buffer::Usage::UNIFORM_BUFFER,
							Buffer::MemoryType::CPU);

						renderInfo.renderTarget->SetBuffer("LightSpaceMatrices", buffer);
						renderUniformWriter->WriteBuffer(binding.buffer->name, buffer);
						renderUniformWriter->Flush();

						break;
					}
				}
			}

			if (!updatedLightSpaceMatrices)
			{
				updatedLightSpaceMatrices = true;
				// TODO: Camera can be ortho, so need to make for ortho as well.
				const glm::mat4 projection = glm::perspective(
					camera.GetFov(),
					(float)renderInfo.viewportSize.x / (float)renderInfo.viewportSize.y,
					camera.GetZNear(),
					shadowsSettings.maxDistance);

				CSMRenderer& csmRenderer = m_CSMRenderersByCSMSetting[renderInfo.scene->GetGraphicsSettings().GetFilepath().wstring()];
				const bool recreateFrameBuffer = csmRenderer.GenerateLightSpaceMatrices(
					projection * camera.GetViewMat4(),
					lightDirection,
					camera.GetZNear(),
					shadowsSettings.maxDistance,
					shadowsSettings.cascadeCount,
					shadowsSettings.splitFactor);

				baseMaterial->WriteToBuffer(
					renderInfo.renderTarget->GetBuffer("LightSpaceMatrices"),
					"LightSpaceMatrices",
					"lightSpaceMatrices",
					*csmRenderer.GetLightSpaceMatrices().data());

				const int cascadeCount = csmRenderer.GetLightSpaceMatrices().size();
				baseMaterial->WriteToBuffer(
					renderInfo.renderTarget->GetBuffer("LightSpaceMatrices"),
					"LightSpaceMatrices",
					"cascadeCount",
					cascadeCount);

				// Recreate if layer count has been changed.
				if (recreateFrameBuffer)
				{
					auto callback = [this, submitInfo, renderInfo, cascadeCount]()
					{
						submitInfo.frameBuffer->GetAttachmentCreateInfos().back().layerCount = cascadeCount;
						submitInfo.frameBuffer->Resize(submitInfo.frameBuffer->GetSize());
					};

					std::shared_ptr<NextFrameEvent> event = std::make_shared<NextFrameEvent>(callback, Event::Type::OnNextFrame, this);
					EventSystem::GetInstance().SendEvent(event);
				}
			}
			
			for (const auto& [material, gameObjectsByMeshes] : meshesByMaterial)
			{
				for (const auto& [mesh, entities] : gameObjectsByMeshes)
				{
					const size_t instanceDataOffset = instanceDatas.size();

					for (const entt::entity& entity : entities)
					{
						InstanceDataCSM data{};
						const Transform& transform = registry.get<Transform>(entity);
						data.transform = transform.GetTransform();
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
						GetVertexBuffers(pipeline, mesh),
						mesh->GetIndexBuffer(),
						mesh->GetIndexCount(),
						pipeline,
						instanceBuffer,
						instanceDataOffset * instanceBuffer->GetInstanceSize(),
						entities.size(),
						uniformWriters,
						renderInfo.frame);
				}
			}
		}

		// Because these are all just commands and will be rendered later we can write the instance buffer
		// just once when all instance data is collected.
		if (instanceBuffer && !instanceDatas.empty())
		{
			instanceBuffer->WriteToBuffer(instanceDatas.data(), instanceDatas.size() * sizeof(InstanceDataCSM));
		}

		renderInfo.renderer->EndRenderPass(submitInfo);
	};

	Create(createInfo);
}

void RenderPassManager::CreateBloom()
{
	glm::vec4 clearColor = { 1.0f, 1.0f, 1.0f, 1.0f };

	RenderPass::AttachmentDescription color{};
	color.format = Format::B10G11R11_UFLOAT_PACK32;
	color.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;
	color.load = RenderPass::Load::LOAD;
	color.store = RenderPass::Store::STORE;

	Texture::SamplerCreateInfo samplerCreateInfo{};
	samplerCreateInfo.addressMode = Texture::SamplerCreateInfo::AddressMode::CLAMP_TO_BORDER;
	samplerCreateInfo.borderColor = Texture::SamplerCreateInfo::BorderColor::FLOAT_OPAQUE_BLACK;
	samplerCreateInfo.maxAnisotropy = 1.0f;

	color.samplerCreateInfo = samplerCreateInfo;

	RenderPass::CreateInfo createInfo{};
	createInfo.type = Bloom;
	createInfo.clearColors = { clearColor };
	createInfo.attachmentDescriptions = { color };
	createInfo.resizeWithViewport = false;
	createInfo.createFrameBuffer = false;
	
	const std::shared_ptr<Mesh> planeMesh = nullptr;

	createInfo.renderCallback = [this](const RenderPass::RenderCallbackInfo& renderInfo)
	{
		const std::string renderPassName = renderInfo.renderPass->GetType();
		const GraphicsSettings::Bloom& bloomSettings = renderInfo.scene->GetGraphicsSettings().bloom;
		const int mipCount = bloomSettings.mipCount;

		if (!bloomSettings.isEnabled)
		{
			renderInfo.renderTarget->DeleteFrameBuffer(renderPassName);
			for (int mipLevel = 0; mipLevel < mipCount; mipLevel++)
			{
				renderInfo.renderTarget->DeleteFrameBuffer("BloomFrameBuffers(" + std::to_string(mipLevel) + ")");
			}

			return;
		}

		const std::shared_ptr<Mesh> plane = MeshManager::GetInstance().LoadMesh("FullScreenQuad");
		if (!plane)
		{
			return;
		}

		// Used to store unique view (camera) bloom information to compare with current graphics settings
		// and in case if not equal then recreate resources.
		struct BloomData
		{
			int mipCount = 0;
			glm::ivec2 sourceSize = { 0, 0 };
		};

		bool recreateResources = false;
		
		BloomData* bloomData = (BloomData*)renderInfo.renderTarget->GetCustomData("BloomData");
		if (!bloomData)
		{
			bloomData = new BloomData();
			bloomData->mipCount = mipCount;
			bloomData->sourceSize = renderInfo.viewportSize;
			renderInfo.renderTarget->SetCustomData("BloomData", bloomData);
		
			recreateResources = true;
		}
		else
		{
			recreateResources = bloomData->mipCount != mipCount;
			bloomData->mipCount = mipCount;
		}

		renderInfo.renderer->BeginCommandLabel("Bloom", topLevelRenderPassDebugColor, renderInfo.frame);
		// Down Sample.
		{
			const std::shared_ptr<BaseMaterial> baseMaterial = MaterialManager::GetInstance().LoadBaseMaterial("Materials\\BloomDownSample.basemat");
			const std::shared_ptr<Pipeline> pipeline = baseMaterial->GetPipeline(renderPassName);
			if (!pipeline)
			{
				return;
			}

			// Create FrameBuffers.
			if (recreateResources || !renderInfo.renderTarget->GetFrameBuffer(renderPassName))
			{
				glm::ivec2 size = renderInfo.viewportSize;
				for (int mipLevel = 0; mipLevel < mipCount; mipLevel++)
				{
					const std::string mipLevelString = std::to_string(mipLevel);

					// NOTE: There are parentheses in the name because square brackets are used in the base material file to find the attachment index.
					renderInfo.renderTarget->SetFrameBuffer("BloomFrameBuffers(" + mipLevelString + ")", FrameBuffer::Create(renderInfo.renderPass, renderInfo.renderTarget.get(), size));

					size.x = glm::max(size.x / 2, 1);
					size.y = glm::max(size.y / 2, 1);
				}

				// For viewport visualization and easy access by render pass name.
				renderInfo.renderTarget->SetFrameBuffer(renderPassName, renderInfo.renderTarget->GetFrameBuffer("BloomFrameBuffers(0)"));
			}

			// Create UniformWriters, Buffers for down sample pass.
			if (recreateResources)
			{
				for (int mipLevel = 0; mipLevel < mipCount; mipLevel++)
				{
					const std::string mipLevelString = std::to_string(mipLevel);

					const std::shared_ptr<UniformLayout> renderUniformLayout =
						pipeline->GetUniformLayout(*pipeline->GetDescriptorSetIndexByType(Pipeline::DescriptorSetIndexType::RENDERER, renderPassName));
					const std::shared_ptr<UniformWriter> renderUniformWriter = UniformWriter::Create(renderUniformLayout);
					renderInfo.renderTarget->SetUniformWriter("BloomDownUniformWriters[" + mipLevelString + "]", renderUniformWriter);

					const std::shared_ptr<Buffer> buffer = Buffer::Create(
						renderUniformLayout->GetBindingByName("MipBuffer")->buffer->size,
						1,
						Buffer::Usage::UNIFORM_BUFFER,
						Buffer::MemoryType::CPU);
					renderInfo.renderTarget->SetBuffer("BloomBuffers[" + mipLevelString + "]", buffer);

					renderUniformWriter->WriteBuffer("MipBuffer", buffer);
				}
			}
			
			// Resize.
			if (bloomData->sourceSize.x != renderInfo.viewportSize.x ||
				bloomData->sourceSize.y != renderInfo.viewportSize.y)
			{
				glm::ivec2 size = renderInfo.viewportSize;
				bloomData->sourceSize = size;
				for (size_t mipLevel = 0; mipLevel < mipCount; mipLevel++)
				{
					const std::shared_ptr<FrameBuffer> frameBuffer = renderInfo.renderTarget->GetFrameBuffer("BloomFrameBuffers(" + std::to_string(mipLevel) + ")");
					frameBuffer->Resize(size);

					size.x = glm::max(size.x / 2, 1);
					size.y = glm::max(size.y / 2, 1);
				}
			}

			std::shared_ptr<Texture> sourceTexture = renderInfo.renderTarget->GetFrameBuffer(Deferred)->GetAttachment(1);
			for (int mipLevel = 0; mipLevel < mipCount; mipLevel++)
			{
				const std::string mipLevelString = std::to_string(mipLevel);

				renderInfo.renderer->MemoryBarrierFragmentReadWrite(renderInfo.frame);

				const std::shared_ptr<FrameBuffer> frameBuffer = renderInfo.renderTarget->GetFrameBuffer("BloomFrameBuffers(" + mipLevelString + ")");

				RenderPass::SubmitInfo submitInfo{};
				submitInfo.frame = renderInfo.frame;
				submitInfo.renderPass = renderInfo.renderPass;
				submitInfo.frameBuffer = frameBuffer;
				renderInfo.renderer->BeginRenderPass(submitInfo, "Bloom Down Sample [" + mipLevelString + "]", { 1.0f, 1.0f, 0.0f });

				glm::vec2 sourceSize = submitInfo.frameBuffer->GetSize();
				const std::shared_ptr<Buffer> mipBuffer = renderInfo.renderTarget->GetBuffer("BloomBuffers[" + mipLevelString + "]");
				baseMaterial->WriteToBuffer(mipBuffer, "MipBuffer", "sourceSize", sourceSize);
				mipBuffer->Flush();

				const std::shared_ptr<UniformWriter> downUniformWriter = renderInfo.renderTarget->GetUniformWriter("BloomDownUniformWriters[" + mipLevelString + "]");
				downUniformWriter->WriteTexture("sourceTexture", sourceTexture);
				downUniformWriter->Flush();

				renderInfo.renderer->Render(
					GetVertexBuffers(pipeline, plane),
					plane->GetIndexBuffer(),
					plane->GetIndexCount(),
					pipeline,
					nullptr,
					0,
					1,
					{
						downUniformWriter
					},
					renderInfo.frame);

				renderInfo.renderer->EndRenderPass(submitInfo);

				sourceTexture = frameBuffer->GetAttachment(0);
			}
		}

		// Up Sample.
		{
			const std::shared_ptr<BaseMaterial> baseMaterial = MaterialManager::GetInstance().LoadBaseMaterial("Materials\\BloomUpSample.basemat");
			const std::shared_ptr<Pipeline> pipeline = baseMaterial->GetPipeline(renderPassName);
			if (!pipeline)
			{
				return;
			}

			// Create UniformWriters for up sample pass.
			if (recreateResources)
			{
				for (int mipLevel = 0; mipLevel < mipCount - 1; mipLevel++)
				{
					const std::string mipLevelString = std::to_string(mipLevel);

					const std::shared_ptr<UniformLayout> renderUniformLayout =
						pipeline->GetUniformLayout(*pipeline->GetDescriptorSetIndexByType(Pipeline::DescriptorSetIndexType::RENDERER, renderPassName));
					renderInfo.renderTarget->SetUniformWriter("BloomUpUniformWriters[" + mipLevelString + "]", UniformWriter::Create(renderUniformLayout));
				}
			}

			for (int mipLevel = mipCount - 1; mipLevel > 0; mipLevel--)
			{
				const std::string mipLevelString = std::to_string(mipLevel - 1);

				renderInfo.renderer->MemoryBarrierFragmentReadWrite(renderInfo.frame);

				RenderPass::SubmitInfo submitInfo{};
				submitInfo.frame = renderInfo.frame;
				submitInfo.renderPass = renderInfo.renderPass;
				submitInfo.frameBuffer = renderInfo.renderTarget->GetFrameBuffer("BloomFrameBuffers(" + mipLevelString + ")");
				renderInfo.renderer->BeginRenderPass(submitInfo, "Bloom Up Sample [" + std::to_string(mipLevel) + "]", { 1.0f, 1.0f, 0.0f });

				const std::shared_ptr<UniformWriter> downUniformWriter = renderInfo.renderTarget->GetUniformWriter("BloomUpUniformWriters[" + mipLevelString + "]");
				downUniformWriter->WriteTexture("sourceTexture", renderInfo.renderTarget->GetFrameBuffer("BloomFrameBuffers(" + std::to_string(mipLevel) + ")")->GetAttachment(0));
				downUniformWriter->Flush();

				renderInfo.renderer->Render(
					GetVertexBuffers(pipeline, plane),
					plane->GetIndexBuffer(),
					plane->GetIndexCount(),
					pipeline,
					nullptr,
					0,
					1,
					{
						downUniformWriter
					},
					renderInfo.frame);

				renderInfo.renderer->EndRenderPass(submitInfo);
			}
		}
		renderInfo.renderer->EndCommandLabel(renderInfo.frame);
	};

	Create(createInfo);
}
