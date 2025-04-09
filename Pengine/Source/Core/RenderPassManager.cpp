#include "RenderPassManager.h"

#include "Logger.h"
#include "MaterialManager.h"
#include "MeshManager.h"
#include "SceneManager.h"
#include "TextureManager.h"
#include "Time.h"
#include "FrustumCulling.h"
#include "Raycast.h"
#include "UIRenderer.h"

#include "../Components/Canvas.h"
#include "../Components/Camera.h"
#include "../Components/DirectionalLight.h"
#include "../Components/PointLight.h"
#include "../Components/Renderer3D.h"
#include "../Components/SkeletalAnimator.h"
#include "../Components/Transform.h"
#include "../Graphics/Device.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/RenderTarget.h"
#include "../Graphics/GraphicsPipeline.h"
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

std::shared_ptr<RenderPass> RenderPassManager::CreateRenderPass(const RenderPass::CreateInfo& createInfo)
{
	std::shared_ptr<RenderPass> renderPass = RenderPass::Create(createInfo);
	m_PassesByName.emplace(createInfo.name, renderPass);

	return renderPass;
}

std::shared_ptr<ComputePass> RenderPassManager::CreateComputePass(const ComputePass::CreateInfo& createInfo)
{
	std::shared_ptr<ComputePass> computePass = std::make_shared<ComputePass>(createInfo);
	m_PassesByName.emplace(createInfo.name, computePass);

	return computePass;
}

std::shared_ptr<Pass> RenderPassManager::GetPass(const std::string& name) const
{
	if (const auto passByName = m_PassesByName.find(name);
		passByName != m_PassesByName.end())
	{
		return passByName->second;
	}

	FATAL_ERROR(name + " id of render pass doesn't exist, please create render pass!");

	return nullptr;
}

std::shared_ptr<RenderPass> RenderPassManager::GetRenderPass(const std::string& name) const
{
	std::shared_ptr<Pass> pass = GetPass(name);

	if (pass->GetType() == Pass::Type::GRAPHICS)
	{
		return std::dynamic_pointer_cast<RenderPass>(pass);
	}

	FATAL_ERROR(name + " id of render pass doesn't have type GRAPHICS!");

	return nullptr;
}

std::shared_ptr<ComputePass> RenderPassManager::GetComputePass(const std::string& name) const
{
	std::shared_ptr<Pass> pass = GetPass(name);

	if (pass->GetType() == Pass::Type::COMPUTE)
	{
		return std::dynamic_pointer_cast<ComputePass>(pass);
	}

	FATAL_ERROR(name + " id of render pass doesn't have type COMPUTE!");

	return nullptr;
}

void RenderPassManager::ShutDown()
{
	m_PassesByName.clear();
}

std::vector<std::shared_ptr<UniformWriter>> RenderPassManager::GetUniformWriters(
	std::shared_ptr<Pipeline> pipeline,
	std::shared_ptr<BaseMaterial> baseMaterial,
	std::shared_ptr<Material> material,
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
		case Pipeline::DescriptorSetIndexType::SCENE:
			uniformWriters.emplace_back(renderInfo.scene->GetRenderTarget()->GetUniformWriter(location.second));
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
	const std::shared_ptr<BaseMaterial> reflectionBaseMaterial = MaterialManager::GetInstance().LoadBaseMaterial(
		std::filesystem::path("Materials") / "DefaultReflection.basemat");
	const std::shared_ptr<Pipeline> pipeline = reflectionBaseMaterial->GetPipeline(DefaultReflection);
	if (!pipeline)
	{
		FATAL_ERROR("DefaultReflection base material is broken! No pipeline found!");
	}

	const std::shared_ptr<UniformWriter> renderUniformWriter = GetOrCreateViewRenderTargetUniformWriter(renderInfo.renderTarget, pipeline, DefaultReflection);
	const std::string globalBufferName = "GlobalBuffer";
	const std::shared_ptr<Buffer> globalBuffer = GetOrCreateRenderBuffer(renderInfo.renderTarget, renderUniformWriter, globalBufferName);

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

	const float deltaTime = Time::GetDeltaTime();
	reflectionBaseMaterial->WriteToBuffer(
		globalBuffer,
		globalBufferName,
		"camera.deltaTime",
		deltaTime);

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

std::shared_ptr<Texture> RenderPassManager::ScaleTexture(
	std::shared_ptr<Texture> sourceTexture,
	const glm::ivec2& dstSize)
{
	const std::shared_ptr<Renderer> renderer = Renderer::Create();
	const std::shared_ptr<BaseMaterial> baseMaterial = MaterialManager::GetInstance().LoadBaseMaterial("Materials/ScaleTexture.basemat");
	const std::shared_ptr<Pipeline> pipeline = baseMaterial->GetPipeline("ScaleTexture");
	if (!renderer || !pipeline)
	{
		return nullptr;
	}

	Texture::CreateInfo createInfo{};
	createInfo.aspectMask = Texture::AspectMask::COLOR;
	createInfo.channels = 4;
	createInfo.filepath = "dstScaleTexture";
	createInfo.name = "dstScaleTexture";
	createInfo.format = Format::R8G8B8A8_UNORM;
	createInfo.size = dstSize;
	createInfo.usage = { Texture::Usage::STORAGE, Texture::Usage::TRANSFER_SRC };
	createInfo.isMultiBuffered = false;
	std::shared_ptr<Texture> dstTexture = TextureManager::GetInstance().Create(createInfo);

	void* frame = device->Begin();

	glm::uvec2 groupCount = glm::uvec2(dstSize.x / 16, dstSize.y / 16);
	groupCount += glm::uvec2(1, 1);
	renderer->Dispatch(
		pipeline,
		{ groupCount.x, groupCount.y, 1 },
		{
			dstTexture->GetUniformWriter(),
			sourceTexture->GetUniformWriter()
		},
		frame);

	device->End(frame);

	return dstTexture;
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
	CreateSSR();
	CreateSSRBlur();
	CreateUI();
}

void RenderPassManager::GetVertexBuffers(
	std::shared_ptr<Pipeline> pipeline,
	std::shared_ptr<Mesh> mesh,
	std::vector<std::shared_ptr<Buffer>>& vertexBuffers,
	std::vector<size_t>& vertexBufferOffsets)
{
	if (pipeline->GetType() != Pipeline::Type::GRAPHICS)
	{
		FATAL_ERROR("Can't get vertex buffers, pipeline type is not Pipeline::Type::GRAPHICS!");
	}

	constexpr size_t arbitraryVertexBufferCount = 4;
	vertexBuffers.reserve(arbitraryVertexBufferCount);
	vertexBufferOffsets.reserve(arbitraryVertexBufferCount);

	std::shared_ptr<GraphicsPipeline> graphicsPipeline = std::static_pointer_cast<GraphicsPipeline>(pipeline);
	const auto& bindingDescriptions = graphicsPipeline->GetCreateInfo().bindingDescriptions;

	// TODO: Optimize search.
	for (const auto& bindingDescription : bindingDescriptions)
	{
		if (bindingDescription.inputRate != GraphicsPipeline::InputRate::VERTEX)
		{
			continue;
		}

		uint32_t vertexLayoutIndex = 0;
		for (const auto& vertexLayout : mesh->GetVertexLayouts())
		{
			if (vertexLayout.tag == bindingDescription.tag)
			{
				vertexBuffers.emplace_back(mesh->GetVertexBuffer(vertexLayoutIndex));
				vertexBufferOffsets.emplace_back(0);
			}

			vertexLayoutIndex++;
		}
	}
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
	createInfo.type = Pass::Type::GRAPHICS;
	createInfo.name = ZPrePass;
	createInfo.clearColors = {};
	createInfo.clearDepths = { clearDepth };
	createInfo.attachmentDescriptions = { depth };
	createInfo.resizeWithViewport = true;
	createInfo.resizeViewportScale = { 1.0f, 1.0f };

	createInfo.executeCallback = [this](const RenderPass::RenderCallbackInfo& renderInfo)
	{
		const std::string renderPassName = renderInfo.renderPass->GetName();

		RenderableEntities renderableEntities;

		size_t renderableCount = 0;
		const std::shared_ptr<Scene> scene = renderInfo.scene;
		entt::registry& registry = scene->GetRegistry();
		const Camera& camera = renderInfo.camera->GetComponent<Camera>();
		const glm::mat4 viewProjectionMat4 = renderInfo.projection * camera.GetViewMat4();
		const auto r3dView = registry.view<Renderer3D>();
		for (entt::entity entity : r3dView)
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
			
			if (r3d.mesh->GetType() == Mesh::Type::SKINNED)
			{
				SkeletalAnimator* skeletalAnimator = registry.try_get<SkeletalAnimator>(entity);
				if (skeletalAnimator)
				{
					UpdateSkeletalAnimator(skeletalAnimator, r3d.material->GetBaseMaterial(), pipeline);
				}

				renderableEntities[r3d.material->GetBaseMaterial()][r3d.material].single.emplace_back(std::make_pair(r3d.mesh, entity));
			}
			else if (r3d.mesh->GetType() == Mesh::Type::STATIC)
			{
				bool isInFrustum = FrustumCulling::CullBoundingBox(viewProjectionMat4, transformMat4, box.min, box.max, camera.GetZNear());
				if (!isInFrustum)
				{
					continue;
				}

				renderableEntities[r3d.material->GetBaseMaterial()][r3d.material].instanced[r3d.mesh].emplace_back(entity);
			}

			// TODO: Remove, for now it is just temporary thing.
			if (auto* canvas = registry.try_get<Canvas>(entity))
			{
				r3d.material->GetUniformWriter(GBuffer)->WriteTexture("albedoTexture", canvas->frameBuffer->GetAttachment(0));
			}

			if (scene->GetSettings().m_DrawBoundingBoxes)
			{
				const glm::vec3 color = glm::vec3(0.0f, 1.0f, 0.0f);

				scene->GetVisualizer().DrawBox(box.min, box.max, color, transformMat4);
			}

			renderableCount++;
		}

		// Commented for now, no really need ZPrePrass, because the engine uses Deferred over Forward renderer.
		//std::shared_ptr<Buffer> instanceBuffer = renderInfo.renderTarget->GetBuffer("InstanceBufferZPrePass");
		//if ((renderableCount != 0 && !instanceBuffer) || (instanceBuffer && renderableCount != 0 && instanceBuffer->GetInstanceCount() < renderableCount))
		//{
		//	instanceBuffer = Buffer::Create(
		//		sizeof(glm::mat4),
		//		renderableCount,
		//		Buffer::Usage::VERTEX_BUFFER,
		//		MemoryType::CPU,
		//		true);

		//	renderInfo.renderTarget->SetBuffer("InstanceBufferZPrePass", instanceBuffer);
		//}

		//std::vector<glm::mat4> instanceDatas;

		//const std::shared_ptr<FrameBuffer> frameBuffer = renderInfo.renderTarget->GetFrameBuffer(renderPassName);

		//RenderPass::SubmitInfo submitInfo{};
		//submitInfo.frame = renderInfo.frame;
		//submitInfo.renderPass = renderInfo.renderPass;
		//submitInfo.frameBuffer = frameBuffer;
		//renderInfo.renderer->BeginRenderPass(submitInfo);

		//// Render all base materials -> materials -> meshes | put gameobjects into the instance buffer.
		//for (const auto& [baseMaterial, meshesByMaterial] : renderableEntities)
		//{
		//	const std::shared_ptr<Pipeline> pipeline = baseMaterial->GetPipeline(renderPassName);

		//	for (const auto& [material, gameObjectsByMeshes] : meshesByMaterial)
		//	{
		//		const std::vector<std::shared_ptr<UniformWriter>> uniformWriters = GetUniformWriters(pipeline, baseMaterial, material, renderInfo);
		//		FlushUniformWriters(uniformWriters);

		//		for (const auto& [mesh, entities] : gameObjectsByMeshes.instanced)
		//		{
		//			const size_t instanceDataOffset = instanceDatas.size();

		//			for (const entt::entity& entity : entities)
		//			{
		//				const Transform& transform = registry.get<Transform>(entity);
		//				instanceDatas.emplace_back(transform.GetTransform());
		//			}

		//			std::vector<std::shared_ptr<Buffer>> vertexBuffers;
		//			std::vector<size_t> vertexBufferOffsets;
		//			GetVertexBuffers(pipeline, mesh, vertexBuffers, vertexBufferOffsets);

		//			renderInfo.renderer->Render(
		//				vertexBuffers,
		//				vertexBufferOffsets,
		//				mesh->GetIndexBuffer(),
		//				0,
		//				mesh->GetIndexCount(),
		//				pipeline,
		//				instanceBuffer,
		//				instanceDataOffset * instanceBuffer->GetInstanceSize(),
		//				entities.size(),
		//				uniformWriters,
		//				renderInfo.frame);
		//		}

		//		for (const auto& [mesh, entity] : gameObjectsByMeshes.single)
		//		{
		//			const size_t instanceDataOffset = instanceDatas.size();

		//			const Transform& transform = registry.get<Transform>(entity);
		//			instanceDatas.emplace_back(transform.GetTransform());

		//			const SkeletalAnimator* skeletalAnimator = registry.try_get<SkeletalAnimator>(entity);
		//			if (skeletalAnimator)
		//			{
		//				std::vector<std::shared_ptr<UniformWriter>> newUniformWriters = uniformWriters;
		//				newUniformWriters.emplace_back(skeletalAnimator->GetUniformWriter());
		//				skeletalAnimator->GetUniformWriter()->Flush();

		//				std::vector<std::shared_ptr<Buffer>> vertexBuffers;
		//				std::vector<size_t> vertexBufferOffsets;
		//				GetVertexBuffers(pipeline, mesh, vertexBuffers, vertexBufferOffsets);

		//				renderInfo.renderer->Render(
		//					vertexBuffers,
		//					vertexBufferOffsets,
		//					mesh->GetIndexBuffer(),
		//					0,
		//					mesh->GetIndexCount(),
		//					pipeline,
		//					instanceBuffer,
		//					instanceDataOffset * instanceBuffer->GetInstanceSize(),
		//					1,
		//					newUniformWriters,
		//					renderInfo.frame);
		//			}
		//		}
		//	}
		//}

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
		/*if (instanceBuffer && !instanceDatas.empty())
		{
			instanceBuffer->WriteToBuffer(instanceDatas.data(), instanceDatas.size() * sizeof(glm::mat4));
			instanceBuffer->Flush();
		}

		renderInfo.renderer->EndRenderPass(submitInfo);*/
	};

	CreateRenderPass(createInfo);
}

void RenderPassManager::CreateGBuffer()
{
	RenderPass::ClearDepth clearDepth{};
	clearDepth.clearDepth = 1.0f;
	clearDepth.clearStencil = 0;

	glm::vec4 clearColor = { 0.1f, 0.1f, 0.1f, 1.0f };
	glm::vec4 clearNormal = { 0.0f, 0.0f, 0.0f, 0.0f };
	glm::vec4 clearShading = { 0.0f, 0.0f, 0.0f, 0.0f };
	glm::vec4 clearEmissive = { 0.0f, 0.0f, 0.0f, 0.0f };

	RenderPass::AttachmentDescription color{};
	color.format = Format::B10G11R11_UFLOAT_PACK32;
	color.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;
	color.load = RenderPass::Load::CLEAR;
	color.store = RenderPass::Store::STORE;

	RenderPass::AttachmentDescription normal{};
	normal.format = Format::R16G16B16A16_SFLOAT;
	normal.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;
	normal.load = RenderPass::Load::CLEAR;
	normal.store = RenderPass::Store::STORE;

	RenderPass::AttachmentDescription shading{};
	shading.format = Format::R8G8B8A8_SRGB;
	shading.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;
	shading.load = RenderPass::Load::CLEAR;
	shading.store = RenderPass::Store::STORE;

	RenderPass::AttachmentDescription emissive{};
	emissive.format = Format::R16G16B16A16_SFLOAT;
	emissive.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;
	emissive.load = RenderPass::Load::CLEAR;
	emissive.store = RenderPass::Store::STORE;

	Texture::SamplerCreateInfo emissiveSamplerCreateInfo{};
	emissiveSamplerCreateInfo.addressMode = Texture::SamplerCreateInfo::AddressMode::CLAMP_TO_BORDER;
	emissiveSamplerCreateInfo.borderColor = Texture::SamplerCreateInfo::BorderColor::FLOAT_OPAQUE_BLACK;
	emissiveSamplerCreateInfo.maxAnisotropy = 1.0f;

	emissive.samplerCreateInfo = emissiveSamplerCreateInfo;

	RenderPass::AttachmentDescription depth{};
	depth.format = Format::D32_SFLOAT;
	depth.layout = Texture::Layout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	// TODO: Revert Changes to LOAD, NONE when ZPrePass is used!
	depth.load = RenderPass::Load::CLEAR;
	depth.store = RenderPass::Store::STORE;
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
	createInfo.type = Pass::Type::GRAPHICS;
	createInfo.name = GBuffer;
	createInfo.clearColors = { clearColor, clearNormal, clearShading, clearEmissive };
	createInfo.clearDepths = { clearDepth };
	createInfo.attachmentDescriptions = { color, normal, shading, emissive, depth };
	createInfo.resizeWithViewport = true;
	createInfo.resizeViewportScale = { 1.0f, 1.0f };

	createInfo.executeCallback = [this](const RenderPass::RenderCallbackInfo& renderInfo)
	{
		const std::string renderPassName = renderInfo.renderPass->GetName();

		RenderableData* renderableData = (RenderableData*)renderInfo.renderTarget->GetCustomData("RenderableData");

		LineRenderer* lineRenderer = (LineRenderer*)renderInfo.scene->GetRenderTarget()->GetCustomData("LineRenderer");
		if (!lineRenderer)
		{
			lineRenderer = new LineRenderer();

			renderInfo.scene->GetRenderTarget()->SetCustomData("LineRenderer", lineRenderer);
		}

		const size_t renderableCount = renderableData->renderableCount;
		const std::shared_ptr<Scene> scene = renderInfo.scene;
		const entt::registry& registry = scene->GetRegistry();

		std::shared_ptr<Buffer> instanceBuffer = renderInfo.renderTarget->GetBuffer("InstanceBuffer");
		if ((renderableCount != 0 && !instanceBuffer) || (instanceBuffer && renderableCount != 0 && instanceBuffer->GetInstanceCount() < renderableCount))
		{
			instanceBuffer = Buffer::Create(
				sizeof(InstanceData),
				renderableCount,
				Buffer::Usage::VERTEX_BUFFER,
				MemoryType::CPU,
				true);

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
				const std::vector<std::shared_ptr<UniformWriter>> uniformWriters = GetUniformWriters(pipeline, baseMaterial, material, renderInfo);
				// Already updated in ZPrePass.
				FlushUniformWriters(uniformWriters);

				for (const auto& [mesh, entities] : gameObjectsByMeshes.instanced)
				{
					const size_t instanceDataOffset = instanceDatas.size();

					bool hasSkltAnim = false;
					for (const entt::entity& entity : entities)
					{
						InstanceData data{};
						const Transform& transform = registry.get<Transform>(entity);
						data.transform = transform.GetTransform();
						data.inverseTransform = glm::transpose(transform.GetInverseTransform());
						instanceDatas.emplace_back(data);
					}

					std::vector<std::shared_ptr<Buffer>> vertexBuffers;
					std::vector<size_t> vertexBufferOffsets;
					GetVertexBuffers(pipeline, mesh, vertexBuffers, vertexBufferOffsets);

					renderInfo.renderer->Render(
						vertexBuffers,
						vertexBufferOffsets,
						mesh->GetIndexBuffer(),
						0,
						mesh->GetIndexCount(),
						pipeline,
						instanceBuffer,
						instanceDataOffset* instanceBuffer->GetInstanceSize(),
						entities.size(),
						uniformWriters,
						renderInfo.frame);
				}

				for (const auto& [mesh, entity] : gameObjectsByMeshes.single)
				{
					const size_t instanceDataOffset = instanceDatas.size();

					InstanceData data{};
					const Transform& transform = registry.get<Transform>(entity);
					data.transform = transform.GetTransform();
					data.inverseTransform = glm::transpose(transform.GetInverseTransform());
					instanceDatas.emplace_back(data);

					const SkeletalAnimator* skeletalAnimator = registry.try_get<SkeletalAnimator>(entity);
					if (skeletalAnimator)
					{
						std::vector<std::shared_ptr<UniformWriter>> newUniformWriters = uniformWriters;
						newUniformWriters.emplace_back(skeletalAnimator->GetUniformWriter());
						// Already updated in ZPrePass.
						skeletalAnimator->GetUniformWriter()->Flush();

						std::vector<std::shared_ptr<Buffer>> vertexBuffers;
						std::vector<size_t> vertexBufferOffsets;
						GetVertexBuffers(pipeline, mesh, vertexBuffers, vertexBufferOffsets);

						renderInfo.renderer->Render(
							vertexBuffers,
							vertexBufferOffsets,
							mesh->GetIndexBuffer(),
							0,
							mesh->GetIndexCount(),
							pipeline,
							instanceBuffer,
							instanceDataOffset * instanceBuffer->GetInstanceSize(),
							1,
							newUniformWriters,
							renderInfo.frame);
					}
				}
			}
		}

		// NOTE: Clear after the draw when it is not needed. If not clear, the crash will appear when closing the application.
		renderableData->renderableCount = 0;
		renderableData->renderableEntities.clear();

		// Because these are all just commands and will be rendered later we can write the instance buffer
		// just once when all instance data is collected.
		if (instanceBuffer && !instanceDatas.empty())
		{
			instanceBuffer->WriteToBuffer(instanceDatas.data(), instanceDatas.size() * sizeof(InstanceData));
			instanceBuffer->Flush();
		}

		lineRenderer->Render(renderInfo);

		// Render SkyBox.
		if (!registry.view<DirectionalLight>().empty())
		{
			std::shared_ptr<Mesh> cubeMesh = MeshManager::GetInstance().LoadMesh("SkyBoxCube");
			std::shared_ptr<BaseMaterial> skyBoxBaseMaterial = MaterialManager::GetInstance().LoadBaseMaterial(
				std::filesystem::path("Materials") / "SkyBox.basemat");

			const std::shared_ptr<Pipeline> pipeline = skyBoxBaseMaterial->GetPipeline(renderPassName);
			if (pipeline)
			{
				std::vector<std::shared_ptr<UniformWriter>> uniformWriters = GetUniformWriters(pipeline, skyBoxBaseMaterial, nullptr, renderInfo);

				std::vector<std::shared_ptr<Buffer>> vertexBuffers;
				std::vector<size_t> vertexBufferOffsets;
				GetVertexBuffers(pipeline, cubeMesh, vertexBuffers, vertexBufferOffsets);

				renderInfo.renderer->Render(
					vertexBuffers,
					vertexBufferOffsets,
					cubeMesh->GetIndexBuffer(),
					0,
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

	CreateRenderPass(createInfo);
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
	createInfo.type = Pass::Type::GRAPHICS;
	createInfo.name = Deferred;
	createInfo.clearColors = { clearColor, clearEmissive };
	createInfo.attachmentDescriptions = { color, emissive };
	createInfo.resizeWithViewport = true;
	createInfo.resizeViewportScale = { 1.0f, 1.0f };

	const std::shared_ptr<Mesh> planeMesh = nullptr;

	createInfo.executeCallback = [this](const RenderPass::RenderCallbackInfo& renderInfo)
	{
		const std::shared_ptr<Mesh> plane = MeshManager::GetInstance().LoadMesh("FullScreenQuad");

		const std::string renderPassName = renderInfo.renderPass->GetName();

		const std::shared_ptr<BaseMaterial> baseMaterial = MaterialManager::GetInstance().LoadBaseMaterial(
			std::filesystem::path("Materials") / "Deferred.basemat");
		const std::shared_ptr<Pipeline> pipeline = baseMaterial->GetPipeline(renderPassName);
		if (!pipeline)
		{
			return;
		}

		const std::shared_ptr<UniformWriter> renderUniformWriter = GetOrCreateViewRenderTargetUniformWriter(renderInfo.renderTarget, pipeline, renderPassName);
		const std::string lightsBufferName = "Lights";
		const std::shared_ptr<Buffer> lightsBuffer = GetOrCreateRenderBuffer(renderInfo.renderTarget, renderUniformWriter, lightsBufferName);

		const Camera& camera = renderInfo.camera->GetComponent<Camera>();

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
			baseMaterial->WriteToBuffer(lightsBuffer, lightsBufferName, valueNamePrefix + ".position", lightPosition);
			baseMaterial->WriteToBuffer(lightsBuffer, lightsBufferName, valueNamePrefix + ".color", pl.color);
			baseMaterial->WriteToBuffer(lightsBuffer, lightsBufferName, valueNamePrefix + ".linear", pl.linear);
			baseMaterial->WriteToBuffer(lightsBuffer, lightsBufferName, valueNamePrefix + ".quadratic", pl.quadratic);
			baseMaterial->WriteToBuffer(lightsBuffer, lightsBufferName, valueNamePrefix + ".constant", pl.constant);

			lightIndex++;
		}

		int pointLightsCount = pointLightView.size();
		baseMaterial->WriteToBuffer(lightsBuffer, lightsBufferName, "pointLightsCount", pointLightsCount);

		const GraphicsSettings& graphicsSettings = renderInfo.scene->GetGraphicsSettings();
		baseMaterial->WriteToBuffer(lightsBuffer, lightsBufferName, "brightnessThreshold", graphicsSettings.bloom.brightnessThreshold);

		auto directionalLightView = renderInfo.scene->GetRegistry().view<DirectionalLight>();
		if (!directionalLightView.empty())
		{
			const entt::entity& entity = directionalLightView.back();
			DirectionalLight& dl = renderInfo.scene->GetRegistry().get<DirectionalLight>(entity);
			const Transform& transform = renderInfo.scene->GetRegistry().get<Transform>(entity);

			baseMaterial->WriteToBuffer(lightsBuffer, lightsBufferName, "directionalLight.color", dl.color);
			baseMaterial->WriteToBuffer(lightsBuffer, lightsBufferName, "directionalLight.intensity", dl.intensity);
			baseMaterial->WriteToBuffer(lightsBuffer, lightsBufferName, "directionalLight.ambient", dl.ambient);

			// View Space!
			const glm::vec3 direction = glm::normalize(glm::mat3(camera.GetViewMat4()) * transform.GetForward());
			baseMaterial->WriteToBuffer(lightsBuffer, lightsBufferName, "directionalLight.direction", direction);

			const int hasDirectionalLight = 1;
			baseMaterial->WriteToBuffer(lightsBuffer, lightsBufferName, "hasDirectionalLight", hasDirectionalLight);

			const GraphicsSettings::Shadows& shadowSettings = renderInfo.scene->GetGraphicsSettings().shadows;

			const int isEnabled = shadowSettings.isEnabled;
			baseMaterial->WriteToBuffer(lightsBuffer, lightsBufferName, "csm.isEnabled", isEnabled);

			CSMRenderer* csmRenderer = (CSMRenderer*)renderInfo.renderTarget->GetCustomData("CSMRenderer");
			if (shadowSettings.isEnabled && !csmRenderer->GetLightSpaceMatrices().empty())
			{
				std::vector<glm::vec4> shadowCascadeLevels;
				for (const float& distance : csmRenderer->GetDistances())
				{
					shadowCascadeLevels.emplace_back(glm::vec4(distance));
				}

				baseMaterial->WriteToBuffer(lightsBuffer, lightsBufferName, "csm.lightSpaceMatrices", *csmRenderer->GetLightSpaceMatrices().data());

				baseMaterial->WriteToBuffer(lightsBuffer, lightsBufferName, "csm.distances", *shadowCascadeLevels.data());

				const int cascadeCount = csmRenderer->GetLightSpaceMatrices().size();
				baseMaterial->WriteToBuffer(lightsBuffer, lightsBufferName, "csm.cascadeCount", cascadeCount);

				baseMaterial->WriteToBuffer(lightsBuffer, lightsBufferName, "csm.fogFactor", shadowSettings.fogFactor);
				baseMaterial->WriteToBuffer(lightsBuffer, lightsBufferName, "csm.maxDistance", shadowSettings.maxDistance);
				baseMaterial->WriteToBuffer(lightsBuffer, lightsBufferName, "csm.pcfRange", shadowSettings.pcfRange);

				const int pcfEnabled = shadowSettings.pcfEnabled;
				baseMaterial->WriteToBuffer(lightsBuffer, lightsBufferName, "csm.pcfEnabled", pcfEnabled);

				const int visualize = shadowSettings.visualize;
				baseMaterial->WriteToBuffer(lightsBuffer, lightsBufferName, "csm.visualize", visualize);

				std::vector<glm::vec4> biases;
				for (const float& bias : shadowSettings.biases)
				{
					biases.emplace_back(glm::vec4(bias));
				}
				baseMaterial->WriteToBuffer(lightsBuffer, lightsBufferName, "csm.biases", *biases.data());
			}
		}
		else
		{
			const int hasDirectionalLight = 0;
			baseMaterial->WriteToBuffer(lightsBuffer, lightsBufferName, "hasDirectionalLight", hasDirectionalLight);
			baseMaterial->WriteToBuffer(lightsBuffer, lightsBufferName, "csm.cascadeCount", hasDirectionalLight);
		}

		WriteRenderTargets(renderInfo.renderTarget, renderInfo.scene->GetRenderTarget(), pipeline, renderUniformWriter);

		const std::vector<std::shared_ptr<UniformWriter>> uniformWriters = GetUniformWriters(pipeline, baseMaterial, nullptr, renderInfo);
		FlushUniformWriters(uniformWriters);

		const std::shared_ptr<FrameBuffer> frameBuffer = renderInfo.renderTarget->GetFrameBuffer(renderPassName);
		RenderPass::SubmitInfo submitInfo{};
		submitInfo.frame = renderInfo.frame;
		submitInfo.renderPass = renderInfo.renderPass;
		submitInfo.frameBuffer = frameBuffer;
		renderInfo.renderer->BeginRenderPass(submitInfo);

		std::vector<std::shared_ptr<Buffer>> vertexBuffers;
		std::vector<size_t> vertexBufferOffsets;
		GetVertexBuffers(pipeline, plane, vertexBuffers, vertexBufferOffsets);

		renderInfo.renderer->Render(
			vertexBuffers,
			vertexBufferOffsets,
			plane->GetIndexBuffer(),
			0,
			plane->GetIndexCount(),
			pipeline,
			nullptr,
			0,
			1,
			uniformWriters,
			renderInfo.frame);

		renderInfo.renderer->EndRenderPass(submitInfo);
	};

	CreateRenderPass(createInfo);
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
	createInfo.type = Pass::Type::GRAPHICS;
	createInfo.name = DefaultReflection;
	createInfo.clearColors = { clearColor };
	createInfo.clearDepths = { clearDepth };
	createInfo.attachmentDescriptions = { color };
	createInfo.createFrameBuffer = false;

	CreateRenderPass(createInfo);
}

void RenderPassManager::CreateAtmosphere()
{
	glm::vec4 clearColor = { 0.0f, 0.0f, 0.0f, 0.0f };

	RenderPass::AttachmentDescription color{};
	color.format = Format::B10G11R11_UFLOAT_PACK32;
	color.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;
	color.load = RenderPass::Load::LOAD;
	color.store = RenderPass::Store::STORE;
	color.size = { 256, 256 };
	color.isCubeMap = true;
	
	RenderPass::CreateInfo createInfo{};
	createInfo.type = Pass::Type::GRAPHICS;
	createInfo.name = Atmosphere;
	createInfo.clearColors = { clearColor };
	createInfo.attachmentDescriptions = { color };
	createInfo.resizeWithViewport = false;

	createInfo.executeCallback = [this](const RenderPass::RenderCallbackInfo& renderInfo)
	{
		const std::string renderPassName = renderInfo.renderPass->GetName();
		std::shared_ptr<FrameBuffer> frameBuffer = renderInfo.scene->GetRenderTarget()->GetFrameBuffer(Atmosphere);
		auto directionalLightView = renderInfo.scene->GetRegistry().view<DirectionalLight>();

		const bool hasDirectionalLight = !directionalLightView.empty();
		if (!hasDirectionalLight)
		{
			renderInfo.scene->GetRenderTarget()->DeleteFrameBuffer(renderPassName);

			return;
		}
		else
		{
			if (!frameBuffer)
			{
				frameBuffer = FrameBuffer::Create(renderInfo.renderPass, renderInfo.scene->GetRenderTarget().get(), {});
				renderInfo.scene->GetRenderTarget()->SetFrameBuffer(renderPassName, frameBuffer);
			}
		}

		const std::shared_ptr<Mesh> plane = MeshManager::GetInstance().GetMesh("FullScreenQuad");

		const std::shared_ptr<BaseMaterial> baseMaterial = MaterialManager::GetInstance().LoadBaseMaterial(
			std::filesystem::path("Materials") / "Atmosphere.basemat");
		const std::shared_ptr<Pipeline> pipeline = baseMaterial->GetPipeline(renderPassName);
		if (!pipeline)
		{
			return;
		}

		const std::shared_ptr<UniformWriter> renderUniformWriter = GetOrCreateSceneRenderTargetUniformWriter(renderInfo.scene->GetRenderTarget(), pipeline, renderPassName);
		const std::string atmosphereBufferName = "AtmosphereBuffer";
		const std::shared_ptr<Buffer> atmosphereBuffer = GetOrCreateRenderBuffer(renderInfo.scene->GetRenderTarget(), renderUniformWriter, atmosphereBufferName);

		if (hasDirectionalLight)
		{
			const entt::entity& entity = directionalLightView.back();
			DirectionalLight& dl = renderInfo.scene->GetRegistry().get<DirectionalLight>(entity);
			const Transform& transform = renderInfo.scene->GetRegistry().get<Transform>(entity);

			baseMaterial->WriteToBuffer(atmosphereBuffer, "AtmosphereBuffer", "directionalLight.color", dl.color);
			baseMaterial->WriteToBuffer(atmosphereBuffer, "AtmosphereBuffer", "directionalLight.intensity", dl.intensity);
			baseMaterial->WriteToBuffer(atmosphereBuffer, "AtmosphereBuffer", "directionalLight.ambient", dl.ambient);

			const glm::vec3 direction = transform.GetForward();
			baseMaterial->WriteToBuffer(atmosphereBuffer, "AtmosphereBuffer", "directionalLight.direction", direction);

			int hasDirectionalLight = 1;
			baseMaterial->WriteToBuffer(atmosphereBuffer, "AtmosphereBuffer", "hasDirectionalLight", hasDirectionalLight);
		}
		else
		{
			int hasDirectionalLight = 0;
			baseMaterial->WriteToBuffer(atmosphereBuffer, "AtmosphereBuffer", "hasDirectionalLight", hasDirectionalLight);
		}

		const glm::vec2 faceSize = frameBuffer->GetSize();
		baseMaterial->WriteToBuffer(atmosphereBuffer, "AtmosphereBuffer", "faceSize", faceSize);

		const std::vector<std::shared_ptr<UniformWriter>> uniformWriters = GetUniformWriters(pipeline, baseMaterial, nullptr, renderInfo);
		FlushUniformWriters(uniformWriters);
		
		RenderPass::SubmitInfo submitInfo{};
		submitInfo.frame = renderInfo.frame;
		submitInfo.renderPass = renderInfo.renderPass;
		submitInfo.frameBuffer = frameBuffer;
		renderInfo.renderer->BeginRenderPass(submitInfo);

		std::vector<std::shared_ptr<Buffer>> vertexBuffers;
		std::vector<size_t> vertexBufferOffsets;
		GetVertexBuffers(pipeline, plane, vertexBuffers, vertexBufferOffsets);

		renderInfo.renderer->Render(
			vertexBuffers,
			vertexBufferOffsets,
			plane->GetIndexBuffer(),
			0,
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
			std::shared_ptr<BaseMaterial> skyBoxBaseMaterial = MaterialManager::GetInstance().LoadBaseMaterial(
				std::filesystem::path("Materials") / "SkyBox.basemat");

			const std::shared_ptr<Pipeline> pipeline = skyBoxBaseMaterial->GetPipeline(GBuffer);
			if (pipeline)
			{
				const std::shared_ptr<UniformWriter> skyBoxUniformWriter = GetOrCreateSceneRenderTargetUniformWriter(renderInfo.scene->GetRenderTarget(), pipeline, "SkyBox");
				skyBoxUniformWriter->WriteTexture("SkyBox", frameBuffer->GetAttachment(0));
				skyBoxUniformWriter->Flush();
			}
		}
	};

	const std::shared_ptr<RenderPass> renderPass = CreateRenderPass(createInfo);
}

void RenderPassManager::CreateTransparent()
{
	RenderPass::ClearDepth clearDepth{};
	clearDepth.clearDepth = 1.0f;
	clearDepth.clearStencil = 0;

	glm::vec4 clearColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	glm::vec4 clearNormal = { 0.0f, 0.0f, 0.0f, 0.0f };
	glm::vec4 clearShading = { 0.0f, 0.0f, 0.0f, 0.0f };
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

	RenderPass::AttachmentDescription normal{};
	normal.format = Format::R16G16B16A16_SFLOAT;
	normal.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;
	normal.load = RenderPass::Load::LOAD;
	normal.store = RenderPass::Store::STORE;
	normal.getFrameBufferCallback = [](RenderTarget* renderTarget, uint32_t& index)
	{
		index = 1;
		return renderTarget->GetFrameBuffer(GBuffer);
	};

	RenderPass::AttachmentDescription shading{};
	shading.format = Format::R8G8B8A8_SRGB;
	shading.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;
	shading.load = RenderPass::Load::LOAD;
	shading.store = RenderPass::Store::STORE;
	shading.getFrameBufferCallback = [](RenderTarget* renderTarget, uint32_t& index)
	{
		index = 2;
		return renderTarget->GetFrameBuffer(GBuffer);
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
	depth.store = RenderPass::Store::STORE;
	depth.getFrameBufferCallback = [](RenderTarget* renderTarget, uint32_t& index)
	{
		index = 0;
		return renderTarget->GetFrameBuffer(ZPrePass);
	};

	RenderPass::CreateInfo createInfo{};
	createInfo.type = Pass::Type::GRAPHICS;
	createInfo.name = Transparent;
	createInfo.clearDepths = { clearDepth };
	createInfo.clearColors = { clearColor, clearNormal, clearShading, clearEmissive };
	createInfo.attachmentDescriptions = { color, normal, shading, emissive, depth };
	createInfo.resizeWithViewport = true;

	createInfo.executeCallback = [](const RenderPass::RenderCallbackInfo& renderInfo)
	{
		const std::string renderPassName = renderInfo.renderPass->GetName();

		struct RenderData
		{
			Renderer3D r3d;
			glm::mat4 transformMat4;
			glm::mat4 rotationMat4;
			glm::mat3 inversetransformMat3;
			glm::vec3 scale;
			glm::vec3 position;
			entt::entity entity;
			float distance2ToCamera = 0.0f;
		};

		std::vector<std::vector<RenderData>> renderDatasByRenderingOrder;
		renderDatasByRenderingOrder.resize(11);

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

			if (r3d.mesh->GetType() == Mesh::Type::STATIC)
			{
				bool isInFrustum = FrustumCulling::CullBoundingBox(renderInfo.projection * camera.GetViewMat4(), transformMat4, box.min, box.max, camera.GetZNear());
				if (!isInFrustum)
				{
					continue;
				}
			}

			if (scene->GetSettings().m_DrawBoundingBoxes)
			{
				const glm::vec3 color = glm::vec3(0.0f, 1.0f, 0.0f);

				scene->GetVisualizer().DrawBox(box.min, box.max, color, transformMat4);
			}

			RenderData renderData{};
			renderData.entity = entity;
			renderData.r3d = r3d;
			renderData.transformMat4 = transform.GetTransform();
			renderData.rotationMat4 = transform.GetRotationMat4();
			renderData.inversetransformMat3 = transform.GetInverseTransform();
			renderData.scale = transform.GetScale();
			renderData.position = transform.GetPosition();

			SkeletalAnimator* skeletalAnimator = registry.try_get<SkeletalAnimator>(entity);
			if (skeletalAnimator)
			{
				UpdateSkeletalAnimator(skeletalAnimator, r3d.material->GetBaseMaterial(), pipeline);
			}

			renderDatasByRenderingOrder[r3d.renderingOrder].emplace_back(renderData);

			renderableCount++;
		}

		if (renderableCount == 0)
		{
			return;
		}

		const glm::vec3 cameraPosition = renderInfo.camera->GetComponent<Transform>().GetPosition();

		for (auto& renderDataByRenderingOrder : renderDatasByRenderingOrder)
		{
			for (RenderData& renderData : renderDataByRenderingOrder)
			{
				const glm::vec3 boundingBoxWorldPosition = renderData.position + renderData.r3d.mesh->GetBoundingBox().offset * renderData.scale;
				const glm::vec3 direction = glm::normalize((boundingBoxWorldPosition)-cameraPosition);

				Raycast::Hit hit{};
				if (Raycast::IntersectBoxOBB(
					cameraPosition,
					direction,
					renderData.r3d.mesh->GetBoundingBox().min,
					renderData.r3d.mesh->GetBoundingBox().max,
					renderData.position,
					renderData.scale,
					renderData.rotationMat4,
					FLT_MAX,
					hit))
				{
					renderData.distance2ToCamera = glm::distance2(cameraPosition, hit.point);
				}
				else
				{
					renderData.distance2ToCamera = glm::distance2(cameraPosition, renderData.position);
				}
			}

			auto isFurther = [cameraPosition](const RenderData& a, const RenderData& b)
			{
				return a.distance2ToCamera > b.distance2ToCamera;
			};

			std::sort(renderDataByRenderingOrder.begin(), renderDataByRenderingOrder.end(), isFurther);
		}

		std::shared_ptr<Buffer> instanceBuffer = renderInfo.renderTarget->GetBuffer("InstanceBufferTransparent");
		if ((renderableCount != 0 && !instanceBuffer) || (instanceBuffer && renderableCount != 0 && instanceBuffer->GetInstanceCount() < renderableCount))
		{
			instanceBuffer = Buffer::Create(
				sizeof(InstanceData),
				renderableCount,
				Buffer::Usage::VERTEX_BUFFER,
				MemoryType::CPU,
				true);

			renderInfo.renderTarget->SetBuffer("InstanceBufferTransparent", instanceBuffer);
		}

		std::vector<InstanceData> instanceDatas;

		const std::shared_ptr<FrameBuffer> frameBuffer = renderInfo.renderTarget->GetFrameBuffer(renderPassName);
		RenderPass::SubmitInfo submitInfo{};
		submitInfo.frame = renderInfo.frame;
		submitInfo.renderPass = renderInfo.renderPass;
		submitInfo.frameBuffer = frameBuffer;
		renderInfo.renderer->BeginRenderPass(submitInfo);

		for (auto& renderDataByRenderingOrder : renderDatasByRenderingOrder)
		{
			for (const auto& renderData : renderDataByRenderingOrder)
			{
				const std::shared_ptr<Pipeline> pipeline = renderData.r3d.material->GetBaseMaterial()->GetPipeline(renderPassName);

				const size_t instanceDataOffset = instanceDatas.size();

				InstanceData data{};
				data.transform = renderData.transformMat4;
				data.inverseTransform = glm::transpose(renderData.inversetransformMat3);
				instanceDatas.emplace_back(data);

				// Can be done more optimal I guess.
				std::vector<std::shared_ptr<UniformWriter>> uniformWriters = GetUniformWriters(
					pipeline,
					renderData.r3d.material->GetBaseMaterial(),
					renderData.r3d.material,
					renderInfo);
				
				const SkeletalAnimator* skeletalAnimator = registry.try_get<SkeletalAnimator>(renderData.entity);
				if (skeletalAnimator)
				{
					uniformWriters.emplace_back(skeletalAnimator->GetUniformWriter());
				}
				
				FlushUniformWriters(uniformWriters);

				std::vector<std::shared_ptr<Buffer>> vertexBuffers;
				std::vector<size_t> vertexBufferOffsets;
				GetVertexBuffers(pipeline, renderData.r3d.mesh, vertexBuffers, vertexBufferOffsets);

				renderInfo.renderer->Render(
					vertexBuffers,
					vertexBufferOffsets,
					renderData.r3d.mesh->GetIndexBuffer(),
					0,
					renderData.r3d.mesh->GetIndexCount(),
					pipeline,
					instanceBuffer,
					instanceDataOffset * instanceBuffer->GetInstanceSize(),
					1,
					uniformWriters,
					renderInfo.frame);
			}
		}

		// Because these are all just commands and will be rendered later we can write the instance buffer
		// just once when all instance data is collected.
		if (instanceBuffer && !instanceDatas.empty())
		{
			instanceBuffer->WriteToBuffer(instanceDatas.data(), instanceDatas.size() * sizeof(InstanceData));
			instanceBuffer->Flush();
		}

		renderInfo.renderer->EndRenderPass(submitInfo);
	};

	CreateRenderPass(createInfo);
}

void RenderPassManager::CreateFinal()
{
	glm::vec4 clearColor = { 1.0f, 1.0f, 1.0f, 1.0f };

	RenderPass::AttachmentDescription color{};
	color.format = Format::R8G8B8A8_SRGB;//B10G11R11_UFLOAT_PACK32;
	color.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;
	color.load = RenderPass::Load::LOAD;
	color.store = RenderPass::Store::STORE;

	RenderPass::CreateInfo createInfo{};
	createInfo.type = Pass::Type::GRAPHICS;
	createInfo.name = Final;
	createInfo.clearColors = { clearColor };
	createInfo.attachmentDescriptions = { color };
	createInfo.resizeWithViewport = true;
	createInfo.resizeViewportScale = { 1.0f, 1.0f };

	const std::shared_ptr<Mesh> planeMesh = nullptr;

	createInfo.executeCallback = [](const RenderPass::RenderCallbackInfo& renderInfo)
	{
		const std::shared_ptr<Mesh> plane = MeshManager::GetInstance().LoadMesh("FullScreenQuad");

		const std::string renderPassName = renderInfo.renderPass->GetName();

		const std::shared_ptr<BaseMaterial> baseMaterial = MaterialManager::GetInstance().LoadBaseMaterial("Materials/Final.basemat");
		const std::shared_ptr<Pipeline> pipeline = baseMaterial->GetPipeline(renderPassName);
		if (!pipeline)
		{
			return;
		}

		const GraphicsSettings& graphicsSettings = renderInfo.scene->GetGraphicsSettings();

		const std::shared_ptr<UniformWriter> renderUniformWriter = GetOrCreateViewRenderTargetUniformWriter(renderInfo.renderTarget, pipeline, renderPassName);
		const std::string postProcessBufferName = "PostProcessBuffer";
		const std::shared_ptr<Buffer> postProcessBuffer = GetOrCreateRenderBuffer(renderInfo.renderTarget, renderUniformWriter, postProcessBufferName);

		const glm::vec2 viewportSize = renderInfo.viewportSize;
		const int fxaa = graphicsSettings.postProcess.fxaa;
		baseMaterial->WriteToBuffer(postProcessBuffer, "PostProcessBuffer", "toneMapperIndex", graphicsSettings.postProcess.toneMapper);
		baseMaterial->WriteToBuffer(postProcessBuffer, "PostProcessBuffer", "gamma", graphicsSettings.postProcess.gamma);
		baseMaterial->WriteToBuffer(postProcessBuffer, "PostProcessBuffer", "viewportSize", viewportSize);
		baseMaterial->WriteToBuffer(postProcessBuffer, "PostProcessBuffer", "fxaa", fxaa);

		const int isSSREnabled = graphicsSettings.ssr.isEnabled;
		baseMaterial->WriteToBuffer(postProcessBuffer, "PostProcessBuffer", "isSSREnabled", isSSREnabled);

		postProcessBuffer->Flush();

		WriteRenderTargets(renderInfo.renderTarget, renderInfo.scene->GetRenderTarget(), pipeline, renderUniformWriter);

		const std::vector<std::shared_ptr<UniformWriter>> uniformWriters = GetUniformWriters(pipeline, baseMaterial, nullptr, renderInfo);
		FlushUniformWriters(uniformWriters);

		const std::shared_ptr<FrameBuffer> frameBuffer = renderInfo.renderTarget->GetFrameBuffer(renderPassName);
		RenderPass::SubmitInfo submitInfo{};
		submitInfo.frame = renderInfo.frame;
		submitInfo.renderPass = renderInfo.renderPass;
		submitInfo.frameBuffer = frameBuffer;
		renderInfo.renderer->BeginRenderPass(submitInfo);

		std::vector<std::shared_ptr<Buffer>> vertexBuffers;
		std::vector<size_t> vertexBufferOffsets;
		GetVertexBuffers(pipeline, plane, vertexBuffers, vertexBufferOffsets);

		renderInfo.renderer->Render(
			vertexBuffers,
			vertexBufferOffsets,
			plane->GetIndexBuffer(),
			0,
			plane->GetIndexCount(),
			pipeline,
			nullptr,
			0,
			1,
			uniformWriters,
			renderInfo.frame);

		renderInfo.renderer->EndRenderPass(submitInfo);
	};

	CreateRenderPass(createInfo);
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
	createInfo.type = Pass::Type::GRAPHICS;
	createInfo.name = CSM;
	createInfo.clearDepths = { clearDepth };
	createInfo.attachmentDescriptions = { depth };
	createInfo.resizeWithViewport = false;
	createInfo.createFrameBuffer = false;

	const std::shared_ptr<Mesh> planeMesh = nullptr;

	createInfo.executeCallback = [this](const RenderPass::RenderCallbackInfo& renderInfo)
	{
		const std::string renderPassName = renderInfo.renderPass->GetName();

		const GraphicsSettings::Shadows& shadowsSettings = renderInfo.scene->GetGraphicsSettings().shadows;
		if (!shadowsSettings.isEnabled)
		{
			renderInfo.renderTarget->DeleteUniformWriter(renderPassName);
			renderInfo.renderTarget->DeleteCustomData("CSMRenderer");
			renderInfo.renderTarget->DeleteBuffer("LightSpaceMatrices");
			renderInfo.renderTarget->DeleteBuffer("InstanceBufferCSM");
			renderInfo.renderTarget->DeleteFrameBuffer(renderPassName);

			return;
		}

		CSMRenderer* csmRenderer = (CSMRenderer*)renderInfo.renderTarget->GetCustomData("CSMRenderer");
		if (!csmRenderer)
		{
			csmRenderer = new CSMRenderer();

			renderInfo.renderTarget->SetCustomData("CSMRenderer", csmRenderer);
		}

		glm::ivec2 resolutions[3] = { { 1024, 1024 }, { 2048, 2048 }, { 4096, 4096 } };

		std::shared_ptr<FrameBuffer> frameBuffer = renderInfo.renderTarget->GetFrameBuffer(renderPassName);
		if (!frameBuffer)
		{
			// NOTE: Maybe should be send as the next frame event, because creating a frame buffer here may cause some problems.
			const std::string renderPassName = renderInfo.renderPass->GetName();
			renderInfo.renderPass->GetAttachmentDescriptions().back().layerCount = renderInfo.scene->GetGraphicsSettings().shadows.cascadeCount;
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

		RenderableEntities renderableEntities;

		size_t renderableCount = 0;
		const std::shared_ptr<Scene> scene = renderInfo.scene;
		const Camera& camera = renderInfo.camera->GetComponent<Camera>();
		entt::registry& registry = scene->GetRegistry();
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

		// TODO: Camera can be ortho, so need to make for ortho as well.
		const glm::mat4 projection = glm::perspective(
			camera.GetFov(),
			(float)renderInfo.viewportSize.x / (float)renderInfo.viewportSize.y,
			camera.GetZNear(),
			shadowsSettings.maxDistance);

		const bool recreateFrameBuffer = csmRenderer->GenerateLightSpaceMatrices(
			projection * camera.GetViewMat4(),
			lightDirection,
			camera.GetZNear(),
			shadowsSettings.maxDistance,
			shadowsSettings.cascadeCount,
			shadowsSettings.splitFactor);

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

			if (r3d.mesh->GetType() == Mesh::Type::SKINNED)
			{
				SkeletalAnimator* skeletalAnimator = registry.try_get<SkeletalAnimator>(entity);
				if (skeletalAnimator)
				{
					UpdateSkeletalAnimator(skeletalAnimator, r3d.material->GetBaseMaterial(), pipeline);
				}

				renderableEntities[r3d.material->GetBaseMaterial()][r3d.material].single.emplace_back(std::make_pair(r3d.mesh, entity));
			}
			else if (r3d.mesh->GetType() == Mesh::Type::STATIC)
			{
				//bool isInFrustum = FrustumCulling::CullBoundingBox(
				//	csmRenderer->GetLightSpaceMatrices().front(),
				//	transform.GetTransform(),
				//	r3d.mesh->GetBoundingBox().min,
				//	r3d.mesh->GetBoundingBox().max,
				//	camera.GetZNear());
				//if (!isInFrustum)
				//{
				//	continue;
				//}

				renderableEntities[r3d.material->GetBaseMaterial()][r3d.material].instanced[r3d.mesh].emplace_back(entity);
			}

			renderableCount++;
		}

		struct InstanceDataCSM
		{
			glm::mat4 transform;
		};

		std::shared_ptr<Buffer> instanceBuffer = renderInfo.renderTarget->GetBuffer("InstanceBufferCSM");
		if ((renderableCount != 0 && !instanceBuffer) || (instanceBuffer && renderableCount != 0 && instanceBuffer->GetInstanceCount() < renderableCount))
		{
			instanceBuffer = Buffer::Create(
				sizeof(InstanceDataCSM),
				renderableCount,
				Buffer::Usage::VERTEX_BUFFER,
				MemoryType::CPU,
				true);

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
		for (const auto& [baseMaterial, meshesByMaterial] : renderableEntities)
		{
			const std::shared_ptr<Pipeline> pipeline = baseMaterial->GetPipeline(renderPassName);

			const std::shared_ptr<UniformWriter> renderUniformWriter = GetOrCreateViewRenderTargetUniformWriter(renderInfo.renderTarget, pipeline, renderPassName);
			const std::string lightSpaceMatricesBufferName = "LightSpaceMatrices";
			const std::shared_ptr<Buffer> lightSpaceMatricesBuffer = GetOrCreateRenderBuffer(renderInfo.renderTarget, renderUniformWriter, lightSpaceMatricesBufferName);

			if (!updatedLightSpaceMatrices)
			{
				updatedLightSpaceMatrices = true;

				baseMaterial->WriteToBuffer(
					lightSpaceMatricesBuffer,
					lightSpaceMatricesBufferName,
					"lightSpaceMatrices",
					*csmRenderer->GetLightSpaceMatrices().data());

				const int cascadeCount = csmRenderer->GetLightSpaceMatrices().size();
				baseMaterial->WriteToBuffer(
					lightSpaceMatricesBuffer,
					lightSpaceMatricesBufferName,
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
				std::vector<std::shared_ptr<UniformWriter>> uniformWriters = GetUniformWriters(pipeline, baseMaterial, material, renderInfo);
				FlushUniformWriters(uniformWriters);

				for (const auto& [mesh, entities] : gameObjectsByMeshes.instanced)
				{
					const size_t instanceDataOffset = instanceDatas.size();

					for (const entt::entity& entity : entities)
					{
						InstanceDataCSM data{};
						const Transform& transform = registry.get<Transform>(entity);
						data.transform = transform.GetTransform();
						instanceDatas.emplace_back(data);
					}

					std::vector<std::shared_ptr<Buffer>> vertexBuffers;
					std::vector<size_t> vertexBufferOffsets;
					GetVertexBuffers(pipeline, mesh, vertexBuffers, vertexBufferOffsets);

					renderInfo.renderer->Render(
						vertexBuffers,
						vertexBufferOffsets,
						mesh->GetIndexBuffer(),
						0,
						mesh->GetIndexCount(),
						pipeline,
						instanceBuffer,
						instanceDataOffset * instanceBuffer->GetInstanceSize(),
						entities.size(),
						uniformWriters,
						renderInfo.frame);
				}

				for (const auto& [mesh, entity] : gameObjectsByMeshes.single)
				{
					const size_t instanceDataOffset = instanceDatas.size();

					const Transform& transform = registry.get<Transform>(entity);
					instanceDatas.emplace_back(transform.GetTransform());

					const SkeletalAnimator* skeletalAnimator = registry.try_get<SkeletalAnimator>(entity);
					if (skeletalAnimator)
					{
						std::vector<std::shared_ptr<UniformWriter>> newUniformWriters = uniformWriters;
						newUniformWriters.emplace_back(skeletalAnimator->GetUniformWriter());

						std::vector<std::shared_ptr<Buffer>> vertexBuffers;
						std::vector<size_t> vertexBufferOffsets;
						GetVertexBuffers(pipeline, mesh, vertexBuffers, vertexBufferOffsets);

						renderInfo.renderer->Render(
							vertexBuffers,
							vertexBufferOffsets,
							mesh->GetIndexBuffer(),
							0,
							mesh->GetIndexCount(),
							pipeline,
							instanceBuffer,
							instanceDataOffset * instanceBuffer->GetInstanceSize(),
							1,
							newUniformWriters,
							renderInfo.frame);
					}
				}
			}
		}

		// Because these are all just commands and will be rendered later we can write the instance buffer
		// just once when all instance data is collected.
		if (instanceBuffer && !instanceDatas.empty())
		{
			instanceBuffer->WriteToBuffer(instanceDatas.data(), instanceDatas.size() * sizeof(InstanceDataCSM));
			instanceBuffer->Flush();
		}

		renderInfo.renderer->EndRenderPass(submitInfo);
	};

	CreateRenderPass(createInfo);
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
	createInfo.type = Pass::Type::GRAPHICS;
	createInfo.name = Bloom;
	createInfo.clearColors = { clearColor };
	createInfo.attachmentDescriptions = { color };
	createInfo.resizeWithViewport = false;
	createInfo.createFrameBuffer = false;
	
	const std::shared_ptr<Mesh> planeMesh = nullptr;

	createInfo.executeCallback = [this](const RenderPass::RenderCallbackInfo& renderInfo)
	{
		const std::string renderPassName = renderInfo.renderPass->GetName();
		const GraphicsSettings::Bloom& bloomSettings = renderInfo.scene->GetGraphicsSettings().bloom;
		const int mipCount = bloomSettings.mipCount;

		if (!bloomSettings.isEnabled)
		{
			renderInfo.renderTarget->DeleteCustomData("BloomData");
			renderInfo.renderTarget->DeleteFrameBuffer(renderPassName);
			for (int mipLevel = 0; mipLevel < mipCount; mipLevel++)
			{
				renderInfo.renderTarget->DeleteFrameBuffer("BloomFrameBuffers(" + std::to_string(mipLevel) + ")");
				renderInfo.renderTarget->DeleteBuffer("BloomBuffers(" + std::to_string(mipLevel) + ")");
				renderInfo.renderTarget->DeleteUniformWriter("BloomDownUniformWriters(" + std::to_string(mipLevel) + ")");
				renderInfo.renderTarget->DeleteUniformWriter("BloomUpUniformWriters(" + std::to_string(mipLevel) + ")");
			}

			return;
		}

		const std::shared_ptr<Mesh> plane = MeshManager::GetInstance().LoadMesh("FullScreenQuad");

		// Used to store unique view (camera) bloom information to compare with current graphics settings
		// and in case if not equal then recreate resources.
		struct BloomData : public CustomData
		{
			int mipCount = 0;
			glm::ivec2 sourceSize = { 0, 0 };
		};
		
		constexpr float resolutionScales[] = { 0.25f, 0.5f, 0.75f, 1.0f };
		if (renderInfo.renderPass->GetResizeViewportScale() != glm::vec2(resolutionScales[bloomSettings.resolutionScale]))
		{
			renderInfo.renderPass->SetResizeViewportScale(glm::vec2(resolutionScales[bloomSettings.resolutionScale]));
		}
		const glm::ivec2 viewportSize = glm::vec2(renderInfo.viewportSize) * renderInfo.renderPass->GetResizeViewportScale();

		bool recreateResources = false;
		BloomData* bloomData = (BloomData*)renderInfo.renderTarget->GetCustomData("BloomData");
		if (!bloomData)
		{
			bloomData = new BloomData();
			bloomData->mipCount = mipCount;
			bloomData->sourceSize = viewportSize;
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
			const std::shared_ptr<BaseMaterial> baseMaterial = MaterialManager::GetInstance().LoadBaseMaterial(
				std::filesystem::path("Materials") / "BloomDownSample.basemat");
			const std::shared_ptr<Pipeline> pipeline = baseMaterial->GetPipeline(renderPassName);
			if (!pipeline)
			{
				return;
			}

			// Create FrameBuffers.
			if (recreateResources || !renderInfo.renderTarget->GetFrameBuffer(renderPassName))
			{
				glm::ivec2 size = viewportSize;
				for (int mipLevel = 0; mipLevel < mipCount; mipLevel++)
				{
					const std::string mipLevelString = std::to_string(mipLevel);

					// NOTE: There are parentheses in the name because square brackets are used in the base material file to find the attachment index.
					renderInfo.renderTarget->SetFrameBuffer("BloomFrameBuffers(" + mipLevelString + ")", FrameBuffer::Create(renderInfo.renderPass, renderInfo.renderTarget.get(), size));

					size.x = glm::max(size.x / 2, 4);
					size.y = glm::max(size.y / 2, 4);
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

					const std::shared_ptr<UniformWriter> renderUniformWriter = GetOrCreateViewRenderTargetUniformWriter(
						renderInfo.renderTarget, pipeline, renderPassName, "BloomDownUniformWriters[" + mipLevelString + "]");
					GetOrCreateRenderBuffer(renderInfo.renderTarget, renderUniformWriter, "MipBuffer", "BloomBuffers[" + mipLevelString + "]");
				}
			}

			// Resize.
			if (bloomData->sourceSize.x != viewportSize.x ||
				bloomData->sourceSize.y != viewportSize.y)
			{
				glm::ivec2 size = viewportSize;
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
				baseMaterial->WriteToBuffer(mipBuffer, "MipBuffer", "bloomIntensity", bloomSettings.intensity);
				baseMaterial->WriteToBuffer(mipBuffer, "MipBuffer", "mipLevel", mipLevel);

				mipBuffer->Flush();

				const std::shared_ptr<UniformWriter> downUniformWriter = renderInfo.renderTarget->GetUniformWriter("BloomDownUniformWriters[" + mipLevelString + "]");
				downUniformWriter->WriteTexture("sourceTexture", sourceTexture);
				downUniformWriter->Flush();

				std::vector<std::shared_ptr<Buffer>> vertexBuffers;
				std::vector<size_t> vertexBufferOffsets;
				GetVertexBuffers(pipeline, plane, vertexBuffers, vertexBufferOffsets);

				renderInfo.renderer->Render(
					vertexBuffers,
					vertexBufferOffsets,
					plane->GetIndexBuffer(),
					0,
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
			const std::shared_ptr<BaseMaterial> baseMaterial = MaterialManager::GetInstance().LoadBaseMaterial(
				std::filesystem::path("Materials") / "BloomUpSample.basemat");
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
					GetOrCreateViewRenderTargetUniformWriter(renderInfo.renderTarget, pipeline, renderPassName, "BloomUpUniformWriters[" + std::to_string(mipLevel) + "]");
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

				std::vector<std::shared_ptr<Buffer>> vertexBuffers;
				std::vector<size_t> vertexBufferOffsets;
				GetVertexBuffers(pipeline, plane, vertexBuffers, vertexBufferOffsets);

				renderInfo.renderer->Render(
					vertexBuffers,
					vertexBufferOffsets,
					plane->GetIndexBuffer(),
					0,
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

	CreateRenderPass(createInfo);
}

void RenderPassManager::CreateSSR()
{
	RenderPass::CreateInfo createInfo{};
	createInfo.type = Pass::Type::COMPUTE;
	createInfo.name = SSR;

	createInfo.executeCallback = [this, passName = createInfo.name](const RenderPass::RenderCallbackInfo& renderInfo)
	{
		const std::shared_ptr<BaseMaterial> baseMaterial = MaterialManager::GetInstance().LoadBaseMaterial(
			std::filesystem::path("Materials") / "SSR.basemat");
		const std::shared_ptr<Pipeline> pipeline = baseMaterial->GetPipeline(passName);
		if (!pipeline)
		{
			return;
		}

		const GraphicsSettings::SSR& ssrSettings = renderInfo.scene->GetGraphicsSettings().ssr;
		const std::string ssrBufferName = passName;
		constexpr float resolutionScales[] = { 0.25f, 0.5f, 0.75f, 1.0f };
		const glm::ivec2 currentViewportSize = glm::vec2(renderInfo.viewportSize) * glm::vec2(resolutionScales[ssrSettings.resolutionScale]);

		Texture::CreateInfo createInfo{};
		createInfo.aspectMask = Texture::AspectMask::COLOR;
		createInfo.channels = 4;
		createInfo.filepath = passName;
		createInfo.name = passName;
		createInfo.format = Format::R8G8B8A8_UNORM;
		createInfo.size = currentViewportSize;
		createInfo.usage = { Texture::Usage::STORAGE, Texture::Usage::SAMPLED };
		createInfo.isMultiBuffered = true;

		std::shared_ptr<Texture> ssrTexture = renderInfo.renderTarget->GetStorageImage(passName);

		if (ssrSettings.isEnabled)
		{
			if (!ssrTexture)
			{
				ssrTexture = Texture::Create(createInfo);
				renderInfo.renderTarget->SetStorageImage(passName, ssrTexture);
				GetOrCreateViewRenderTargetUniformWriter(renderInfo.renderTarget, pipeline, passName)->WriteTexture("outColor", ssrTexture);
			}
		}
		else
		{
			renderInfo.renderTarget->DeleteUniformWriter(passName);
			renderInfo.renderTarget->DeleteStorageImage(passName);
			renderInfo.renderTarget->DeleteBuffer(ssrBufferName);

			return;
		}

		if (currentViewportSize != ssrTexture->GetSize())
		{
			const std::shared_ptr<UniformWriter> renderUniformWriter = GetOrCreateViewRenderTargetUniformWriter(renderInfo.renderTarget, pipeline, passName);
			auto callback = [renderUniformWriter, passName, createInfo, renderInfo]()
			{
				const std::shared_ptr<Texture> ssrTexture = Texture::Create(createInfo);
				renderInfo.renderTarget->SetStorageImage(passName, ssrTexture);
				renderUniformWriter->WriteTexture("outColor", ssrTexture);
			};

			std::shared_ptr<NextFrameEvent> resizeEvent = std::make_shared<NextFrameEvent>(callback, Event::Type::OnNextFrame, this);
			EventSystem::GetInstance().SendEvent(resizeEvent);
		}

		const std::shared_ptr<UniformWriter> renderUniformWriter = GetOrCreateViewRenderTargetUniformWriter(renderInfo.renderTarget, pipeline, passName);

		WriteRenderTargets(renderInfo.renderTarget, renderInfo.scene->GetRenderTarget(), pipeline, renderUniformWriter);

		const glm::vec2 viewportScale = glm::vec2(resolutionScales[ssrSettings.resolutionScale]);
		const std::shared_ptr<Buffer> ssrBuffer = GetOrCreateRenderBuffer(renderInfo.renderTarget, renderUniformWriter, ssrBufferName);
		baseMaterial->WriteToBuffer(ssrBuffer, ssrBufferName, "viewportScale", viewportScale);
		baseMaterial->WriteToBuffer(ssrBuffer, ssrBufferName, "maxDistance", ssrSettings.maxDistance);
		baseMaterial->WriteToBuffer(ssrBuffer, ssrBufferName, "resolution", ssrSettings.resolution);
		baseMaterial->WriteToBuffer(ssrBuffer, ssrBufferName, "stepCount", ssrSettings.stepCount);
		baseMaterial->WriteToBuffer(ssrBuffer, ssrBufferName, "thickness", ssrSettings.thickness);

		std::vector<std::shared_ptr<UniformWriter>> uniformWriters = GetUniformWriters(pipeline, baseMaterial, nullptr, renderInfo);
		FlushUniformWriters(uniformWriters);

		renderInfo.renderer->BeginCommandLabel(passName, topLevelRenderPassDebugColor, renderInfo.frame);

		renderInfo.renderer->BeginCommandLabel(passName, { 1.0f, 1.0f, 0.0f }, renderInfo.frame);

		glm::uvec2 groupCount = currentViewportSize / glm::ivec2(16, 16);
		groupCount += glm::uvec2(1, 1);
		renderInfo.renderer->Dispatch(pipeline, { groupCount.x, groupCount.y, 1 }, uniformWriters, renderInfo.frame);

		renderInfo.renderer->EndCommandLabel(renderInfo.frame);
	};

	CreateRenderPass(createInfo);
}

void RenderPassManager::CreateSSRBlur()
{
	RenderPass::CreateInfo createInfo{};
	createInfo.type = Pass::Type::COMPUTE;
	createInfo.name = SSRBlur;

	createInfo.executeCallback = [this, passName = createInfo.name](const RenderPass::RenderCallbackInfo& renderInfo)
	{
		const std::shared_ptr<BaseMaterial> baseMaterial = MaterialManager::GetInstance().LoadBaseMaterial(
			std::filesystem::path("Materials") / "SSRBlur.basemat");
		const std::shared_ptr<Pipeline> pipeline = baseMaterial->GetPipeline(passName);
		if (!pipeline)
		{
			return;
		}

		const GraphicsSettings::SSR& ssrSettings = renderInfo.scene->GetGraphicsSettings().ssr;
		const std::string ssrBufferName = passName;
		constexpr float resolutionScales[] = { 0.125f, 0.25f, 0.5f, 0.75f, 1.0f };
		const glm::ivec2 currentViewportSize = glm::vec2(renderInfo.viewportSize) * glm::vec2(resolutionScales[ssrSettings.resolutionBlurScale]);

		Texture::CreateInfo createInfo{};
		createInfo.aspectMask = Texture::AspectMask::COLOR;
		createInfo.channels = 4;
		createInfo.filepath = passName;
		createInfo.name = passName;
		createInfo.format = Format::R8G8B8A8_UNORM;
		createInfo.size = currentViewportSize;
		createInfo.usage = { Texture::Usage::STORAGE, Texture::Usage::SAMPLED };
		createInfo.isMultiBuffered = true;

		std::shared_ptr<Texture> ssrBlurTexture = renderInfo.renderTarget->GetStorageImage(passName);

		if (ssrSettings.isEnabled)
		{
			if (!ssrBlurTexture)
			{
				ssrBlurTexture = Texture::Create(createInfo);
				renderInfo.renderTarget->SetStorageImage(passName, ssrBlurTexture);
				GetOrCreateViewRenderTargetUniformWriter(renderInfo.renderTarget, pipeline, passName)->WriteTexture("outColor", ssrBlurTexture);
			}
		}
		else
		{
			renderInfo.renderTarget->DeleteUniformWriter(passName);
			renderInfo.renderTarget->DeleteStorageImage(passName);
			renderInfo.renderTarget->DeleteBuffer(ssrBufferName);

			return;
		}

		if (currentViewportSize != ssrBlurTexture->GetSize())
		{
			const std::shared_ptr<UniformWriter> renderUniformWriter = GetOrCreateViewRenderTargetUniformWriter(renderInfo.renderTarget, pipeline, passName);
			auto callback = [renderUniformWriter, passName, createInfo, renderInfo]()
			{
				const std::shared_ptr<Texture> ssrBlurTexture = Texture::Create(createInfo);
				renderInfo.renderTarget->SetStorageImage(passName, ssrBlurTexture);
				renderUniformWriter->WriteTexture("outColor", ssrBlurTexture);
			};

			std::shared_ptr<NextFrameEvent> resizeEvent = std::make_shared<NextFrameEvent>(callback, Event::Type::OnNextFrame, this);
			EventSystem::GetInstance().SendEvent(resizeEvent);
		}

		const std::shared_ptr<UniformWriter> renderUniformWriter = GetOrCreateViewRenderTargetUniformWriter(renderInfo.renderTarget, pipeline, passName);

		const std::shared_ptr<Buffer> ssrBuffer = GetOrCreateRenderBuffer(renderInfo.renderTarget, renderUniformWriter, ssrBufferName);
		baseMaterial->WriteToBuffer(ssrBuffer, ssrBufferName, "blurRange", ssrSettings.blurRange);
		baseMaterial->WriteToBuffer(ssrBuffer, ssrBufferName, "blurOffset", ssrSettings.blurOffset);

		WriteRenderTargets(renderInfo.renderTarget, renderInfo.scene->GetRenderTarget(), pipeline, renderUniformWriter);

		std::vector<std::shared_ptr<UniformWriter>> uniformWriters = GetUniformWriters(pipeline, baseMaterial, nullptr, renderInfo);
		FlushUniformWriters(uniformWriters);

		renderInfo.renderer->BeginCommandLabel(passName, { 1.0f, 1.0f, 0.0f }, renderInfo.frame);

		glm::uvec2 groupCount = currentViewportSize / glm::ivec2(16, 16);
		groupCount += glm::uvec2(1, 1);
		renderInfo.renderer->Dispatch(pipeline, { groupCount.x, groupCount.y, 1 }, uniformWriters, renderInfo.frame);

		renderInfo.renderer->EndCommandLabel(renderInfo.frame);

		renderInfo.renderer->EndCommandLabel(renderInfo.frame);
	};

	CreateRenderPass(createInfo);
}

void RenderPassManager::CreateSSAO()
{
	ComputePass::CreateInfo createInfo{};
	createInfo.type = Pass::Type::COMPUTE;
	createInfo.name = SSAO;

	createInfo.executeCallback = [this, passName = createInfo.name](const RenderPass::RenderCallbackInfo& renderInfo)
	{
		const std::shared_ptr<BaseMaterial> baseMaterial = MaterialManager::GetInstance().LoadBaseMaterial(
			std::filesystem::path("Materials") / "SSAO.basemat");
		const std::shared_ptr<Pipeline> pipeline = baseMaterial->GetPipeline(passName);
		if (!pipeline)
		{
			return;
		}

		const GraphicsSettings::SSAO& ssaoSettings = renderInfo.scene->GetGraphicsSettings().ssao;
		constexpr float resolutionScales[] = { 0.25f, 0.5f, 0.75f, 1.0f };
		const glm::ivec2 currentViewportSize = glm::vec2(renderInfo.viewportSize) * glm::vec2(resolutionScales[ssaoSettings.resolutionScale]);

		Texture::CreateInfo createInfo{};
		createInfo.aspectMask = Texture::AspectMask::COLOR;
		createInfo.channels = 4;
		createInfo.filepath = passName;
		createInfo.name = passName;
		createInfo.format = Format::R8G8B8A8_UNORM;
		createInfo.size = currentViewportSize;
		createInfo.usage = { Texture::Usage::STORAGE, Texture::Usage::SAMPLED };
		createInfo.isMultiBuffered = true;

		SSAORenderer* ssaoRenderer = (SSAORenderer*)renderInfo.renderTarget->GetCustomData("SSAORenderer");
		if (ssaoSettings.isEnabled && !ssaoRenderer)
		{
			ssaoRenderer = new SSAORenderer();
			renderInfo.renderTarget->SetCustomData("SSAORenderer", ssaoRenderer);
		}

		const std::string ssaoBufferName = passName;
		std::shared_ptr<Texture> ssaoTexture = renderInfo.renderTarget->GetStorageImage(passName);

		if (!ssaoSettings.isEnabled)
		{
			renderInfo.renderTarget->DeleteUniformWriter(passName);
			renderInfo.renderTarget->DeleteCustomData("SSAORenderer");
			renderInfo.renderTarget->DeleteBuffer(ssaoBufferName);
			renderInfo.renderTarget->DeleteStorageImage(passName);

			return;
		}
		else
		{
			if (!renderInfo.renderTarget->GetStorageImage(passName))
			{
				ssaoTexture = Texture::Create(createInfo);
				renderInfo.renderTarget->SetStorageImage(passName, ssaoTexture);
				GetOrCreateViewRenderTargetUniformWriter(renderInfo.renderTarget, pipeline, passName)->WriteTexture("outColor", ssaoTexture);
			}
		}

		if (currentViewportSize != ssaoTexture->GetSize())
		{
			const std::shared_ptr<UniformWriter> renderUniformWriter = GetOrCreateViewRenderTargetUniformWriter(renderInfo.renderTarget, pipeline, passName);
			auto callback = [renderUniformWriter, passName, createInfo, renderInfo]()
			{
				const std::shared_ptr<Texture> ssaoTexture = Texture::Create(createInfo);
				renderInfo.renderTarget->SetStorageImage(passName, ssaoTexture);
				renderUniformWriter->WriteTexture("outColor", ssaoTexture);
			};

			std::shared_ptr<NextFrameEvent> resizeEvent = std::make_shared<NextFrameEvent>(callback, Event::Type::OnNextFrame, this);
			EventSystem::GetInstance().SendEvent(resizeEvent);
		}

		if (ssaoRenderer->GetKernelSize() != ssaoSettings.kernelSize)
		{
			ssaoRenderer->GenerateSamples(ssaoSettings.kernelSize);
		}
		if (ssaoRenderer->GetNoiseSize() != ssaoSettings.noiseSize)
		{
			ssaoRenderer->GenerateNoiseTexture(ssaoSettings.noiseSize);
		}

		const std::shared_ptr<UniformWriter> renderUniformWriter = GetOrCreateViewRenderTargetUniformWriter(renderInfo.renderTarget, pipeline, passName);
		const std::shared_ptr<Buffer> ssaoBuffer = GetOrCreateRenderBuffer(renderInfo.renderTarget, renderUniformWriter, ssaoBufferName);

		WriteRenderTargets(renderInfo.renderTarget, renderInfo.scene->GetRenderTarget(), pipeline, renderUniformWriter);
		renderUniformWriter->WriteTexture("noiseTexture", ssaoRenderer->GetNoiseTexture());

		const glm::vec2 viewportScale = glm::vec2(resolutionScales[ssaoSettings.resolutionScale]);
		baseMaterial->WriteToBuffer(ssaoBuffer, ssaoBufferName, "viewportScale", viewportScale);
		baseMaterial->WriteToBuffer(ssaoBuffer, ssaoBufferName, "kernelSize", ssaoSettings.kernelSize);
		baseMaterial->WriteToBuffer(ssaoBuffer, ssaoBufferName, "noiseSize", ssaoSettings.noiseSize);
		baseMaterial->WriteToBuffer(ssaoBuffer, ssaoBufferName, "aoScale", ssaoSettings.aoScale);
		baseMaterial->WriteToBuffer(ssaoBuffer, ssaoBufferName, "samples", ssaoRenderer->GetSamples());
		baseMaterial->WriteToBuffer(ssaoBuffer, ssaoBufferName, "radius", ssaoSettings.radius);
		baseMaterial->WriteToBuffer(ssaoBuffer, ssaoBufferName, "bias", ssaoSettings.bias);

		std::vector<std::shared_ptr<UniformWriter>> uniformWriters = GetUniformWriters(pipeline, baseMaterial, nullptr, renderInfo);
		FlushUniformWriters(uniformWriters);

		renderInfo.renderer->BeginCommandLabel(passName, topLevelRenderPassDebugColor, renderInfo.frame);

		renderInfo.renderer->BeginCommandLabel(passName, { 1.0f, 1.0f, 0.0f }, renderInfo.frame);

		glm::uvec2 groupCount = currentViewportSize / glm::ivec2(16, 16);
		groupCount += glm::uvec2(1, 1);
		renderInfo.renderer->Dispatch(pipeline, { groupCount.x, groupCount.y, 1 }, uniformWriters, renderInfo.frame);

		renderInfo.renderer->EndCommandLabel(renderInfo.frame);
	};

	CreateComputePass(createInfo);
}

void RenderPassManager::CreateSSAOBlur()
{
	ComputePass::CreateInfo createInfo{};
	createInfo.type = Pass::Type::COMPUTE;
	createInfo.name = SSAOBlur;

	createInfo.executeCallback = [this, passName = createInfo.name](const RenderPass::RenderCallbackInfo& renderInfo)
	{
		const std::shared_ptr<BaseMaterial> baseMaterial = MaterialManager::GetInstance().LoadBaseMaterial(
			std::filesystem::path("Materials") / "SSAOBlur.basemat");
		const std::shared_ptr<Pipeline> pipeline = baseMaterial->GetPipeline(passName);
		if (!pipeline)
		{
			return;
		}

		const GraphicsSettings::SSAO& ssaoSettings = renderInfo.scene->GetGraphicsSettings().ssao;
		constexpr float resolutionScales[] = { 0.25f, 0.5f, 0.75f, 1.0f };
		const glm::ivec2 currentViewportSize = glm::vec2(renderInfo.viewportSize) * glm::vec2(resolutionScales[ssaoSettings.resolutionBlurScale]);

		Texture::CreateInfo createInfo{};
		createInfo.aspectMask = Texture::AspectMask::COLOR;
		createInfo.channels = 4;
		createInfo.filepath = passName;
		createInfo.name = passName;
		createInfo.format = Format::R8G8B8A8_UNORM;
		createInfo.size = currentViewportSize;
		createInfo.usage = { Texture::Usage::STORAGE, Texture::Usage::SAMPLED };
		createInfo.isMultiBuffered = true;

		std::shared_ptr<Texture> ssaoBlurTexture = renderInfo.renderTarget->GetStorageImage(passName);
		if (!ssaoSettings.isEnabled)
		{
			renderInfo.renderTarget->DeleteUniformWriter(passName);
			renderInfo.renderTarget->DeleteStorageImage(passName);

			return;
		}
		else
		{
			if (!renderInfo.renderTarget->GetStorageImage(passName))
			{
				ssaoBlurTexture = Texture::Create(createInfo);
				renderInfo.renderTarget->SetStorageImage(passName, ssaoBlurTexture);
				GetOrCreateViewRenderTargetUniformWriter(renderInfo.renderTarget, pipeline, passName)->WriteTexture("outColor", ssaoBlurTexture);
			}
		}

		const std::shared_ptr<UniformWriter> renderUniformWriter = GetOrCreateViewRenderTargetUniformWriter(renderInfo.renderTarget, pipeline, passName);
		if (currentViewportSize != ssaoBlurTexture->GetSize())
		{
			auto callback = [renderUniformWriter, passName, createInfo, renderInfo]()
			{
				const std::shared_ptr<Texture> ssaoBlurTexture = Texture::Create(createInfo);
				renderInfo.renderTarget->SetStorageImage(passName, ssaoBlurTexture);
				renderUniformWriter->WriteTexture("outColor", ssaoBlurTexture);
			};

			std::shared_ptr<NextFrameEvent> resizeEvent = std::make_shared<NextFrameEvent>(callback, Event::Type::OnNextFrame, this);
			EventSystem::GetInstance().SendEvent(resizeEvent);
		}

		WriteRenderTargets(renderInfo.renderTarget, renderInfo.scene->GetRenderTarget(), pipeline, renderUniformWriter);

		std::vector<std::shared_ptr<UniformWriter>> uniformWriters = GetUniformWriters(pipeline, baseMaterial, nullptr, renderInfo);
		FlushUniformWriters(uniformWriters);

		renderInfo.renderer->BeginCommandLabel(passName, { 1.0f, 1.0f, 0.0f }, renderInfo.frame);

		glm::uvec2 groupCount = currentViewportSize / glm::ivec2(16, 16);
		groupCount += glm::uvec2(1, 1);
		renderInfo.renderer->Dispatch(pipeline, { groupCount.x, groupCount.y, 1 }, uniformWriters, renderInfo.frame);

		renderInfo.renderer->EndCommandLabel(renderInfo.frame);

		renderInfo.renderer->EndCommandLabel(renderInfo.frame);
	};

	CreateComputePass(createInfo);
}

void RenderPassManager::CreateUI()
{
	glm::vec4 clearColor = { 0.0f, 0.0f, 0.0f, 0.0f };

	RenderPass::AttachmentDescription color{};
	color.format = Format::R8G8B8A8_UNORM;
	color.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;
	color.load = RenderPass::Load::CLEAR;
	color.store = RenderPass::Store::STORE;

	RenderPass::CreateInfo createInfo{};
	createInfo.type = Pass::Type::GRAPHICS;
	createInfo.name = UI;
	createInfo.clearColors = { clearColor };
	createInfo.attachmentDescriptions = { color };
	createInfo.resizeWithViewport = true;
	createInfo.resizeViewportScale = { 1.0f, 1.0f };

	createInfo.executeCallback = [](const RenderPass::RenderCallbackInfo& renderInfo)
	{
		UIRenderer* uiRenderer = (UIRenderer*)renderInfo.renderTarget->GetCustomData("UIRenderer");
		if (!uiRenderer)
		{
			uiRenderer = new UIRenderer();

			renderInfo.renderTarget->SetCustomData("UIRenderer", uiRenderer);
		}

		const std::shared_ptr<BaseMaterial> baseMaterial = MaterialManager::GetInstance().LoadBaseMaterial("Materials/RectangleUI.basemat");
		const auto& view = renderInfo.scene->GetRegistry().view<Canvas>();
		std::vector<entt::entity> entities;
		entities.reserve(view.size());
		for (const auto& entity : view)
		{
			entities.emplace_back(entity);

			Canvas& canvas = renderInfo.scene->GetRegistry().get<Canvas>(entity);
			if (!canvas.drawInMainViewport)
			{
				if (canvas.size != glm::ivec2(0, 0))
				{
					if (!canvas.frameBuffer)
					{
						canvas.frameBuffer = FrameBuffer::Create(renderInfo.renderPass, nullptr, canvas.size);
					}
					else
					{
						if (canvas.size != canvas.frameBuffer->GetSize())
						{
							canvas.frameBuffer->Resize(canvas.size);
						}
					}
				}
			}
			else
			{
				if (canvas.frameBuffer)
				{
					canvas.frameBuffer = nullptr;
				}
			}
		}

		uiRenderer->Render(entities, baseMaterial, renderInfo);
	};

	CreateRenderPass(createInfo);
}

void RenderPassManager::FlushUniformWriters(const std::vector<std::shared_ptr<UniformWriter>>& uniformWriters)
{
	for (const auto& uniformWriter : uniformWriters)
	{
		uniformWriter->Flush();

		for (const auto& [name, buffers] : uniformWriter->GetBuffersByName())
		{
			for (const auto& buffer : buffers)
			{
				buffer->Flush();
			}
		}
	}
}

void RenderPassManager::WriteRenderTargets(
	std::shared_ptr<RenderTarget> cameraRenderTarget,
	std::shared_ptr<RenderTarget> sceneRenderTarget,
	std::shared_ptr<Pipeline> pipeline,
	std::shared_ptr<UniformWriter> uniformWriter)
{
	for (const auto& [name, textureAttachmentInfo] : pipeline->GetUniformInfo().textureAttachmentsByName)
	{
		if (cameraRenderTarget)
		{
			const std::shared_ptr<FrameBuffer> frameBuffer = cameraRenderTarget->GetFrameBuffer(textureAttachmentInfo.name);
			if (frameBuffer)
			{
				uniformWriter->WriteTexture(name, frameBuffer->GetAttachment(textureAttachmentInfo.attachmentIndex));
				continue;
			}

			const std::shared_ptr<Texture> texture = cameraRenderTarget->GetStorageImage(textureAttachmentInfo.name);
			if (texture)
			{
				uniformWriter->WriteTexture(name, texture);
				continue;
			}
		}

		if (sceneRenderTarget)
		{
			const std::shared_ptr<FrameBuffer> frameBuffer = sceneRenderTarget->GetFrameBuffer(textureAttachmentInfo.name);
			if (frameBuffer)
			{
				uniformWriter->WriteTexture(name, frameBuffer->GetAttachment(textureAttachmentInfo.attachmentIndex));
				continue;
			}

			const std::shared_ptr<Texture> texture = sceneRenderTarget->GetStorageImage(textureAttachmentInfo.name);
			if (texture)
			{
				uniformWriter->WriteTexture(name, texture);
				continue;
			}
		}

		if (!textureAttachmentInfo.defaultName.empty())
		{
			uniformWriter->WriteTexture(name, TextureManager::GetInstance().GetTexture(textureAttachmentInfo.defaultName));
			continue;
		}

		uniformWriter->WriteTexture(name, TextureManager::GetInstance().GetWhite());
	}
}

std::shared_ptr<UniformWriter> RenderPassManager::GetOrCreateViewRenderTargetUniformWriter(
	std::shared_ptr<RenderTarget> renderTarget,
	std::shared_ptr<Pipeline> pipeline,
	const std::string& passName,
	const std::string& setUniformWriterName)
{
	std::shared_ptr<UniformWriter> renderUniformWriter = renderTarget->GetUniformWriter(passName);
	if (!renderUniformWriter)
	{
		const std::shared_ptr<UniformLayout> renderUniformLayout =
			pipeline->GetUniformLayout(*pipeline->GetDescriptorSetIndexByType(Pipeline::DescriptorSetIndexType::RENDERER, passName));
		renderUniformWriter = UniformWriter::Create(renderUniformLayout);
		renderTarget->SetUniformWriter(setUniformWriterName.empty() ? passName : setUniformWriterName, renderUniformWriter);
	}

	return renderUniformWriter;
}

std::shared_ptr<UniformWriter> RenderPassManager::GetOrCreateSceneRenderTargetUniformWriter(
	std::shared_ptr<RenderTarget> renderTarget,
	std::shared_ptr<Pipeline> pipeline,
	const std::string& passName,
	const std::string& setUniformWriterName)
{
	std::shared_ptr<UniformWriter> renderUniformWriter = renderTarget->GetUniformWriter(passName);
	if (!renderUniformWriter)
	{
		const std::shared_ptr<UniformLayout> renderUniformLayout =
			pipeline->GetUniformLayout(*pipeline->GetDescriptorSetIndexByType(Pipeline::DescriptorSetIndexType::SCENE, passName));
		renderUniformWriter = UniformWriter::Create(renderUniformLayout);
		renderTarget->SetUniformWriter(setUniformWriterName.empty() ? passName : setUniformWriterName, renderUniformWriter);
	}

	return renderUniformWriter;
}

std::shared_ptr<Buffer> RenderPassManager::GetOrCreateRenderBuffer(
	std::shared_ptr<RenderTarget> renderTarget,
	std::shared_ptr<UniformWriter> uniformWriter,
	const std::string& bufferName,
	const std::string& setBufferName)
{
	std::shared_ptr<Buffer> buffer = renderTarget->GetBuffer(bufferName);
	if (buffer)
	{
		return buffer;
	}

	auto binding = uniformWriter->GetUniformLayout()->GetBindingByName(bufferName);
	if (binding && binding->buffer)
	{
		buffer = Buffer::Create(
			binding->buffer->size,
			1,
			Buffer::Usage::UNIFORM_BUFFER,
			MemoryType::CPU);

		renderTarget->SetBuffer(setBufferName.empty() ? bufferName : setBufferName, buffer);
		uniformWriter->WriteBuffer(binding->buffer->name, buffer);
		uniformWriter->Flush();

		return buffer;
	}

	FATAL_ERROR(bufferName + ":Failed to create buffer, no such binding was found!");
}

void RenderPassManager::UpdateSkeletalAnimator(
	SkeletalAnimator* skeletalAnimator,
	std::shared_ptr<BaseMaterial> baseMaterial,
	std::shared_ptr<Pipeline> pipeline)
{
	if (!skeletalAnimator->GetUniformWriter())
	{
		const std::shared_ptr<UniformLayout> renderUniformLayout =
			pipeline->GetUniformLayout(*pipeline->GetDescriptorSetIndexByType(Pipeline::DescriptorSetIndexType::OBJECT, GBuffer));
		skeletalAnimator->SetUniformWriter(UniformWriter::Create(renderUniformLayout));
	}

	if (!skeletalAnimator->GetBuffer())
	{
		auto binding = skeletalAnimator->GetUniformWriter()->GetUniformLayout()->GetBindingByName("BonesMatrices");
		if (binding && binding->buffer)
		{
			skeletalAnimator->SetBuffer(Buffer::Create(
				binding->buffer->size,
				1,
				Buffer::Usage::UNIFORM_BUFFER,
				MemoryType::CPU));

			skeletalAnimator->GetUniformWriter()->WriteBuffer(binding->buffer->name, skeletalAnimator->GetBuffer());
			skeletalAnimator->GetUniformWriter()->Flush();
		}
	}

	baseMaterial->WriteToBuffer(skeletalAnimator->GetBuffer(), "BonesMatrices", "bonesMatrices", *skeletalAnimator->GetFinalBoneMatrices().data());
	skeletalAnimator->GetBuffer()->Flush();
}
