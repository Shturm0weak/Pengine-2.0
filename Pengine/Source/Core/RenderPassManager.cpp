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

	FATAL_ERROR(type + " id of render pass doesn't exist!");

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

RenderPassManager::RenderPassManager()
{
	CreateDefaultReflection();
	CreateGBuffer();
	CreateDeferred();
	CreateAtmosphere();
	CreateTransparent();
	CreateFinal();
	CreateSSAO();
	CreateSSAOBlur();
	CreateCSM();
}

void RenderPassManager::CreateGBuffer()
{
	RenderPass::ClearDepth clearDepth{};
	clearDepth.clearDepth = 1.0f;
	clearDepth.clearStencil = 0;

	glm::vec4 clearColor = { 0.4f, 0.4f, 0.4f, 1.0f };
	glm::vec4 clearNormal = { 0.0f, 0.0f, 0.0f, 0.0f };
	glm::vec4 clearShading = { 0.0f, 0.0f, 0.0f, 0.0f };

	RenderPass::AttachmentDescription color{};
	color.format = Format::R8G8B8A8_SRGB;
	color.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;

	RenderPass::AttachmentDescription normal{};
	normal.format = Format::R32G32B32A32_SFLOAT;
	normal.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;

	RenderPass::AttachmentDescription shading{};
	shading.format = Format::R8G8B8A8_SRGB;
	shading.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;

	RenderPass::AttachmentDescription depth{};
	depth.format = Format::D32_SFLOAT;
	depth.layout = Texture::Layout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	
	Texture::SamplerCreateInfo depthSamplerCreateInfo{};
	depthSamplerCreateInfo.addressMode = Texture::SamplerCreateInfo::AddressMode::CLAMP_TO_BORDER;
	depthSamplerCreateInfo.borderColor = Texture::SamplerCreateInfo::BorderColor::FLOAT_OPAQUE_WHITE;

	depth.samplerCreateInfo = depthSamplerCreateInfo;

	RenderPass::CreateInfo createInfo{};
	createInfo.type = GBuffer;
	createInfo.clearColors = { clearColor, clearNormal, clearShading };
	createInfo.clearDepths = { clearDepth };
	createInfo.attachmentDescriptions = { color, normal, shading, depth };
	createInfo.resizeWithViewport = true;
	createInfo.resizeViewportScale = { 1.0f, 1.0f };

	createInfo.renderCallback = [this](const RenderPass::RenderCallbackInfo& renderInfo)
	{
		const std::string globalBufferName = "GlobalBuffer";
		const std::shared_ptr<BaseMaterial> reflectionBaseMaterial = MaterialManager::GetInstance().LoadBaseMaterial("Materials\\DefaultReflection.basemat");

		const std::string renderPassName = renderInfo.renderPass->GetType();
		const std::shared_ptr<FrameBuffer> frameBuffer = renderInfo.renderer->GetRenderPassFrameBuffer(renderPassName);
		const glm::vec2 viewportSize = frameBuffer->GetSize();
		reflectionBaseMaterial->WriteToBuffer(
			renderInfo.renderer->GetBuffer(globalBufferName),
			globalBufferName,
			"camera.viewportSize",
			viewportSize);

		const float aspectRation = viewportSize.x / viewportSize.y;
		reflectionBaseMaterial->WriteToBuffer(
			renderInfo.renderer->GetBuffer(globalBufferName),
			globalBufferName,
			"camera.aspectRatio",
			aspectRation);

		Camera& camera = renderInfo.camera->GetComponent<Camera>();
		const float tanHalfFOV = tanf(camera.GetFov() / 2.0f);
		reflectionBaseMaterial->WriteToBuffer(
			renderInfo.renderer->GetBuffer(globalBufferName),
			globalBufferName,
			"camera.tanHalfFOV",
			tanHalfFOV);

		using EntitiesByMesh = std::unordered_map<std::shared_ptr<Mesh>, std::vector<entt::entity>>;
		using MeshesByMaterial = std::unordered_map<std::shared_ptr<Material>, EntitiesByMesh>;
		using MaterialByBaseMaterial = std::unordered_map<std::shared_ptr<BaseMaterial>, MeshesByMaterial>;

		MaterialByBaseMaterial materialMeshGameObjects;

		size_t renderableCount = 0;
		const std::shared_ptr<Scene> scene = renderInfo.scene;
		const entt::registry& registry = scene->GetRegistry();
		const auto r3dView = registry.view<Renderer3D>();
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

			if (scene->GetSettings().m_DrawBoundingBoxes)
			{
				const glm::mat4& transformMat4 = transform.GetTransform();
				const BoundingBox& box = r3d.mesh->GetBoundingBox();
				const glm::vec3 color = glm::vec3(0.0f, 1.0f, 0.0f);

				scene->GetVisualizer().DrawBox(box.min, box.max, color, transformMat4);
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

		RenderPass::SubmitInfo submitInfo{};
		submitInfo.frame = renderInfo.frame;
		submitInfo.renderPass = renderInfo.renderPass;
		submitInfo.frameBuffer = frameBuffer;
		renderInfo.renderer->BeginRenderPass(submitInfo);

		// Render all base materials -> materials -> meshes | put gameobjects into the instance buffer.
		for (const auto& [baseMaterial, meshesByMaterial] : materialMeshGameObjects)
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
						InstanceData data{};
						const Transform& transform = registry.get<Transform>(entity);
						data.transform = transform.GetTransform();
						data.inverseTransform = glm::transpose(transform.GetInverseTransform());
						instanceDatas.emplace_back(data);
					}

					renderInfo.renderer->Render(
						mesh->GetVertexBuffer(),
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
			instanceBuffer->WriteToBuffer(instanceDatas.data(), instanceDatas.size() * sizeof(InstanceData));
		}

		m_LineRenderer.Render(renderInfo);

		// Render SkyBox.
		{
			std::shared_ptr<Mesh> cubeMesh = MeshManager::GetInstance().LoadMesh("Meshes\\Cube.mesh");
			std::shared_ptr<BaseMaterial> skyBoxBaseMaterial = MaterialManager::GetInstance().LoadBaseMaterial("Materials\\SkyBox.basemat");

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
					cubeMesh->GetVertexBuffer(),
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

	RenderPass::AttachmentDescription color{};
	color.format = Format::B10G11R11_UFLOAT_PACK32;
	color.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;

	RenderPass::CreateInfo createInfo{};
	createInfo.type = Deferred;
	createInfo.clearColors = { clearColor };
	createInfo.attachmentDescriptions = { color };
	createInfo.resizeWithViewport = true;
	createInfo.resizeViewportScale = { 1.0f, 1.0f };

	const std::shared_ptr<Mesh> planeMesh = nullptr;

	createInfo.renderCallback = [this](const RenderPass::RenderCallbackInfo& renderInfo)
	{
		const std::shared_ptr<Mesh> plane = MeshManager::GetInstance().LoadMesh("Meshes\\Plane.mesh");
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

		for (const auto& [name, renderTargetInfo] : pipeline->GetCreateInfo().uniformInfo.renderTargetsByName)
		{
			const std::shared_ptr<FrameBuffer> frameBuffer = renderInfo.renderer->GetRenderPassFrameBuffer(renderTargetInfo.renderPassName);
			renderUniformWriter->WriteTexture(name, frameBuffer->GetAttachment(renderTargetInfo.attachmentIndex));
		}

		const Camera& camera = renderInfo.camera->GetComponent<Camera>();

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

		auto directionalLightView = renderInfo.scene->GetRegistry().view<DirectionalLight>();
		if (!directionalLightView.empty())
		{
			const entt::entity& entity = directionalLightView.back();
			DirectionalLight& dl = renderInfo.scene->GetRegistry().get<DirectionalLight>(entity);
			const Transform& transform = renderInfo.scene->GetRegistry().get<Transform>(entity);

			baseMaterial->WriteToBuffer(lightsBuffer, "Lights", "directionalLight.color", dl.color);
			baseMaterial->WriteToBuffer(lightsBuffer, "Lights", "directionalLight.intensity", dl.intensity);

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

		const std::shared_ptr<FrameBuffer> frameBuffer = renderInfo.renderer->GetRenderPassFrameBuffer(renderPassName);
		RenderPass::SubmitInfo submitInfo{};
		submitInfo.frame = renderInfo.frame;
		submitInfo.renderPass = renderInfo.renderPass;
		submitInfo.frameBuffer = frameBuffer;
		renderInfo.renderer->BeginRenderPass(submitInfo);

		renderInfo.renderer->Render(
			plane->GetVertexBuffer(),
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
	color.size = { 256, 256 };
	color.isCubeMap = true;
	
	RenderPass::CreateInfo createInfo{};
	createInfo.type = Atmosphere;
	createInfo.clearColors = { clearColor };
	createInfo.attachmentDescriptions = { color };
	createInfo.resizeWithViewport = false;

	const std::string globalBufferName = "GlobalBuffer";
	createInfo.renderCallback = [globalBufferName](const RenderPass::RenderCallbackInfo& renderInfo)
	{
		std::shared_ptr<Buffer> globalBuffer = renderInfo.renderer->GetBuffer(globalBufferName);
		const std::shared_ptr<BaseMaterial> reflectionBaseMaterial = MaterialManager::GetInstance().LoadBaseMaterial("Materials\\DefaultReflection.basemat");
		if (!renderInfo.renderer->GetBuffer(globalBufferName))
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
			renderInfo.renderer->SetBuffer(globalBufferName, globalBuffer);
			renderInfo.renderer->SetUniformWriter(DefaultReflection, uniformWriter);
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

		const std::shared_ptr<Mesh> plane = MeshManager::GetInstance().LoadMesh("Meshes\\Plane.mesh");
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

		const std::shared_ptr<FrameBuffer> frameBuffer = renderInfo.renderer->GetRenderPassFrameBuffer(renderPassName);
		RenderPass::SubmitInfo submitInfo{};
		submitInfo.frame = renderInfo.frame;
		submitInfo.renderPass = renderInfo.renderPass;
		submitInfo.frameBuffer = frameBuffer;
		renderInfo.renderer->BeginRenderPass(submitInfo);

		renderInfo.renderer->Render(
			plane->GetVertexBuffer(),
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

void RenderPassManager::CreateTransparent()
{
	RenderPass::ClearDepth clearDepth{};
	clearDepth.clearDepth = 1.0f;
	clearDepth.clearStencil = 0;

	glm::vec4 clearColor = { 1.0f, 1.0f, 1.0f, 1.0f };

	RenderPass::AttachmentDescription color{};
	color.format = Format::B10G11R11_UFLOAT_PACK32;
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
		index = 3;
		return renderer->GetRenderPassFrameBuffer(GBuffer);
	};

	RenderPass::CreateInfo createInfo{};
	createInfo.type = Transparent;
	createInfo.clearDepths = { clearDepth };
	createInfo.clearColors = { clearColor };
	createInfo.attachmentDescriptions = { color, depth };
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

		const std::shared_ptr<FrameBuffer> frameBuffer = renderInfo.renderer->GetRenderPassFrameBuffer(renderPassName);
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
				renderData.r3d.mesh->GetVertexBuffer(),
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
		const std::shared_ptr<Mesh> plane = MeshManager::GetInstance().LoadMesh("Meshes/Plane.mesh");
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

		std::shared_ptr<UniformWriter> renderUniformWriter = renderInfo.renderer->GetUniformWriter(renderPassName);
		if (!renderUniformWriter)
		{
			std::shared_ptr<UniformLayout> renderUniformLayout =
				pipeline->GetUniformLayout(*pipeline->GetDescriptorSetIndexByType(Pipeline::DescriptorSetIndexType::RENDERER, renderPassName));
			renderUniformWriter = UniformWriter::Create(renderUniformLayout);
			renderInfo.renderer->SetUniformWriter(renderPassName, renderUniformWriter);
		}

		for (const auto& [name, renderTargetInfo] : pipeline->GetCreateInfo().uniformInfo.renderTargetsByName)
		{
			const std::shared_ptr<FrameBuffer> frameBuffer = renderInfo.renderer->GetRenderPassFrameBuffer(renderTargetInfo.renderPassName);
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

		const std::shared_ptr<FrameBuffer> frameBuffer = renderInfo.renderer->GetRenderPassFrameBuffer(renderPassName);
		RenderPass::SubmitInfo submitInfo{};
		submitInfo.frame = renderInfo.frame;
		submitInfo.renderPass = renderInfo.renderPass;
		submitInfo.frameBuffer = frameBuffer;
		renderInfo.renderer->BeginRenderPass(submitInfo);

		renderInfo.renderer->Render(
			plane->GetVertexBuffer(),
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

		const std::shared_ptr<FrameBuffer> frameBuffer = renderInfo.renderer->GetRenderPassFrameBuffer(renderPassName);
		RenderPass::SubmitInfo submitInfo{};
		submitInfo.frame = renderInfo.frame;
		submitInfo.renderPass = renderInfo.renderPass;
		submitInfo.frameBuffer = frameBuffer;
		renderInfo.renderer->BeginRenderPass(submitInfo);

		const GraphicsSettings& graphicsSettings = renderInfo.scene->GetGraphicsSettings();

		if (!graphicsSettings.ssao.isEnabled)
		{
			renderInfo.renderer->EndRenderPass(submitInfo);
			return;
		}

		const std::shared_ptr<Mesh> plane = MeshManager::GetInstance().LoadMesh("Meshes\\Plane.mesh");
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

		m_SSAORenderer.GenerateSamples(graphicsSettings.ssao.kernelSize);

		auto callback = [this, graphicsSettings]()
		{
			m_SSAORenderer.GenerateNoiseTexture(graphicsSettings.ssao.noiseSize);
		};

		std::shared_ptr<NextFrameEvent> event = std::make_shared<NextFrameEvent>(callback, Event::Type::OnNextFrame, this);
		EventSystem::GetInstance().SendEvent(event);

		std::shared_ptr<UniformWriter> renderUniformWriter = renderInfo.renderer->GetUniformWriter(renderPassName);
		if (!renderUniformWriter)
		{
			std::shared_ptr<UniformLayout> renderUniformLayout =
				pipeline->GetUniformLayout(*pipeline->GetDescriptorSetIndexByType(Pipeline::DescriptorSetIndexType::RENDERER, renderPassName));
			renderUniformWriter = UniformWriter::Create(renderUniformLayout);
			renderInfo.renderer->SetUniformWriter(renderPassName, renderUniformWriter);

			for (const auto& binding : renderUniformLayout->GetBindings())
			{
				if (binding.buffer && binding.buffer->name == "SSAOBuffer")
				{
					const std::shared_ptr<Buffer> buffer = Buffer::Create(
						binding.buffer->size,
						1,
						Buffer::Usage::UNIFORM_BUFFER,
						Buffer::MemoryType::CPU);

					renderInfo.renderer->SetBuffer("SSAOBuffer", buffer);
					renderUniformWriter->WriteBuffer(binding.buffer->name, buffer);
					renderUniformWriter->Flush();

					break;
				}
			}
		}

		for (const auto& [name, renderTargetInfo] : pipeline->GetCreateInfo().uniformInfo.renderTargetsByName)
		{
			const std::shared_ptr<FrameBuffer> frameBuffer = renderInfo.renderer->GetRenderPassFrameBuffer(renderTargetInfo.renderPassName);
			renderUniformWriter->WriteTexture(name, frameBuffer->GetAttachment(renderTargetInfo.attachmentIndex));
		}

		const std::shared_ptr<Texture> noiseTexture = m_SSAORenderer.GetNoiseTexture();
		if (noiseTexture)
		{
			renderUniformWriter->WriteTexture("noiseTexture", noiseTexture);
		}

		const std::shared_ptr<Buffer> ssaoBuffer = renderInfo.renderer->GetBuffer("SSAOBuffer");
		baseMaterial->WriteToBuffer(ssaoBuffer, "SSAOBuffer", "viewportScale", GetRenderPass(renderPassName)->GetResizeViewportScale());
		baseMaterial->WriteToBuffer(ssaoBuffer, "SSAOBuffer", "kernelSize", graphicsSettings.ssao.kernelSize);
		baseMaterial->WriteToBuffer(ssaoBuffer, "SSAOBuffer", "noiseSize", graphicsSettings.ssao.noiseSize);
		baseMaterial->WriteToBuffer(ssaoBuffer, "SSAOBuffer", "aoScale", graphicsSettings.ssao.aoScale);
		baseMaterial->WriteToBuffer(ssaoBuffer, "SSAOBuffer", "samples", m_SSAORenderer.GetSamples());
		baseMaterial->WriteToBuffer(ssaoBuffer, "SSAOBuffer", "radius", graphicsSettings.ssao.radius);
		baseMaterial->WriteToBuffer(ssaoBuffer, "SSAOBuffer", "bias", graphicsSettings.ssao.bias);

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
			plane->GetVertexBuffer(),
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

		const std::shared_ptr<FrameBuffer> frameBuffer = renderInfo.renderer->GetRenderPassFrameBuffer(renderPassName);
		RenderPass::SubmitInfo submitInfo{};
		submitInfo.frame = renderInfo.frame;
		submitInfo.renderPass = renderInfo.renderPass;
		submitInfo.frameBuffer = frameBuffer;
		renderInfo.renderer->BeginRenderPass(submitInfo);

		const GraphicsSettings& graphicsSettings = renderInfo.scene->GetGraphicsSettings();

		if (!graphicsSettings.ssao.isEnabled)
		{
			renderInfo.renderer->EndRenderPass(submitInfo);
			return;
		}

		const std::shared_ptr<Mesh> plane = MeshManager::GetInstance().LoadMesh("Meshes\\Plane.mesh");
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

		std::shared_ptr<UniformWriter> renderUniformWriter = renderInfo.renderer->GetUniformWriter(renderPassName);
		if (!renderUniformWriter)
		{
			std::shared_ptr<UniformLayout> renderUniformLayout =
				pipeline->GetUniformLayout(*pipeline->GetDescriptorSetIndexByType(Pipeline::DescriptorSetIndexType::RENDERER, renderPassName));
			renderUniformWriter = UniformWriter::Create(renderUniformLayout);
			renderInfo.renderer->SetUniformWriter(renderPassName, renderUniformWriter);
		}

		for (const auto& [name, renderTargetInfo] : pipeline->GetCreateInfo().uniformInfo.renderTargetsByName)
		{
			const std::shared_ptr<FrameBuffer> frameBuffer = renderInfo.renderer->GetRenderPassFrameBuffer(renderTargetInfo.renderPassName);
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
			plane->GetVertexBuffer(),
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

		std::shared_ptr<FrameBuffer> frameBuffer = renderInfo.renderer->GetRenderPassFrameBuffer(renderPassName);
		if (!frameBuffer)
		{
			// NOTE: Maybe should be send as the next frame event, because creating a frame buffer here may cause some problems.
			const std::string renderPassName = renderInfo.renderPass->GetType();
			renderInfo.renderPass->GetAttachmentDescriptions().back().layercount = renderInfo.scene->GetGraphicsSettings().shadows.cascadeCount;
			frameBuffer = FrameBuffer::Create(renderInfo.renderPass, renderInfo.renderer.get(), { 2048, 2048 });

			renderInfo.renderer->SetFrameBufferToRenderPass(renderPassName, frameBuffer);
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

		std::shared_ptr<Buffer> instanceBuffer = renderInfo.renderer->GetBuffer("InstanceBufferCSM");
		if ((renderableCount != 0 && !instanceBuffer) || (instanceBuffer && renderableCount != 0 && instanceBuffer->GetInstanceCount() != renderableCount))
		{
			instanceBuffer = Buffer::Create(
				sizeof(InstanceDataCSM),
				renderableCount,
				Buffer::Usage::VERTEX_BUFFER,
				Buffer::MemoryType::CPU);

			renderInfo.renderer->SetBuffer("InstanceBufferCSM", instanceBuffer);
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

			std::shared_ptr<UniformWriter> renderUniformWriter = renderInfo.renderer->GetUniformWriter(renderPassName);
			if (!renderUniformWriter)
			{
				std::shared_ptr<UniformLayout> renderUniformLayout =
					pipeline->GetUniformLayout(*pipeline->GetDescriptorSetIndexByType(Pipeline::DescriptorSetIndexType::RENDERER, renderPassName));
				renderUniformWriter = UniformWriter::Create(renderUniformLayout);
				renderInfo.renderer->SetUniformWriter(renderPassName, renderUniformWriter);

				for (const auto& binding : renderUniformLayout->GetBindings())
				{
					if (binding.buffer && binding.buffer->name == "LightSpaceMatrices")
					{
						const std::shared_ptr<Buffer> buffer = Buffer::Create(
							binding.buffer->size,
							1,
							Buffer::Usage::UNIFORM_BUFFER,
							Buffer::MemoryType::CPU);

						renderInfo.renderer->SetBuffer("LightSpaceMatrices", buffer);
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
					renderInfo.renderer->GetBuffer("LightSpaceMatrices"),
					"LightSpaceMatrices",
					"lightSpaceMatrices",
					*csmRenderer.GetLightSpaceMatrices().data());

				const int cascadeCount = csmRenderer.GetLightSpaceMatrices().size();
				baseMaterial->WriteToBuffer(
					renderInfo.renderer->GetBuffer("LightSpaceMatrices"),
					"LightSpaceMatrices",
					"cascadeCount",
					cascadeCount);

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
						mesh->GetVertexBuffer(),
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
