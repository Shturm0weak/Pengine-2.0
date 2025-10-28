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
#include "Profiler.h"

#include "../Components/Canvas.h"
#include "../Components/Camera.h"
#include "../Components/Decal.h"
#include "../Components/DirectionalLight.h"
#include "../Components/PointLight.h"
#include "../Components/Renderer3D.h"
#include "../Components/SkeletalAnimator.h"
#include "../Components/Transform.h"
#include "../Graphics/Device.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/RenderView.h"
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
	PROFILER_SCOPE(__FUNCTION__);

	std::vector<std::shared_ptr<UniformWriter>> uniformWriters;
	for (const auto& [set, location] : pipeline->GetSortedDescriptorSets())
	{
		switch (location.first)
		{
		case Pipeline::DescriptorSetIndexType::RENDERER:
			uniformWriters.emplace_back(renderInfo.renderView->GetUniformWriter(location.second));
			break;
		case Pipeline::DescriptorSetIndexType::SCENE:
			uniformWriters.emplace_back(renderInfo.scene->GetRenderView()->GetUniformWriter(location.second));
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
	PROFILER_SCOPE(__FUNCTION__);

	const std::shared_ptr<BaseMaterial> reflectionBaseMaterial = MaterialManager::GetInstance().LoadBaseMaterial(
		std::filesystem::path("Materials") / "DefaultReflection.basemat");
	const std::shared_ptr<Pipeline> pipeline = reflectionBaseMaterial->GetPipeline(DefaultReflection);
	if (!pipeline)
	{
		FATAL_ERROR("DefaultReflection base material is broken! No pipeline found!");
	}

	const std::shared_ptr<UniformWriter> renderUniformWriter = GetOrCreateUniformWriter(renderInfo.renderView, pipeline, Pipeline::DescriptorSetIndexType::RENDERER, DefaultReflection);
	const std::string globalBufferName = "GlobalBuffer";
	const std::shared_ptr<Buffer> globalBuffer = GetOrCreateRenderBuffer(renderInfo.renderView, renderUniformWriter, globalBufferName);

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

	const glm::vec3 position = cameraTransform.GetPosition();
	reflectionBaseMaterial->WriteToBuffer(
		globalBuffer,
		globalBufferName,
		"camera.position",
		position);

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
		renderInfo.renderView->GetBuffer(globalBufferName),
		globalBufferName,
		"camera.viewportSize",
		viewportSize);

	const float aspectRation = viewportSize.x / viewportSize.y;
	reflectionBaseMaterial->WriteToBuffer(
		renderInfo.renderView->GetBuffer(globalBufferName),
		globalBufferName,
		"camera.aspectRatio",
		aspectRation);

	const float tanHalfFOV = tanf(camera.GetFov() / 2.0f);
	reflectionBaseMaterial->WriteToBuffer(
		renderInfo.renderView->GetBuffer(globalBufferName),
		globalBufferName,
		"camera.tanHalfFOV",
		tanHalfFOV);

	const Scene::WindSettings& windSettings = renderInfo.scene->GetWindSettings();
	const glm::vec3 windDirection = glm::normalize(windSettings.direction);
	reflectionBaseMaterial->WriteToBuffer(
		renderInfo.renderView->GetBuffer(globalBufferName),
		globalBufferName,
		"camera.wind.direction",
		windDirection);

	reflectionBaseMaterial->WriteToBuffer(
		renderInfo.renderView->GetBuffer(globalBufferName),
		globalBufferName,
		"camera.wind.strength",
		windSettings.strength);

	reflectionBaseMaterial->WriteToBuffer(
		renderInfo.renderView->GetBuffer(globalBufferName),
		globalBufferName,
		"camera.wind.frequency",
		windSettings.frequency);
}

std::shared_ptr<Texture> RenderPassManager::ScaleTexture(
	std::shared_ptr<Texture> sourceTexture,
	const glm::ivec2& dstSize)
{
	PROFILER_SCOPE(__FUNCTION__);

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
	CreateToneMappingPass();
	CreateSSAO();
	CreateSSAOBlur();
	CreateCSM();
	CreateBloom();
	CreateSSR();
	CreateSSRBlur();
	CreateAntiAliasingAndComposePass();
	CreateUI();
	CreateDecalPass();
}

void RenderPassManager::GetVertexBuffers(
	std::shared_ptr<Pipeline> pipeline,
	std::shared_ptr<Mesh> mesh,
	std::vector<std::shared_ptr<Buffer>>& vertexBuffers,
	std::vector<size_t>& vertexBufferOffsets)
{
	PROFILER_SCOPE(__FUNCTION__);

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
	depth.textureCreateInfo.format = Format::D32_SFLOAT;
	depth.textureCreateInfo.aspectMask = Texture::AspectMask::DEPTH;
	depth.textureCreateInfo.channels = 1;
	depth.textureCreateInfo.isMultiBuffered = true;
	depth.textureCreateInfo.usage = { Texture::Usage::SAMPLED, Texture::Usage::TRANSFER_SRC, Texture::Usage::DEPTH_STENCIL_ATTACHMENT };
	depth.textureCreateInfo.name = "ZPrePassDepth";
	depth.textureCreateInfo.filepath = depth.textureCreateInfo.name;
	depth.layout = Texture::Layout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	Texture::SamplerCreateInfo depthSamplerCreateInfo{};
	depthSamplerCreateInfo.addressMode = Texture::SamplerCreateInfo::AddressMode::CLAMP_TO_BORDER;
	depthSamplerCreateInfo.borderColor = Texture::SamplerCreateInfo::BorderColor::FLOAT_OPAQUE_WHITE;

	depth.textureCreateInfo.samplerCreateInfo = depthSamplerCreateInfo;

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
		PROFILER_SCOPE(ZPrePass);

		const std::string renderPassName = renderInfo.renderPass->GetName();

		RenderableEntities renderableEntities;

		size_t renderableCount = 0;
		const std::shared_ptr<Scene> scene = renderInfo.scene;
		entt::registry& registry = scene->GetRegistry();
		const Camera& camera = renderInfo.camera->GetComponent<Camera>();
		const glm::mat4 viewProjectionMat4 = renderInfo.projection * camera.GetViewMat4();
		const auto r3dView = registry.view<Renderer3D>();

		/*scene->GetBVH()->Traverse([scene](SceneBVH::BVHNode* node)
			{
				scene->GetVisualizer().DrawBox(node->aabb.min, node->aabb.max, { 1.0f, 0.5f, 0.0f }, glm::mat4(1.0f));
			}
		);*/

		for (const entt::entity& entity : r3dView)
		{
			const Renderer3D& r3d = registry.get<Renderer3D>(entity);
			if ((r3d.objectVisibilityMask & camera.GetObjectVisibilityMask()) == 0)
			{
				continue;
			}

			const Transform& transform = registry.get<Transform>(entity);
			if (!transform.GetEntity()->IsEnabled() || !r3d.isEnabled)
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
				if (const auto skeletalAnimatorEntity = transform.GetEntity()->GetTopEntity()->FindEntityInHierarchy(r3d.skeletalAnimatorEntityName))
				{
					SkeletalAnimator* skeletalAnimator = registry.try_get<SkeletalAnimator>(skeletalAnimatorEntity->GetHandle());
					if (skeletalAnimator)
					{
						UpdateSkeletalAnimator(skeletalAnimator, r3d.material->GetBaseMaterial(), pipeline);
					}
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

			if (scene->GetSettings().drawBoundingBoxes)
			{
				const glm::vec3 color = glm::vec3(0.0f, 1.0f, 0.0f);

				scene->GetVisualizer().DrawBox(box.min, box.max, color, transformMat4);
			
				/*r3d.mesh->GetBVH()->Traverse([scene, transformMat4](MeshBVH::BVHNode* node)
					{
						scene->GetVisualizer().DrawBox(node->aabb.min, node->aabb.max, { 1.0f, 1.0f, 0.0f }, transformMat4);
					}
				);*/
			}

			renderableCount++;
		}

		// Commented for now, no really need ZPrePrass, because the engine uses Deferred over Forward renderer.
		//std::shared_ptr<Buffer> instanceBuffer = renderInfo.renderView->GetBuffer("InstanceBufferZPrePass");
		//if ((renderableCount != 0 && !instanceBuffer) || (instanceBuffer && renderableCount != 0 && instanceBuffer->GetInstanceCount() < renderableCount))
		//{
		//	instanceBuffer = Buffer::Create(
		//		sizeof(glm::mat4),
		//		renderableCount,
		//		Buffer::Usage::VERTEX_BUFFER,
		//		MemoryType::CPU,
		//		true);

		//	renderInfo.renderView->SetBuffer("InstanceBufferZPrePass", instanceBuffer);
		//}

		//std::vector<glm::mat4> instanceDatas;

		//const std::shared_ptr<FrameBuffer> frameBuffer = renderInfo.renderView->GetFrameBuffer(renderPassName);

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

		RenderableData* renderableData = (RenderableData*)renderInfo.renderView->GetCustomData("RenderableData");
		if (!renderableData)
		{
			renderableData = new RenderableData();
			renderInfo.renderView->SetCustomData("RenderableData", renderableData);
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
	color.textureCreateInfo.format = Format::B10G11R11_UFLOAT_PACK32;
	color.textureCreateInfo.aspectMask = Texture::AspectMask::COLOR;
	color.textureCreateInfo.channels = 3;
	color.textureCreateInfo.isMultiBuffered = true;
	color.textureCreateInfo.usage = { Texture::Usage::SAMPLED, Texture::Usage::TRANSFER_SRC, Texture::Usage::COLOR_ATTACHMENT };
	color.textureCreateInfo.name = "GBufferColor";
	color.textureCreateInfo.filepath = color.textureCreateInfo.name;
	color.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;
	color.load = RenderPass::Load::CLEAR;
	color.store = RenderPass::Store::STORE;

	RenderPass::AttachmentDescription normal{};
	normal.textureCreateInfo.format = Format::R16G16B16A16_SFLOAT;
	normal.textureCreateInfo.aspectMask = Texture::AspectMask::COLOR;
	normal.textureCreateInfo.channels = 4;
	normal.textureCreateInfo.isMultiBuffered = true;
	normal.textureCreateInfo.usage = { Texture::Usage::SAMPLED, Texture::Usage::TRANSFER_SRC, Texture::Usage::STORAGE, Texture::Usage::COLOR_ATTACHMENT };
	normal.textureCreateInfo.name = "GBufferNormal";
	normal.textureCreateInfo.filepath = normal.textureCreateInfo.name;
	normal.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;
	normal.load = RenderPass::Load::CLEAR;
	normal.store = RenderPass::Store::STORE;

	RenderPass::AttachmentDescription shading{};
	shading.textureCreateInfo.format = Format::R8G8B8A8_UNORM;
	shading.textureCreateInfo.aspectMask = Texture::AspectMask::COLOR;
	shading.textureCreateInfo.channels = 4;
	shading.textureCreateInfo.isMultiBuffered = true;
	shading.textureCreateInfo.usage = { Texture::Usage::SAMPLED, Texture::Usage::TRANSFER_SRC, Texture::Usage::COLOR_ATTACHMENT };
	shading.textureCreateInfo.name = "GBufferShading";
	shading.textureCreateInfo.filepath = shading.textureCreateInfo.name;
	shading.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;
	shading.load = RenderPass::Load::CLEAR;
	shading.store = RenderPass::Store::STORE;

	RenderPass::AttachmentDescription emissive{};
	emissive.textureCreateInfo.format = Format::B10G11R11_UFLOAT_PACK32;
	emissive.textureCreateInfo.aspectMask = Texture::AspectMask::COLOR;
	emissive.textureCreateInfo.channels = 3;
	emissive.textureCreateInfo.isMultiBuffered = true;
	emissive.textureCreateInfo.usage = { Texture::Usage::SAMPLED, Texture::Usage::TRANSFER_SRC, Texture::Usage::STORAGE, Texture::Usage::COLOR_ATTACHMENT };
	emissive.textureCreateInfo.name = "GBufferEmissive";
	emissive.textureCreateInfo.filepath = emissive.textureCreateInfo.name;
	emissive.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;
	emissive.load = RenderPass::Load::CLEAR;
	emissive.store = RenderPass::Store::STORE;

	Texture::SamplerCreateInfo emissiveSamplerCreateInfo{};
	emissiveSamplerCreateInfo.addressMode = Texture::SamplerCreateInfo::AddressMode::CLAMP_TO_BORDER;
	emissiveSamplerCreateInfo.borderColor = Texture::SamplerCreateInfo::BorderColor::FLOAT_OPAQUE_BLACK;
	emissiveSamplerCreateInfo.maxAnisotropy = 1.0f;

	emissive.textureCreateInfo.samplerCreateInfo = emissiveSamplerCreateInfo;

	RenderPass::AttachmentDescription depth = GetRenderPass(ZPrePass)->GetAttachmentDescriptions()[0];
	// TODO: Revert Changes to LOAD, NONE when ZPrePass is used!
	depth.load = RenderPass::Load::CLEAR;
	depth.store = RenderPass::Store::STORE;
	depth.getFrameBufferCallback = [](RenderView* renderView)
	{
		return renderView->GetFrameBuffer(ZPrePass)->GetAttachment(0);
	};

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
		PROFILER_SCOPE(GBuffer);
		
		const std::string renderPassName = renderInfo.renderPass->GetName();

		RenderableData* renderableData = (RenderableData*)renderInfo.renderView->GetCustomData("RenderableData");

		LineRenderer* lineRenderer = (LineRenderer*)renderInfo.scene->GetRenderView()->GetCustomData("LineRenderer");
		if (!lineRenderer)
		{
			lineRenderer = new LineRenderer();

			renderInfo.scene->GetRenderView()->SetCustomData("LineRenderer", lineRenderer);
		}

		const size_t renderableCount = renderableData->renderableCount;
		const std::shared_ptr<Scene> scene = renderInfo.scene;
		const entt::registry& registry = scene->GetRegistry();

		std::shared_ptr<Buffer> instanceBuffer = renderInfo.renderView->GetBuffer("InstanceBuffer");
		if ((renderableCount != 0 && !instanceBuffer) || (instanceBuffer && renderableCount != 0 && instanceBuffer->GetInstanceCount() < renderableCount))
		{
			instanceBuffer = Buffer::Create(
				sizeof(InstanceData),
				renderableCount,
				Buffer::Usage::VERTEX_BUFFER,
				MemoryType::CPU,
				true);

			renderInfo.renderView->SetBuffer("InstanceBuffer", instanceBuffer);
		}

		std::vector<InstanceData> instanceDatas;

		const std::shared_ptr<FrameBuffer> frameBuffer = renderInfo.renderView->GetFrameBuffer(renderPassName);

		RenderPass::SubmitInfo submitInfo{};
		submitInfo.frame = renderInfo.frame;
		submitInfo.renderPass = renderInfo.renderPass;
		submitInfo.frameBuffer = frameBuffer;
		renderInfo.renderer->BeginRenderPass(submitInfo);

		// Render all base materials -> materials -> meshes | put gameobjects into the instance buffer.
		for (const auto& [baseMaterial, meshesByMaterial] : renderableData->renderableEntities)
		{
			const std::shared_ptr<Pipeline> pipeline = baseMaterial->GetPipeline(renderPassName);
			if (!pipeline)
			{
				continue;
			}

			for (const auto& [material, gameObjectsByMeshes] : meshesByMaterial)
			{
				const std::vector<std::shared_ptr<UniformWriter>> uniformWriters = GetUniformWriters(pipeline, baseMaterial, material, renderInfo);
				// Already updated in ZPrePass.
				if (!FlushUniformWriters(uniformWriters))
				{
					continue;
				}

				for (const auto& [mesh, entities] : gameObjectsByMeshes.instanced)
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

					const SkeletalAnimator* skeletalAnimator = nullptr;
					const Renderer3D& r3d = registry.get<Renderer3D>(entity);
					if (const auto skeletalAnimatorEntity = transform.GetEntity()->GetTopEntity()->FindEntityInHierarchy(r3d.skeletalAnimatorEntityName))
					{
						skeletalAnimator = registry.try_get<SkeletalAnimator>(skeletalAnimatorEntity->GetHandle());
					}

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
			std::shared_ptr<Mesh> cubeMesh = MeshManager::GetInstance().LoadMesh("UnitCube");
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
	ComputePass::CreateInfo createInfo{};
	createInfo.type = Pass::Type::COMPUTE;
	createInfo.name = Deferred;

	createInfo.executeCallback = [this, passName = createInfo.name](const RenderPass::RenderCallbackInfo& renderInfo)
	{
		PROFILER_SCOPE(Deferred);

		const std::shared_ptr<BaseMaterial> baseMaterial = MaterialManager::GetInstance().LoadBaseMaterial(
			std::filesystem::path("Materials") / "Deferred.basemat");
		const std::shared_ptr<Pipeline> pipeline = baseMaterial->GetPipeline(passName);
		if (!pipeline)
		{
			return;
		}

		const std::string lightsBufferName = "Lights";
		const std::shared_ptr<UniformWriter> lightsUniformWriter = GetOrCreateRendererUniformWriter(renderInfo.renderView, pipeline, lightsBufferName);
		const std::shared_ptr<Buffer> lightsBuffer = GetOrCreateRenderBuffer(renderInfo.renderView, lightsUniformWriter, lightsBufferName);

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
			baseMaterial->WriteToBuffer(lightsBuffer, lightsBufferName, valueNamePrefix + ".intensity", pl.intensity);
			baseMaterial->WriteToBuffer(lightsBuffer, lightsBufferName, valueNamePrefix + ".radius", pl.radius);

			if (pl.drawBoundingSphere)
			{
				constexpr glm::vec3 color = glm::vec3(0.0f, 1.0f, 0.0f);
				renderInfo.scene->GetVisualizer().DrawSphere(color, transform.GetTransform(), pl.radius, 10);
			}

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

			CSMRenderer* csmRenderer = (CSMRenderer*)renderInfo.renderView->GetCustomData("CSMRenderer");
			if (shadowSettings.isEnabled && !csmRenderer->GetLightSpaceMatrices().empty())
			{
				// Also in Shaders/Includes/CSM.h
				constexpr size_t maxCascadeCount = 10;

				std::vector<glm::vec4> shadowCascadeLevels(maxCascadeCount, glm::vec4{});
				for (size_t i = 0; i < csmRenderer->GetDistances().size(); i++)
				{
					shadowCascadeLevels[i] = glm::vec4(csmRenderer->GetDistances()[i]);
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

				std::vector<glm::vec4> biases(maxCascadeCount);
				for (size_t i = 0; i < shadowSettings.biases.size(); i++)
				{
					biases[i] = glm::vec4(shadowSettings.biases[i]);
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

		const std::shared_ptr<UniformWriter> renderUniformWriter = GetOrCreateRendererUniformWriter(renderInfo.renderView, pipeline, passName);
		WriteRenderViews(renderInfo.renderView, renderInfo.scene->GetRenderView(), pipeline, renderUniformWriter);

		const std::shared_ptr<Texture> colorTexture = renderInfo.renderView->GetFrameBuffer(Transparent)->GetAttachment(0);
		const std::shared_ptr<Texture> emissiveTexture = renderInfo.renderView->GetFrameBuffer(GBuffer)->GetAttachment(3);

		const std::shared_ptr<UniformWriter> outputUniformWriter = GetOrCreateRendererUniformWriter(renderInfo.renderView, pipeline, "DeferredOutput");
		outputUniformWriter->WriteTexture("outColor", colorTexture);
		outputUniformWriter->WriteTexture("outEmissive", emissiveTexture);
		
		std::vector<std::shared_ptr<UniformWriter>> uniformWriters = GetUniformWriters(pipeline, baseMaterial, nullptr, renderInfo);
		if (FlushUniformWriters(uniformWriters))
		{
			renderInfo.renderer->BeginCommandLabel(passName, topLevelRenderPassDebugColor, renderInfo.frame);

			glm::uvec2 groupCount = renderInfo.viewportSize / glm::ivec2(16, 16);
			groupCount += glm::uvec2(1, 1);
			renderInfo.renderer->Dispatch(pipeline, { groupCount.x, groupCount.y, 1 }, uniformWriters, renderInfo.frame);

			renderInfo.renderer->EndCommandLabel(renderInfo.frame);
		}
	};

	CreateComputePass(createInfo);
}

void RenderPassManager::CreateDefaultReflection()
{
	RenderPass::ClearDepth clearDepth{};
	clearDepth.clearDepth = 1.0f;
	clearDepth.clearStencil = 0;

	glm::vec4 clearColor = { 1.0f, 1.0f, 1.0f, 1.0f };

	RenderPass::AttachmentDescription color{};
	color.textureCreateInfo.format = Format::R8G8B8A8_SRGB;
	color.textureCreateInfo.aspectMask = Texture::AspectMask::COLOR;
	color.textureCreateInfo.channels = 4;
	color.textureCreateInfo.isMultiBuffered = true;
	color.textureCreateInfo.usage = { Texture::Usage::SAMPLED, Texture::Usage::TRANSFER_SRC, Texture::Usage::COLOR_ATTACHMENT };
	color.textureCreateInfo.name = "DefaultReflectionColor";
	color.textureCreateInfo.filepath = color.textureCreateInfo.name;
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

	color.textureCreateInfo.format = Format::B10G11R11_UFLOAT_PACK32;
	color.textureCreateInfo.aspectMask = Texture::AspectMask::COLOR;
	color.textureCreateInfo.channels = 3;
	color.textureCreateInfo.isMultiBuffered = true;
	color.textureCreateInfo.usage = { Texture::Usage::SAMPLED, Texture::Usage::TRANSFER_SRC, Texture::Usage::COLOR_ATTACHMENT };
	color.textureCreateInfo.name = "AtmosphereColor";
	color.textureCreateInfo.filepath = color.textureCreateInfo.name;
	color.textureCreateInfo.size = { 256, 256 };
	color.textureCreateInfo.isCubeMap = true;
	color.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;
	color.load = RenderPass::Load::LOAD;
	color.store = RenderPass::Store::STORE;
	
	RenderPass::CreateInfo createInfo{};
	createInfo.type = Pass::Type::GRAPHICS;
	createInfo.name = Atmosphere;
	createInfo.clearColors = { clearColor };
	createInfo.attachmentDescriptions = { color };
	createInfo.resizeWithViewport = false;

	createInfo.executeCallback = [this](const RenderPass::RenderCallbackInfo& renderInfo)
	{
		PROFILER_SCOPE(Atmosphere);

		const std::string renderPassName = renderInfo.renderPass->GetName();
		std::shared_ptr<FrameBuffer> frameBuffer = renderInfo.scene->GetRenderView()->GetFrameBuffer(Atmosphere);
		auto directionalLightView = renderInfo.scene->GetRegistry().view<DirectionalLight>();

		const bool hasDirectionalLight = !directionalLightView.empty();
		if (!hasDirectionalLight)
		{
			renderInfo.scene->GetRenderView()->DeleteFrameBuffer(renderPassName);

			return;
		}
		else
		{
			if (!frameBuffer)
			{
				frameBuffer = FrameBuffer::Create(renderInfo.renderPass, renderInfo.scene->GetRenderView().get(), {});
				renderInfo.scene->GetRenderView()->SetFrameBuffer(renderPassName, frameBuffer);
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

		const std::shared_ptr<UniformWriter> renderUniformWriter = GetOrCreateUniformWriter(
			renderInfo.scene->GetRenderView(), pipeline, Pipeline::DescriptorSetIndexType::SCENE, renderPassName);
		const std::string atmosphereBufferName = "AtmosphereBuffer";
		const std::shared_ptr<Buffer> atmosphereBuffer = GetOrCreateRenderBuffer(renderInfo.scene->GetRenderView(), renderUniformWriter, atmosphereBufferName);

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
		if (!FlushUniformWriters(uniformWriters))
		{
			return;
		}
		
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
				const std::shared_ptr<UniformWriter> skyBoxUniformWriter = GetOrCreateUniformWriter(
					renderInfo.scene->GetRenderView(), pipeline, Pipeline::DescriptorSetIndexType::SCENE, "SkyBox");
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
	color.textureCreateInfo.format = Format::B10G11R11_UFLOAT_PACK32;
	color.textureCreateInfo.aspectMask = Texture::AspectMask::COLOR;
	color.textureCreateInfo.channels = 3;
	color.textureCreateInfo.isMultiBuffered = true;
	color.textureCreateInfo.usage = { Texture::Usage::SAMPLED, Texture::Usage::TRANSFER_SRC, Texture::Usage::STORAGE, Texture::Usage::COLOR_ATTACHMENT };
	color.textureCreateInfo.name = "DeferredColor";
	color.textureCreateInfo.filepath = color.textureCreateInfo.name;
	color.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;
	color.load = RenderPass::Load::LOAD;
	color.store = RenderPass::Store::STORE;

	RenderPass::AttachmentDescription normal = GetRenderPass(GBuffer)->GetAttachmentDescriptions()[1];
	normal.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;
	normal.load = RenderPass::Load::LOAD;
	normal.store = RenderPass::Store::STORE;
	normal.getFrameBufferCallback = [](RenderView* renderView)
	{
		return renderView->GetFrameBuffer(GBuffer)->GetAttachment(1);
	};

	RenderPass::AttachmentDescription shading = GetRenderPass(GBuffer)->GetAttachmentDescriptions()[2];
	shading.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;
	shading.load = RenderPass::Load::LOAD;
	shading.store = RenderPass::Store::STORE;
	shading.getFrameBufferCallback = [](RenderView* renderView)
	{
		return renderView->GetFrameBuffer(GBuffer)->GetAttachment(2);
	};

	RenderPass::AttachmentDescription emissive = GetRenderPass(GBuffer)->GetAttachmentDescriptions()[3];
	emissive.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;
	emissive.load = RenderPass::Load::LOAD;
	emissive.store = RenderPass::Store::STORE;
	emissive.getFrameBufferCallback = [](RenderView* renderView)
	{
		return renderView->GetFrameBuffer(GBuffer)->GetAttachment(3);
	};

	RenderPass::AttachmentDescription depth = GetRenderPass(ZPrePass)->GetAttachmentDescriptions()[0];
	depth.layout = Texture::Layout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depth.load = RenderPass::Load::LOAD;
	depth.store = RenderPass::Store::STORE;
	depth.getFrameBufferCallback = [](RenderView* renderView)
	{
		return renderView->GetFrameBuffer(ZPrePass)->GetAttachment(0);
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
		PROFILER_SCOPE(Transparent);

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
		const Camera& camera = renderInfo.camera->GetComponent<Camera>();
		const std::shared_ptr<Scene> scene = renderInfo.scene;
		entt::registry& registry = scene->GetRegistry();
		const auto r3dView = registry.view<Renderer3D>();
		for (const entt::entity& entity : r3dView)
		{
			Renderer3D& r3d = registry.get<Renderer3D>(entity);
			if ((r3d.objectVisibilityMask & camera.GetObjectVisibilityMask()) == 0)
			{
				continue;
			}

			Transform& transform = registry.get<Transform>(entity);
			if (!transform.GetEntity()->IsEnabled() || !r3d.isEnabled)
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

			if (scene->GetSettings().drawBoundingBoxes)
			{
				constexpr glm::vec3 color = glm::vec3(0.0f, 1.0f, 0.0f);
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

			if (const auto skeletalAnimatorEntity = transform.GetEntity()->GetTopEntity()->FindEntityInHierarchy(r3d.skeletalAnimatorEntityName))
			{
				SkeletalAnimator* skeletalAnimator = registry.try_get<SkeletalAnimator>(skeletalAnimatorEntity->GetHandle());
				if (skeletalAnimator)
				{
					UpdateSkeletalAnimator(skeletalAnimator, r3d.material->GetBaseMaterial(), pipeline);
				}
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

		std::shared_ptr<Buffer> instanceBuffer = renderInfo.renderView->GetBuffer("InstanceBufferTransparent");
		if ((renderableCount != 0 && !instanceBuffer) || (instanceBuffer && renderableCount != 0 && instanceBuffer->GetInstanceCount() < renderableCount))
		{
			instanceBuffer = Buffer::Create(
				sizeof(InstanceData),
				renderableCount,
				Buffer::Usage::VERTEX_BUFFER,
				MemoryType::CPU,
				true);

			renderInfo.renderView->SetBuffer("InstanceBufferTransparent", instanceBuffer);
		}

		std::vector<InstanceData> instanceDatas;

		const std::shared_ptr<FrameBuffer> frameBuffer = renderInfo.renderView->GetFrameBuffer(renderPassName);
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
				
				SkeletalAnimator* skeletalAnimator = nullptr;
				const Renderer3D& r3d = registry.get<Renderer3D>(renderData.entity);
				if (const auto skeletalAnimatorEntity = registry.get<Transform>(renderData.entity).GetEntity()->GetTopEntity()->FindEntityInHierarchy(r3d.skeletalAnimatorEntityName))
				{
					skeletalAnimator = registry.try_get<SkeletalAnimator>(skeletalAnimatorEntity->GetHandle());
				}

				if (skeletalAnimator)
				{
					uniformWriters.emplace_back(skeletalAnimator->GetUniformWriter());
				}
				
				if (!FlushUniformWriters(uniformWriters))
				{
					continue;
				}

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

void RenderPassManager::CreateCSM()
{
	RenderPass::ClearDepth clearDepth{};
	clearDepth.clearDepth = 1.0f;
	clearDepth.clearStencil = 0;

	RenderPass::AttachmentDescription depth{};
	depth.textureCreateInfo.format = Format::D32_SFLOAT;
	depth.textureCreateInfo.aspectMask = Texture::AspectMask::DEPTH;
	depth.textureCreateInfo.channels = 1;
	depth.textureCreateInfo.isMultiBuffered = true;
	depth.textureCreateInfo.usage = { Texture::Usage::SAMPLED, Texture::Usage::TRANSFER_SRC, Texture::Usage::DEPTH_STENCIL_ATTACHMENT };
	depth.textureCreateInfo.name = "CSM";
	depth.textureCreateInfo.filepath = depth.textureCreateInfo.name;
	depth.layout = Texture::Layout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	Texture::SamplerCreateInfo samplerCreateInfo{};
	samplerCreateInfo.filter = Texture::SamplerCreateInfo::Filter::NEAREST;
	samplerCreateInfo.borderColor = Texture::SamplerCreateInfo::BorderColor::FLOAT_OPAQUE_WHITE;
	samplerCreateInfo.addressMode = Texture::SamplerCreateInfo::AddressMode::CLAMP_TO_EDGE;
	samplerCreateInfo.maxAnisotropy = 1.0f;

	depth.textureCreateInfo.samplerCreateInfo = samplerCreateInfo;

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
		PROFILER_SCOPE(CSM);

		const std::string renderPassName = renderInfo.renderPass->GetName();

		const GraphicsSettings::Shadows& shadowsSettings = renderInfo.scene->GetGraphicsSettings().shadows;
		if (!shadowsSettings.isEnabled)
		{
			renderInfo.renderView->DeleteUniformWriter(renderPassName);
			renderInfo.renderView->DeleteCustomData("CSMRenderer");
			renderInfo.renderView->DeleteBuffer("LightSpaceMatrices");
			renderInfo.renderView->DeleteBuffer("InstanceBufferCSM");
			renderInfo.renderView->DeleteFrameBuffer(renderPassName);

			return;
		}

		CSMRenderer* csmRenderer = (CSMRenderer*)renderInfo.renderView->GetCustomData("CSMRenderer");
		if (!csmRenderer)
		{
			csmRenderer = new CSMRenderer();

			renderInfo.renderView->SetCustomData("CSMRenderer", csmRenderer);
		}

		glm::ivec2 resolutions[3] = { { 1024, 1024 }, { 2048, 2048 }, { 4096, 4096 } };

		std::shared_ptr<FrameBuffer> frameBuffer = renderInfo.renderView->GetFrameBuffer(renderPassName);
		if (!frameBuffer)
		{
			// NOTE: Maybe should be send as the next frame event, because creating a frame buffer here may cause some problems.
			const std::string renderPassName = renderInfo.renderPass->GetName();
			renderInfo.renderPass->GetAttachmentDescriptions().back().textureCreateInfo.layerCount = renderInfo.scene->GetGraphicsSettings().shadows.cascadeCount;
			frameBuffer = FrameBuffer::Create(renderInfo.renderPass, renderInfo.renderView.get(), resolutions[shadowsSettings.quality]);

			renderInfo.renderView->SetFrameBuffer(renderPassName, frameBuffer);
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

			if (!r3d.castShadows)
			{
				continue;
			}

			if ((r3d.shadowVisibilityMask & camera.GetShadowVisibilityMask()) == 0)
			{
				continue;
			}

			const Transform& transform = registry.get<Transform>(entity);
			if (!transform.GetEntity()->IsEnabled() || !r3d.isEnabled)
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
				if (const auto skeletalAnimatorEntity = transform.GetEntity()->GetTopEntity()->FindEntityInHierarchy(r3d.skeletalAnimatorEntityName))
				{
					SkeletalAnimator* skeletalAnimator = registry.try_get<SkeletalAnimator>(skeletalAnimatorEntity->GetHandle());
					if (skeletalAnimator)
					{
						UpdateSkeletalAnimator(skeletalAnimator, r3d.material->GetBaseMaterial(), pipeline);
					}
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

		std::shared_ptr<Buffer> instanceBuffer = renderInfo.renderView->GetBuffer("InstanceBufferCSM");
		if ((renderableCount != 0 && !instanceBuffer) || (instanceBuffer && renderableCount != 0 && instanceBuffer->GetInstanceCount() < renderableCount))
		{
			instanceBuffer = Buffer::Create(
				sizeof(InstanceDataCSM),
				renderableCount,
				Buffer::Usage::VERTEX_BUFFER,
				MemoryType::CPU,
				true);

			renderInfo.renderView->SetBuffer("InstanceBufferCSM", instanceBuffer);
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
			if (!pipeline)
			{
				continue;
			}

			const std::shared_ptr<UniformWriter> renderUniformWriter = GetOrCreateRendererUniformWriter(renderInfo.renderView, pipeline, renderPassName);
			const std::string lightSpaceMatricesBufferName = "LightSpaceMatrices";
			const std::shared_ptr<Buffer> lightSpaceMatricesBuffer = GetOrCreateRenderBuffer(renderInfo.renderView, renderUniformWriter, lightSpaceMatricesBufferName);

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
				if (!FlushUniformWriters(uniformWriters))
				{
					continue;
				}

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

					SkeletalAnimator* skeletalAnimator = nullptr;
					const Renderer3D& r3d = registry.get<Renderer3D>(entity);
					if (const auto skeletalAnimatorEntity = transform.GetEntity()->GetTopEntity()->FindEntityInHierarchy(r3d.skeletalAnimatorEntityName))
					{
						skeletalAnimator = registry.try_get<SkeletalAnimator>(skeletalAnimatorEntity->GetHandle());
					}

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
	color.textureCreateInfo.format = Format::B10G11R11_UFLOAT_PACK32;
	color.textureCreateInfo.aspectMask = Texture::AspectMask::COLOR;
	color.textureCreateInfo.channels = 3;
	color.textureCreateInfo.isMultiBuffered = true;
	color.textureCreateInfo.usage = { Texture::Usage::SAMPLED, Texture::Usage::TRANSFER_SRC, Texture::Usage::COLOR_ATTACHMENT };
	color.textureCreateInfo.name = "BloomColor";
	color.textureCreateInfo.filepath = color.textureCreateInfo.name;
	color.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;
	color.load = RenderPass::Load::LOAD;
	color.store = RenderPass::Store::STORE;

	Texture::SamplerCreateInfo samplerCreateInfo{};
	samplerCreateInfo.addressMode = Texture::SamplerCreateInfo::AddressMode::CLAMP_TO_BORDER;
	samplerCreateInfo.borderColor = Texture::SamplerCreateInfo::BorderColor::FLOAT_OPAQUE_BLACK;
	samplerCreateInfo.maxAnisotropy = 1.0f;

	color.textureCreateInfo.samplerCreateInfo = samplerCreateInfo;

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
		PROFILER_SCOPE(Bloom);

		const std::string renderPassName = renderInfo.renderPass->GetName();
		const GraphicsSettings::Bloom& bloomSettings = renderInfo.scene->GetGraphicsSettings().bloom;
		const int mipCount = bloomSettings.mipCount;

		if (!bloomSettings.isEnabled)
		{
			renderInfo.renderView->DeleteCustomData("BloomData");
			renderInfo.renderView->DeleteFrameBuffer(renderPassName);
			for (int mipLevel = 0; mipLevel < mipCount; mipLevel++)
			{
				renderInfo.renderView->DeleteFrameBuffer("BloomFrameBuffers(" + std::to_string(mipLevel) + ")");
				renderInfo.renderView->DeleteBuffer("BloomBuffers(" + std::to_string(mipLevel) + ")");
				renderInfo.renderView->DeleteUniformWriter("BloomDownUniformWriters(" + std::to_string(mipLevel) + ")");
				renderInfo.renderView->DeleteUniformWriter("BloomUpUniformWriters(" + std::to_string(mipLevel) + ")");
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
		BloomData* bloomData = (BloomData*)renderInfo.renderView->GetCustomData("BloomData");
		if (!bloomData)
		{
			bloomData = new BloomData();
			bloomData->mipCount = mipCount;
			bloomData->sourceSize = viewportSize;
			renderInfo.renderView->SetCustomData("BloomData", bloomData);
		
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
			PROFILER_SCOPE("DownSample");

			const std::shared_ptr<BaseMaterial> baseMaterial = MaterialManager::GetInstance().LoadBaseMaterial(
				std::filesystem::path("Materials") / "BloomDownSample.basemat");
			const std::shared_ptr<Pipeline> pipeline = baseMaterial->GetPipeline(renderPassName);
			if (!pipeline)
			{
				return;
			}

			// Create FrameBuffers.
			if (recreateResources || !renderInfo.renderView->GetFrameBuffer(renderPassName))
			{
				glm::ivec2 size = viewportSize;
				for (int mipLevel = 0; mipLevel < mipCount; mipLevel++)
				{
					const std::string mipLevelString = std::to_string(mipLevel);

					// NOTE: There are parentheses in the name because square brackets are used in the base material file to find the attachment index.
					renderInfo.renderView->SetFrameBuffer("BloomFrameBuffers(" + mipLevelString + ")", FrameBuffer::Create(renderInfo.renderPass, renderInfo.renderView.get(), size));

					size.x = glm::max(size.x / 2, 4);
					size.y = glm::max(size.y / 2, 4);
				}

				// For viewport visualization and easy access by render pass name.
				renderInfo.renderView->SetFrameBuffer(renderPassName, renderInfo.renderView->GetFrameBuffer("BloomFrameBuffers(0)"));
			}

			// Create UniformWriters, Buffers for down sample pass.
			if (recreateResources)
			{
				for (int mipLevel = 0; mipLevel < mipCount; mipLevel++)
				{
					const std::string mipLevelString = std::to_string(mipLevel);

					const std::shared_ptr<UniformWriter> renderUniformWriter = GetOrCreateRendererUniformWriter(
						renderInfo.renderView, pipeline, renderPassName, "BloomDownUniformWriters[" + mipLevelString + "]");
					GetOrCreateRenderBuffer(renderInfo.renderView, renderUniformWriter, "MipBuffer", "BloomBuffers[" + mipLevelString + "]");
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
					const std::shared_ptr<FrameBuffer> frameBuffer = renderInfo.renderView->GetFrameBuffer("BloomFrameBuffers(" + std::to_string(mipLevel) + ")");
					frameBuffer->Resize(size);

					size.x = glm::max(size.x / 2, 1);
					size.y = glm::max(size.y / 2, 1);
				}
			}

			std::shared_ptr<Texture> sourceTexture = renderInfo.renderView->GetFrameBuffer(GBuffer)->GetAttachment(3);
			for (int mipLevel = 0; mipLevel < mipCount; mipLevel++)
			{
				const std::string mipLevelString = std::to_string(mipLevel);

				renderInfo.renderer->MemoryBarrierFragmentReadWrite(renderInfo.frame);

				const std::shared_ptr<FrameBuffer> frameBuffer = renderInfo.renderView->GetFrameBuffer("BloomFrameBuffers(" + mipLevelString + ")");

				RenderPass::SubmitInfo submitInfo{};
				submitInfo.frame = renderInfo.frame;
				submitInfo.renderPass = renderInfo.renderPass;
				submitInfo.frameBuffer = frameBuffer;
				renderInfo.renderer->BeginRenderPass(submitInfo, "Bloom Down Sample [" + mipLevelString + "]", { 1.0f, 1.0f, 0.0f });

				glm::vec2 sourceSize = submitInfo.frameBuffer->GetSize();
				const std::shared_ptr<Buffer> mipBuffer = renderInfo.renderView->GetBuffer("BloomBuffers[" + mipLevelString + "]");
				baseMaterial->WriteToBuffer(mipBuffer, "MipBuffer", "sourceSize", sourceSize);
				baseMaterial->WriteToBuffer(mipBuffer, "MipBuffer", "bloomIntensity", bloomSettings.intensity);
				baseMaterial->WriteToBuffer(mipBuffer, "MipBuffer", "mipLevel", mipLevel);

				mipBuffer->Flush();

				const std::shared_ptr<UniformWriter> downUniformWriter = renderInfo.renderView->GetUniformWriter("BloomDownUniformWriters[" + mipLevelString + "]");
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
			PROFILER_SCOPE("UpSample");

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
					GetOrCreateRendererUniformWriter(renderInfo.renderView, pipeline, renderPassName, "BloomUpUniformWriters[" + std::to_string(mipLevel) + "]");
				}
			}

			for (int mipLevel = mipCount - 1; mipLevel > 0; mipLevel--)
			{
				const std::string mipLevelString = std::to_string(mipLevel - 1);

				renderInfo.renderer->MemoryBarrierFragmentReadWrite(renderInfo.frame);

				RenderPass::SubmitInfo submitInfo{};
				submitInfo.frame = renderInfo.frame;
				submitInfo.renderPass = renderInfo.renderPass;
				submitInfo.frameBuffer = renderInfo.renderView->GetFrameBuffer("BloomFrameBuffers(" + mipLevelString + ")");
				renderInfo.renderer->BeginRenderPass(submitInfo, "Bloom Up Sample [" + std::to_string(mipLevel) + "]", { 1.0f, 1.0f, 0.0f });

				const std::shared_ptr<UniformWriter> downUniformWriter = renderInfo.renderView->GetUniformWriter("BloomUpUniformWriters[" + mipLevelString + "]");
				downUniformWriter->WriteTexture("sourceTexture", renderInfo.renderView->GetFrameBuffer("BloomFrameBuffers(" + std::to_string(mipLevel) + ")")->GetAttachment(0));
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
		PROFILER_SCOPE(SSR);

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

		std::shared_ptr<Texture> ssrTexture = renderInfo.renderView->GetStorageImage(passName);

		if (ssrSettings.isEnabled)
		{
			if (!ssrTexture)
			{
				ssrTexture = Texture::Create(createInfo);
				renderInfo.renderView->SetStorageImage(passName, ssrTexture);
				GetOrCreateRendererUniformWriter(renderInfo.renderView, pipeline, passName)->WriteTexture("outColor", ssrTexture);
			}
		}
		else
		{
			renderInfo.renderView->DeleteUniformWriter(passName);
			renderInfo.renderView->DeleteStorageImage(passName);
			renderInfo.renderView->DeleteBuffer(ssrBufferName);

			return;
		}

		if (currentViewportSize != ssrTexture->GetSize())
		{
			const std::shared_ptr<UniformWriter> renderUniformWriter = GetOrCreateRendererUniformWriter(renderInfo.renderView, pipeline, passName);
			auto callback = [renderUniformWriter, passName, createInfo, renderInfo]()
			{
				const std::shared_ptr<Texture> ssrTexture = Texture::Create(createInfo);
				renderInfo.renderView->SetStorageImage(passName, ssrTexture);
				renderUniformWriter->WriteTexture("outColor", ssrTexture);
			};

			std::shared_ptr<NextFrameEvent> resizeEvent = std::make_shared<NextFrameEvent>(callback, Event::Type::OnNextFrame, this);
			EventSystem::GetInstance().SendEvent(resizeEvent);
		}

		const std::shared_ptr<UniformWriter> renderUniformWriter = GetOrCreateRendererUniformWriter(renderInfo.renderView, pipeline, passName);

		WriteRenderViews(renderInfo.renderView, renderInfo.scene->GetRenderView(), pipeline, renderUniformWriter);

		const glm::vec2 viewportScale = glm::vec2(resolutionScales[ssrSettings.resolutionScale]);
		const std::shared_ptr<Buffer> ssrBuffer = GetOrCreateRenderBuffer(renderInfo.renderView, renderUniformWriter, ssrBufferName);
		baseMaterial->WriteToBuffer(ssrBuffer, ssrBufferName, "viewportScale", viewportScale);
		baseMaterial->WriteToBuffer(ssrBuffer, ssrBufferName, "maxDistance", ssrSettings.maxDistance);
		baseMaterial->WriteToBuffer(ssrBuffer, ssrBufferName, "resolution", ssrSettings.resolution);
		baseMaterial->WriteToBuffer(ssrBuffer, ssrBufferName, "stepCount", ssrSettings.stepCount);
		baseMaterial->WriteToBuffer(ssrBuffer, ssrBufferName, "thickness", ssrSettings.thickness);

		std::vector<std::shared_ptr<UniformWriter>> uniformWriters = GetUniformWriters(pipeline, baseMaterial, nullptr, renderInfo);
		if (FlushUniformWriters(uniformWriters))
		{
			renderInfo.renderer->BeginCommandLabel(passName, topLevelRenderPassDebugColor, renderInfo.frame);

			renderInfo.renderer->BeginCommandLabel(passName, { 1.0f, 1.0f, 0.0f }, renderInfo.frame);

			glm::uvec2 groupCount = currentViewportSize / glm::ivec2(16, 16);
			groupCount += glm::uvec2(1, 1);
			renderInfo.renderer->Dispatch(pipeline, { groupCount.x, groupCount.y, 1 }, uniformWriters, renderInfo.frame);

			renderInfo.renderer->EndCommandLabel(renderInfo.frame);
		}
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
		PROFILER_SCOPE(SSRBlur);

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

		std::shared_ptr<Texture> ssrBlurTexture = renderInfo.renderView->GetStorageImage(passName);

		if (ssrSettings.isEnabled)
		{
			if (!ssrBlurTexture)
			{
				ssrBlurTexture = Texture::Create(createInfo);
				renderInfo.renderView->SetStorageImage(passName, ssrBlurTexture);
				GetOrCreateRendererUniformWriter(renderInfo.renderView, pipeline, passName)->WriteTexture("outColor", ssrBlurTexture);
			}
		}
		else
		{
			renderInfo.renderView->DeleteUniformWriter(passName);
			renderInfo.renderView->DeleteStorageImage(passName);
			renderInfo.renderView->DeleteBuffer(ssrBufferName);

			return;
		}

		if (currentViewportSize != ssrBlurTexture->GetSize())
		{
			const std::shared_ptr<UniformWriter> renderUniformWriter = GetOrCreateRendererUniformWriter(renderInfo.renderView, pipeline, passName);
			auto callback = [renderUniformWriter, passName, createInfo, renderInfo]()
			{
				const std::shared_ptr<Texture> ssrBlurTexture = Texture::Create(createInfo);
				renderInfo.renderView->SetStorageImage(passName, ssrBlurTexture);
				renderUniformWriter->WriteTexture("outColor", ssrBlurTexture);
			};

			std::shared_ptr<NextFrameEvent> resizeEvent = std::make_shared<NextFrameEvent>(callback, Event::Type::OnNextFrame, this);
			EventSystem::GetInstance().SendEvent(resizeEvent);
		}

		const std::shared_ptr<UniformWriter> renderUniformWriter = GetOrCreateRendererUniformWriter(renderInfo.renderView, pipeline, passName);

		const std::shared_ptr<Buffer> ssrBuffer = GetOrCreateRenderBuffer(renderInfo.renderView, renderUniformWriter, ssrBufferName);
		baseMaterial->WriteToBuffer(ssrBuffer, ssrBufferName, "blurRange", ssrSettings.blurRange);
		baseMaterial->WriteToBuffer(ssrBuffer, ssrBufferName, "blurOffset", ssrSettings.blurOffset);

		WriteRenderViews(renderInfo.renderView, renderInfo.scene->GetRenderView(), pipeline, renderUniformWriter);

		std::vector<std::shared_ptr<UniformWriter>> uniformWriters = GetUniformWriters(pipeline, baseMaterial, nullptr, renderInfo);
		if (FlushUniformWriters(uniformWriters))
		{
			renderInfo.renderer->BeginCommandLabel(passName, { 1.0f, 1.0f, 0.0f }, renderInfo.frame);

			glm::uvec2 groupCount = currentViewportSize / glm::ivec2(16, 16);
			groupCount += glm::uvec2(1, 1);
			renderInfo.renderer->Dispatch(pipeline, { groupCount.x, groupCount.y, 1 }, uniformWriters, renderInfo.frame);

			renderInfo.renderer->EndCommandLabel(renderInfo.frame);

			renderInfo.renderer->EndCommandLabel(renderInfo.frame);
		}
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
		PROFILER_SCOPE(SSAO);

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

		SSAORenderer* ssaoRenderer = (SSAORenderer*)renderInfo.renderView->GetCustomData("SSAORenderer");
		if (ssaoSettings.isEnabled && !ssaoRenderer)
		{
			ssaoRenderer = new SSAORenderer();
			renderInfo.renderView->SetCustomData("SSAORenderer", ssaoRenderer);
		}

		const std::string ssaoBufferName = passName;
		std::shared_ptr<Texture> ssaoTexture = renderInfo.renderView->GetStorageImage(passName);

		if (!ssaoSettings.isEnabled)
		{
			renderInfo.renderView->DeleteUniformWriter(passName);
			renderInfo.renderView->DeleteCustomData("SSAORenderer");
			renderInfo.renderView->DeleteBuffer(ssaoBufferName);
			renderInfo.renderView->DeleteStorageImage(passName);

			return;
		}
		else
		{
			if (!renderInfo.renderView->GetStorageImage(passName))
			{
				ssaoTexture = Texture::Create(createInfo);
				renderInfo.renderView->SetStorageImage(passName, ssaoTexture);
				GetOrCreateRendererUniformWriter(renderInfo.renderView, pipeline, passName)->WriteTexture("outColor", ssaoTexture);
			}
		}

		if (currentViewportSize != ssaoTexture->GetSize())
		{
			const std::shared_ptr<UniformWriter> renderUniformWriter = GetOrCreateRendererUniformWriter(renderInfo.renderView, pipeline, passName);
			auto callback = [renderUniformWriter, passName, createInfo, renderInfo]()
			{
				const std::shared_ptr<Texture> ssaoTexture = Texture::Create(createInfo);
				renderInfo.renderView->SetStorageImage(passName, ssaoTexture);
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

		const std::shared_ptr<UniformWriter> renderUniformWriter = GetOrCreateRendererUniformWriter(renderInfo.renderView, pipeline, passName);
		const std::shared_ptr<Buffer> ssaoBuffer = GetOrCreateRenderBuffer(renderInfo.renderView, renderUniformWriter, ssaoBufferName);

		WriteRenderViews(renderInfo.renderView, renderInfo.scene->GetRenderView(), pipeline, renderUniformWriter);
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
		if (FlushUniformWriters(uniformWriters))
		{
			renderInfo.renderer->BeginCommandLabel(passName, topLevelRenderPassDebugColor, renderInfo.frame);

			renderInfo.renderer->BeginCommandLabel(passName, { 1.0f, 1.0f, 0.0f }, renderInfo.frame);

			glm::uvec2 groupCount = currentViewportSize / glm::ivec2(16, 16);
			groupCount += glm::uvec2(1, 1);
			renderInfo.renderer->Dispatch(pipeline, { groupCount.x, groupCount.y, 1 }, uniformWriters, renderInfo.frame);

			renderInfo.renderer->EndCommandLabel(renderInfo.frame);
		}
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
		PROFILER_SCOPE(SSAOBlur);

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

		std::shared_ptr<Texture> ssaoBlurTexture = renderInfo.renderView->GetStorageImage(passName);
		if (!ssaoSettings.isEnabled)
		{
			renderInfo.renderView->DeleteUniformWriter(passName);
			renderInfo.renderView->DeleteStorageImage(passName);

			return;
		}
		else
		{
			if (!renderInfo.renderView->GetStorageImage(passName))
			{
				ssaoBlurTexture = Texture::Create(createInfo);
				renderInfo.renderView->SetStorageImage(passName, ssaoBlurTexture);
				GetOrCreateRendererUniformWriter(renderInfo.renderView, pipeline, passName)->WriteTexture("outColor", ssaoBlurTexture);
			}
		}

		const std::shared_ptr<UniformWriter> renderUniformWriter = GetOrCreateRendererUniformWriter(renderInfo.renderView, pipeline, passName);
		if (currentViewportSize != ssaoBlurTexture->GetSize())
		{
			auto callback = [renderUniformWriter, passName, createInfo, renderInfo]()
			{
				const std::shared_ptr<Texture> ssaoBlurTexture = Texture::Create(createInfo);
				renderInfo.renderView->SetStorageImage(passName, ssaoBlurTexture);
				renderUniformWriter->WriteTexture("outColor", ssaoBlurTexture);
			};

			std::shared_ptr<NextFrameEvent> resizeEvent = std::make_shared<NextFrameEvent>(callback, Event::Type::OnNextFrame, this);
			EventSystem::GetInstance().SendEvent(resizeEvent);
		}

		WriteRenderViews(renderInfo.renderView, renderInfo.scene->GetRenderView(), pipeline, renderUniformWriter);

		std::vector<std::shared_ptr<UniformWriter>> uniformWriters = GetUniformWriters(pipeline, baseMaterial, nullptr, renderInfo);
		if (FlushUniformWriters(uniformWriters))
		{
			renderInfo.renderer->BeginCommandLabel(passName, { 1.0f, 1.0f, 0.0f }, renderInfo.frame);

			glm::uvec2 groupCount = currentViewportSize / glm::ivec2(16, 16);
			groupCount += glm::uvec2(1, 1);
			renderInfo.renderer->Dispatch(pipeline, { groupCount.x, groupCount.y, 1 }, uniformWriters, renderInfo.frame);

			renderInfo.renderer->EndCommandLabel(renderInfo.frame);

			renderInfo.renderer->EndCommandLabel(renderInfo.frame);
		}
	};

	CreateComputePass(createInfo);
}

void RenderPassManager::CreateUI()
{
	glm::vec4 clearColor = { 0.0f, 0.0f, 0.0f, 0.0f };

	RenderPass::AttachmentDescription color{};
	color.textureCreateInfo.format = Format::R8G8B8A8_UNORM;
	color.textureCreateInfo.aspectMask = Texture::AspectMask::COLOR;
	color.textureCreateInfo.channels = 4;
	color.textureCreateInfo.isMultiBuffered = true;
	color.textureCreateInfo.usage = { Texture::Usage::SAMPLED, Texture::Usage::TRANSFER_SRC, Texture::Usage::COLOR_ATTACHMENT };
	color.textureCreateInfo.name = "UIColor";
	color.textureCreateInfo.filepath = color.textureCreateInfo.name;
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
		PROFILER_SCOPE(UI);
		
		UIRenderer* uiRenderer = (UIRenderer*)renderInfo.renderView->GetCustomData("UIRenderer");
		if (!uiRenderer)
		{
			uiRenderer = new UIRenderer();

			renderInfo.renderView->SetCustomData("UIRenderer", uiRenderer);
		}

		const std::shared_ptr<BaseMaterial> baseMaterial = MaterialManager::GetInstance().LoadBaseMaterial("Materials/RectangleUI.basemat");
		const auto& view = renderInfo.scene->GetRegistry().view<Canvas>();
		std::vector<entt::entity> entities;
		entities.reserve(view.size());
		for (const auto& entity : view)
		{
			entities.emplace_back(entity);

			Canvas& canvas = renderInfo.scene->GetRegistry().get<Canvas>(entity);
			Renderer3D* r3d = renderInfo.scene->GetRegistry().try_get<Renderer3D>(entity);
			if (!canvas.drawInMainViewport)
			{
				if (canvas.size.x > 0 && canvas.size.y > 0)
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

					if (r3d)
					{
						r3d->isEnabled = true;
						r3d->material->GetUniformWriter(GBuffer)->WriteTexture("albedoTexture", canvas.frameBuffer->GetAttachment(0));
					}
				}
			}
			else
			{
				if (canvas.frameBuffer)
				{
					r3d->isEnabled = false;
					r3d->material->GetUniformWriter(GBuffer)->WriteTexture("albedoTexture", TextureManager::GetInstance().GetWhite());
					canvas.frameBuffer = nullptr;
				}
			}
		}

		uiRenderer->Render(entities, baseMaterial, renderInfo);
	};

	CreateRenderPass(createInfo);
}

void RenderPassManager::CreateDecalPass()
{
	glm::vec4 clearColor = { 0.1f, 0.1f, 0.1f, 1.0f };
	glm::vec4 clearShading = { 0.0f, 0.0f, 0.0f, 0.0f };
	glm::vec4 clearEmissive = { 0.0f, 0.0f, 0.0f, 0.0f };

	RenderPass::AttachmentDescription color = GetRenderPass(GBuffer)->GetAttachmentDescriptions()[0];
	color.load = RenderPass::Load::LOAD;
	color.store = RenderPass::Store::STORE;
	color.getFrameBufferCallback = [](RenderView* renderView)
	{
		return renderView->GetFrameBuffer(GBuffer)->GetAttachment(0);
	};

	RenderPass::AttachmentDescription shading = GetRenderPass(GBuffer)->GetAttachmentDescriptions()[2];
	shading.load = RenderPass::Load::LOAD;
	shading.store = RenderPass::Store::STORE;
	shading.getFrameBufferCallback = [](RenderView* renderView)
	{
		return renderView->GetFrameBuffer(GBuffer)->GetAttachment(2);
	};

	RenderPass::AttachmentDescription emissive = GetRenderPass(GBuffer)->GetAttachmentDescriptions()[3];
	emissive.load = RenderPass::Load::LOAD;
	emissive.store = RenderPass::Store::STORE;
	emissive.getFrameBufferCallback = [](RenderView* renderView)
	{
		return renderView->GetFrameBuffer(GBuffer)->GetAttachment(3);
	};

	Texture::SamplerCreateInfo emissiveSamplerCreateInfo{};
	emissiveSamplerCreateInfo.addressMode = Texture::SamplerCreateInfo::AddressMode::CLAMP_TO_BORDER;
	emissiveSamplerCreateInfo.borderColor = Texture::SamplerCreateInfo::BorderColor::FLOAT_OPAQUE_BLACK;
	emissiveSamplerCreateInfo.maxAnisotropy = 1.0f;

	emissive.textureCreateInfo.samplerCreateInfo = emissiveSamplerCreateInfo;

	RenderPass::CreateInfo createInfo{};
	createInfo.type = Pass::Type::GRAPHICS;
	createInfo.name = Decals;
	createInfo.clearColors = { clearColor, clearShading, clearEmissive };
	createInfo.attachmentDescriptions = { color, shading, emissive };
	createInfo.resizeWithViewport = true;
	createInfo.resizeViewportScale = { 1.0f, 1.0f };

	createInfo.executeCallback = [this](const RenderPass::RenderCallbackInfo& renderInfo)
	{
		PROFILER_SCOPE(Decals);

		const std::string renderPassName = renderInfo.renderPass->GetName();

		size_t renderableCount = 0;
		const std::shared_ptr<Scene> scene = renderInfo.scene;
		entt::registry& registry = scene->GetRegistry();
		const Camera& camera = renderInfo.camera->GetComponent<Camera>();
		const glm::mat4 viewProjectionMat4 = renderInfo.projection * camera.GetViewMat4();
		const auto r3dView = registry.view<Decal>();
		const auto unitCubeMesh = MeshManager::GetInstance().LoadMesh("UnitCube");

		std::unordered_map<std::shared_ptr<BaseMaterial>, std::unordered_map<std::shared_ptr<Material>, std::vector<entt::entity>>> renderableEntities;

		for (const entt::entity& entity : r3dView)
		{
			const Decal& decal = registry.get<Decal>(entity);
			if ((decal.objectVisibilityMask & camera.GetObjectVisibilityMask()) == 0)
			{
				continue;
			}

			const Transform& transform = registry.get<Transform>(entity);
			if (!transform.GetEntity()->IsEnabled())
			{
				continue;
			}

			if (!decal.material || !decal.material->IsPipelineEnabled(Decals))
			{
				continue;
			}

			const std::shared_ptr<Pipeline> pipeline = decal.material->GetBaseMaterial()->GetPipeline(renderPassName);
			if (!pipeline)
			{
				continue;
			}

			const glm::mat4& transformMat4 = transform.GetTransform();
			const BoundingBox& box = unitCubeMesh->GetBoundingBox();

			bool isInFrustum = FrustumCulling::CullBoundingBox(viewProjectionMat4, transformMat4, box.min, box.max, camera.GetZNear());
			if (!isInFrustum)
			{
				continue;
			}

			renderableEntities[decal.material->GetBaseMaterial()][decal.material].emplace_back(entity);

			renderableCount++;
		}

		struct DecalInstanceData
		{
			glm::mat4 transform;
			glm::mat4 inverseTransform;
		};

		std::shared_ptr<Buffer> instanceBuffer = renderInfo.renderView->GetBuffer("DecalInstanceBuffer");
		if ((renderableCount != 0 && !instanceBuffer) || (instanceBuffer && renderableCount != 0 && instanceBuffer->GetInstanceCount() < renderableCount))
		{
			instanceBuffer = Buffer::Create(
				sizeof(DecalInstanceData),
				renderableCount,
				Buffer::Usage::VERTEX_BUFFER,
				MemoryType::CPU,
				true);

			renderInfo.renderView->SetBuffer("DecalInstanceBuffer", instanceBuffer);
		}

		std::vector<DecalInstanceData> instanceDatas;

		const std::shared_ptr<BaseMaterial> decalBaseMaterial = MaterialManager::GetInstance().LoadBaseMaterial(
			std::filesystem::path("Materials") / "DecalBase.basemat");

		const std::shared_ptr<Pipeline> decalBasePipeline = decalBaseMaterial->GetPipeline(Decals);
		if (!decalBasePipeline)
		{
			return;
		}

		const std::shared_ptr<UniformWriter> renderUniformWriter = GetOrCreateRendererUniformWriter(renderInfo.renderView, decalBasePipeline, Decals);

		const std::shared_ptr<Texture> depthTexture = renderInfo.renderView->GetFrameBuffer(GBuffer)->GetAttachment(4);
		const std::shared_ptr<Texture> normalTexture = renderInfo.renderView->GetFrameBuffer(GBuffer)->GetAttachment(1);
		renderUniformWriter->WriteTexture("depthGBufferTexture", depthTexture);
		renderUniformWriter->WriteTexture("normalGBufferTexture", normalTexture);

		const std::shared_ptr<FrameBuffer> frameBuffer = renderInfo.renderView->GetFrameBuffer(renderPassName);

		RenderPass::SubmitInfo submitInfo{};
		submitInfo.frame = renderInfo.frame;
		submitInfo.renderPass = renderInfo.renderPass;
		submitInfo.frameBuffer = frameBuffer;
		renderInfo.renderer->BeginRenderPass(submitInfo);

		for (const auto& [baseMaterial, entitiesByMaterial] : renderableEntities)
		{
			const std::shared_ptr<Pipeline> pipeline = baseMaterial->GetPipeline(renderPassName);
			if (!pipeline)
			{
				continue;
			}

			for (const auto& [material, entities] : entitiesByMaterial)
			{
				const std::vector<std::shared_ptr<UniformWriter>> uniformWriters = GetUniformWriters(pipeline, baseMaterial, material, renderInfo);
				if (!FlushUniformWriters(uniformWriters))
				{
					continue;
				}

				const size_t instanceDataOffset = instanceDatas.size();

				for (const entt::entity& entity : entities)
				{
					DecalInstanceData data{};
					const Transform& transform = registry.get<Transform>(entity);
					data.transform = transform.GetTransform();
					data.inverseTransform = transform.GetInverseTransformMat4();
					instanceDatas.emplace_back(data);
				}

				std::vector<std::shared_ptr<Buffer>> vertexBuffers;
				std::vector<size_t> vertexBufferOffsets;
				GetVertexBuffers(pipeline, unitCubeMesh, vertexBuffers, vertexBufferOffsets);

				renderInfo.renderer->Render(
					vertexBuffers,
					vertexBufferOffsets,
					unitCubeMesh->GetIndexBuffer(),
					0,
					unitCubeMesh->GetIndexCount(),
					pipeline,
					instanceBuffer,
					instanceDataOffset * instanceBuffer->GetInstanceSize(),
					entities.size(),
					uniformWriters,
					renderInfo.frame);
			}
		}

		// Because these are all just commands and will be rendered later we can write the instance buffer
		// just once when all instance data is collected.
		if (instanceBuffer && !instanceDatas.empty())
		{
			instanceBuffer->WriteToBuffer(instanceDatas.data(), instanceDatas.size() * sizeof(DecalInstanceData));
			instanceBuffer->Flush();
		}

		renderInfo.renderer->EndRenderPass(submitInfo);
	};

	CreateRenderPass(createInfo);
}

void RenderPassManager::CreateToneMappingPass()
{
	glm::vec4 clearColor = { 1.0f, 1.0f, 1.0f, 1.0f };

	RenderPass::AttachmentDescription color{};
	color.textureCreateInfo.format = Format::R8G8B8A8_SRGB;
	color.textureCreateInfo.aspectMask = Texture::AspectMask::COLOR;
	color.textureCreateInfo.channels = 4;
	color.textureCreateInfo.isMultiBuffered = true;
	color.textureCreateInfo.usage = { Texture::Usage::SAMPLED, Texture::Usage::TRANSFER_SRC, Texture::Usage::COLOR_ATTACHMENT };
	color.textureCreateInfo.name = "ToneMappedColor";
	color.textureCreateInfo.filepath = color.textureCreateInfo.name;
	color.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;
	color.load = RenderPass::Load::DONT_CARE;
	color.store = RenderPass::Store::STORE;

	RenderPass::CreateInfo createInfo{};
	createInfo.type = Pass::Type::GRAPHICS;
	createInfo.name = ToneMapping;
	createInfo.clearColors = { clearColor };
	createInfo.attachmentDescriptions = { color };
	createInfo.resizeWithViewport = true;
	createInfo.resizeViewportScale = { 1.0f, 1.0f };

	createInfo.executeCallback = [](const RenderPass::RenderCallbackInfo& renderInfo)
	{
		PROFILER_SCOPE(ToneMapping);

		const std::shared_ptr<Mesh> plane = MeshManager::GetInstance().LoadMesh("FullScreenQuad");

		const std::string renderPassName = renderInfo.renderPass->GetName();

		const std::shared_ptr<BaseMaterial> baseMaterial = MaterialManager::GetInstance().LoadBaseMaterial("Materials/ToneMapping.basemat");
		const std::shared_ptr<Pipeline> pipeline = baseMaterial->GetPipeline(renderPassName);
		if (!pipeline)
		{
			return;
		}

		const GraphicsSettings& graphicsSettings = renderInfo.scene->GetGraphicsSettings();

		const std::shared_ptr<UniformWriter> renderUniformWriter = GetOrCreateRendererUniformWriter(renderInfo.renderView, pipeline, renderPassName);
		const std::string toneMappingBufferBufferName = "ToneMappingBuffer";
		const std::shared_ptr<Buffer> toneMappingBufferBuffer = GetOrCreateRenderBuffer(renderInfo.renderView, renderUniformWriter, toneMappingBufferBufferName);

		const int isSSREnabled = graphicsSettings.ssr.isEnabled;
		baseMaterial->WriteToBuffer(toneMappingBufferBuffer, toneMappingBufferBufferName, "toneMapperIndex", graphicsSettings.postProcess.toneMapper);
		baseMaterial->WriteToBuffer(toneMappingBufferBuffer, toneMappingBufferBufferName, "gamma", graphicsSettings.postProcess.gamma);
		baseMaterial->WriteToBuffer(toneMappingBufferBuffer, toneMappingBufferBufferName, "isSSREnabled", isSSREnabled);

		toneMappingBufferBuffer->Flush();

		WriteRenderViews(renderInfo.renderView, renderInfo.scene->GetRenderView(), pipeline, renderUniformWriter);

		const std::vector<std::shared_ptr<UniformWriter>> uniformWriters = GetUniformWriters(pipeline, baseMaterial, nullptr, renderInfo);
		if (!FlushUniformWriters(uniformWriters))
		{
			return;
		}

		const std::shared_ptr<FrameBuffer> frameBuffer = renderInfo.renderView->GetFrameBuffer(renderPassName);
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

void RenderPassManager::CreateAntiAliasingAndComposePass()
{
	glm::vec4 clearColor = { 1.0f, 1.0f, 1.0f, 1.0f };

	RenderPass::AttachmentDescription color{};
	color.textureCreateInfo.format = Format::R8G8B8A8_SRGB;
	color.textureCreateInfo.aspectMask = Texture::AspectMask::COLOR;
	color.textureCreateInfo.channels = 4;
	color.textureCreateInfo.isMultiBuffered = true;
	color.textureCreateInfo.usage = { Texture::Usage::SAMPLED, Texture::Usage::TRANSFER_SRC, Texture::Usage::COLOR_ATTACHMENT };
	color.textureCreateInfo.name = "AntiAliasingAndCompose";
	color.textureCreateInfo.filepath = color.textureCreateInfo.name;
	color.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;
	color.load = RenderPass::Load::DONT_CARE;
	color.store = RenderPass::Store::STORE;

	RenderPass::CreateInfo createInfo{};
	createInfo.type = Pass::Type::GRAPHICS;
	createInfo.name = AntiAliasingAndCompose;
	createInfo.clearColors = { clearColor };
	createInfo.attachmentDescriptions = { color };
	createInfo.resizeWithViewport = true;
	createInfo.resizeViewportScale = { 1.0f, 1.0f };

	createInfo.executeCallback = [](const RenderPass::RenderCallbackInfo& renderInfo)
	{
		PROFILER_SCOPE(AntiAliasingAndCompose);

		const std::shared_ptr<Mesh> plane = MeshManager::GetInstance().LoadMesh("FullScreenQuad");

		const std::string renderPassName = renderInfo.renderPass->GetName();

		const std::shared_ptr<BaseMaterial> baseMaterial = MaterialManager::GetInstance().LoadBaseMaterial("Materials/AntiAliasingAndCompose.basemat");
		const std::shared_ptr<Pipeline> pipeline = baseMaterial->GetPipeline(renderPassName);
		if (!pipeline)
		{
			return;
		}

		const GraphicsSettings& graphicsSettings = renderInfo.scene->GetGraphicsSettings();

		const std::shared_ptr<UniformWriter> renderUniformWriter = GetOrCreateRendererUniformWriter(renderInfo.renderView, pipeline, renderPassName);
		const std::string postProcessBufferName = "PostProcessBuffer";
		const std::shared_ptr<Buffer> postProcessBuffer = GetOrCreateRenderBuffer(renderInfo.renderView, renderUniformWriter, postProcessBufferName);

		const glm::vec2 viewportSize = renderInfo.viewportSize;
		const int fxaa = graphicsSettings.postProcess.fxaa;
		baseMaterial->WriteToBuffer(postProcessBuffer, "PostProcessBuffer", "viewportSize", viewportSize);
		baseMaterial->WriteToBuffer(postProcessBuffer, "PostProcessBuffer", "fxaa", fxaa);

		postProcessBuffer->Flush();

		WriteRenderViews(renderInfo.renderView, renderInfo.scene->GetRenderView(), pipeline, renderUniformWriter);

		const std::vector<std::shared_ptr<UniformWriter>> uniformWriters = GetUniformWriters(pipeline, baseMaterial, nullptr, renderInfo);
		if (!FlushUniformWriters(uniformWriters))
		{
			return;
		}

		const std::shared_ptr<FrameBuffer> frameBuffer = renderInfo.renderView->GetFrameBuffer(renderPassName);
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

bool RenderPassManager::FlushUniformWriters(const std::vector<std::shared_ptr<UniformWriter>>& uniformWriters)
{
	PROFILER_SCOPE(__FUNCTION__);

	for (const auto& uniformWriter : uniformWriters)
	{
		if (!uniformWriter)
		{
			return false;
		}

		uniformWriter->Flush();

		for (const auto& [name, buffers] : uniformWriter->GetBuffersByName())
		{
			for (const auto& buffer : buffers)
			{
				buffer->Flush();
			}
		}
	}

	return true;
}

void RenderPassManager::WriteRenderViews(
	std::shared_ptr<RenderView> cameraRenderView,
	std::shared_ptr<RenderView> sceneRenderView,
	std::shared_ptr<Pipeline> pipeline,
	std::shared_ptr<UniformWriter> uniformWriter)
{
	PROFILER_SCOPE(__FUNCTION__);

	for (const auto& [name, textureAttachmentInfo] : pipeline->GetUniformInfo().textureAttachmentsByName)
	{
		if (cameraRenderView)
		{
			const std::shared_ptr<FrameBuffer> frameBuffer = cameraRenderView->GetFrameBuffer(textureAttachmentInfo.name);
			if (frameBuffer)
			{
				uniformWriter->WriteTexture(name, frameBuffer->GetAttachment(textureAttachmentInfo.attachmentIndex));
				continue;
			}

			const std::shared_ptr<Texture> texture = cameraRenderView->GetStorageImage(textureAttachmentInfo.name);
			if (texture)
			{
				uniformWriter->WriteTexture(name, texture);
				continue;
			}
		}

		if (sceneRenderView)
		{
			const std::shared_ptr<FrameBuffer> frameBuffer = sceneRenderView->GetFrameBuffer(textureAttachmentInfo.name);
			if (frameBuffer)
			{
				uniformWriter->WriteTexture(name, frameBuffer->GetAttachment(textureAttachmentInfo.attachmentIndex));
				continue;
			}

			const std::shared_ptr<Texture> texture = sceneRenderView->GetStorageImage(textureAttachmentInfo.name);
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

std::shared_ptr<UniformWriter> RenderPassManager::GetOrCreateUniformWriter(
	std::shared_ptr<RenderView> renderView,
	std::shared_ptr<Pipeline> pipeline,
	Pipeline::DescriptorSetIndexType descriptorSetIndexType,
	const std::string& uniformWriterName,
	const std::string& uniformWriterIndexByName)
{
	PROFILER_SCOPE(__FUNCTION__);

	std::shared_ptr<UniformWriter> renderUniformWriter = renderView->GetUniformWriter(uniformWriterIndexByName.empty() ? uniformWriterName : uniformWriterIndexByName);
	if (!renderUniformWriter)
	{
		const std::shared_ptr<UniformLayout> renderUniformLayout =
			pipeline->GetUniformLayout(*pipeline->GetDescriptorSetIndexByType(descriptorSetIndexType, uniformWriterName));
		renderUniformWriter = UniformWriter::Create(renderUniformLayout);
		renderView->SetUniformWriter(uniformWriterIndexByName.empty() ? uniformWriterName : uniformWriterIndexByName, renderUniformWriter);
	}

	return renderUniformWriter;
}

std::shared_ptr<UniformWriter> RenderPassManager::GetOrCreateRendererUniformWriter(
	std::shared_ptr<RenderView> renderView,
	std::shared_ptr<Pipeline> pipeline,
	const std::string& uniformWriterName,
	const std::string& uniformWriterIndexByName)
{
	return GetOrCreateUniformWriter(renderView, pipeline, Pipeline::DescriptorSetIndexType::RENDERER, uniformWriterName, uniformWriterIndexByName);
}

std::shared_ptr<Buffer> RenderPassManager::GetOrCreateRenderBuffer(
	std::shared_ptr<RenderView> renderView,
	std::shared_ptr<UniformWriter> uniformWriter,
	const std::string& bufferName,
	const std::string& setBufferName)
{
	PROFILER_SCOPE(__FUNCTION__);

	std::shared_ptr<Buffer> buffer = renderView->GetBuffer(setBufferName.empty() ? bufferName : setBufferName);
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

		renderView->SetBuffer(setBufferName.empty() ? bufferName : setBufferName, buffer);
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
	PROFILER_SCOPE(__FUNCTION__);

	if (!skeletalAnimator->GetUniformWriter())
	{
		const std::shared_ptr<UniformLayout> renderUniformLayout =
			pipeline->GetUniformLayout(*pipeline->GetDescriptorSetIndexByType(Pipeline::DescriptorSetIndexType::OBJECT, GBuffer));
		skeletalAnimator->SetUniformWriter(UniformWriter::Create(renderUniformLayout));
	}

	if (!skeletalAnimator->GetBuffer())
	{
		auto binding = skeletalAnimator->GetUniformWriter()->GetUniformLayout()->GetBindingByName("BoneMatrices");
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

	baseMaterial->WriteToBuffer(skeletalAnimator->GetBuffer(), "BoneMatrices", "boneMatrices", *skeletalAnimator->GetFinalBoneMatrices().data());
	skeletalAnimator->GetBuffer()->Flush();
}
