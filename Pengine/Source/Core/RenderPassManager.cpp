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
	color.format = Texture::Format::B8G8R8A8_SRGB;
	color.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;

	RenderPass::AttachmentDescription normal{};
	normal.format = Texture::Format::R16G16B16A16_SFLOAT;
	normal.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;

	RenderPass::AttachmentDescription position{};
	position.format = Texture::Format::R16G16B16A16_SFLOAT;
	position.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;

	RenderPass::AttachmentDescription depth{};
	depth.format = Texture::Format::D32_SFLOAT;
	depth.layout = Texture::Layout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	RenderPass::CreateInfo createInfo{};
	createInfo.type = GBuffer;
	createInfo.clearColors = { clearColor, clearNormal, clearPosition };
	createInfo.clearDepths = { clearDepth };
	createInfo.attachmentDescriptions = { color, normal, position, depth };

	struct InstanceDataExplicit
	{
		glm::vec4 row1;
		glm::vec4 row2;
		glm::vec4 row3;
		glm::vec4 row4;
		glm::vec3 inverseRow1;
		glm::vec3 inverseRow2;
		glm::vec3 inverseRow3;
	};

	struct InstanceData
	{
		glm::mat4 transform;
		glm::mat3 inverseTransform;
	};

	createInfo.attributeDescriptions = Vertex::GetDefaultVertexAttributeDescriptions();
	createInfo.attributeDescriptions.emplace_back(Vertex::AttributeDescription{ 1, 5, Texture::Format::R32G32B32A32_SFLOAT, offsetof(InstanceDataExplicit, row1) });
	createInfo.attributeDescriptions.emplace_back(Vertex::AttributeDescription{ 1, 6, Texture::Format::R32G32B32A32_SFLOAT, offsetof(InstanceDataExplicit, row2) });
	createInfo.attributeDescriptions.emplace_back(Vertex::AttributeDescription{ 1, 7, Texture::Format::R32G32B32A32_SFLOAT, offsetof(InstanceDataExplicit, row3) });
	createInfo.attributeDescriptions.emplace_back(Vertex::AttributeDescription{ 1, 8, Texture::Format::R32G32B32A32_SFLOAT, offsetof(InstanceDataExplicit, row4) });
	createInfo.attributeDescriptions.emplace_back(Vertex::AttributeDescription{ 1, 9, Texture::Format::R32G32B32_SFLOAT, offsetof(InstanceDataExplicit, inverseRow1) });
	createInfo.attributeDescriptions.emplace_back(Vertex::AttributeDescription{ 1, 10, Texture::Format::R32G32B32_SFLOAT, offsetof(InstanceDataExplicit, inverseRow2) });
	createInfo.attributeDescriptions.emplace_back(Vertex::AttributeDescription{ 1, 11, Texture::Format::R32G32B32_SFLOAT, offsetof(InstanceDataExplicit, inverseRow3) });
	
	createInfo.bindingDescriptions = Vertex::GetDefaultVertexBindingDescriptions();
	createInfo.bindingDescriptions.emplace_back(Vertex::BindingDescription{ 1, sizeof(InstanceData), Vertex::InputRate::INSTANCE });

	UniformLayout::Binding binding;
	binding.name = "GlobalBuffer";
	binding.stages = { UniformLayout::Stage::VERTEX };
	binding.type = UniformLayout::Type::BUFFER;
	UniformLayout::Variable viewProjectionMat4;
	viewProjectionMat4.name = "viewProjectionMat4";
	viewProjectionMat4.type = "mat4";

	binding.values = { viewProjectionMat4 };
	createInfo.uniformBindings[0] = binding;

	struct GlobalData
	{
		glm::mat4 viewProjectionMat4;
	};

	//TODO: all these staging buffers make written to the gpu memory in the end of commands recording.
	createInfo.buffersByName["GlobalBuffer"] = Buffer::Create(sizeof(GlobalData), 1,
		{ Buffer::Usage::TRANSFER_SRC, Buffer::Usage::UNIFORM_BUFFER });

	createInfo.buffersByName["InstanceBuffer"] = nullptr;

	createInfo.createCallback = [](const RenderPass& renderPass)
	{
		renderPass.GetUniformWriter()->WriteBuffer("GlobalBuffer", renderPass.GetBuffer("GlobalBuffer"));
		renderPass.GetUniformWriter()->Flush();
	};

	createInfo.renderCallback = [](const RenderPass::RenderCallbackInfo& renderInfo)
	{
		GlobalData globalData{};
		globalData.viewProjectionMat4 = renderInfo.camera->GetComponent<Camera>().GetViewProjectionMat4();
		renderInfo.submitInfo.renderPass->GetBuffer("GlobalBuffer")->WriteToBuffer(&globalData);

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
			instanceBuffer = Buffer::Create(sizeof(InstanceData), renderableCount,
				{ Buffer::Usage::TRANSFER_SRC, Buffer::Usage::VERTEX_BUFFER });

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

			if (pipeline->GetUniformWriter())
			{
				pipeline->GetUniformWriter()->Flush();
			}

			for (const auto& [material, gameObjectsByMeshes] : meshesByMaterial)
			{
				const std::shared_ptr<UniformWriter> materialUniformWriter = material->GetUniformWriter(
					renderInfo.submitInfo.renderPass->GetType());
				materialUniformWriter->WriteTexture("albedoTexture", material->GetTexture("albedoTexture"));
				materialUniformWriter->WriteTexture("normalTexture", material->GetTexture("normalTexture"));
				materialUniformWriter->Flush();

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

					std::vector<std::shared_ptr<UniformWriter>> uniformWriters =
					{ 
						renderInfo.submitInfo.renderPass->GetUniformWriter(),
						materialUniformWriter
					};

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
			instanceBuffer->WriteToBuffer(instanceDatas.data());
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
	color.format = Texture::Format::R8G8B8A8_SRGB;
	color.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;

	RenderPass::CreateInfo createInfo{};
	createInfo.type = Deferred;
	createInfo.clearColors = { clearColor };
	createInfo.clearDepths = { clearDepth };
	createInfo.attachmentDescriptions = { color };

	createInfo.attributeDescriptions =
	{
		{ 0, 0, Texture::Format::R32G32B32_SFLOAT, offsetof(Vertex, position) },
		{ 0, 1, Texture::Format::R32G32_SFLOAT, offsetof(Vertex, uv) }
	};
	
	createInfo.bindingDescriptions =
	{
		{ 0, sizeof(Vertex), Vertex::InputRate::VERTEX }
	};

	const std::shared_ptr<Mesh> planeMesh = MeshManager::GetInstance().LoadMesh("Meshes/Plane.mesh");

	createInfo.renderCallback = [planeWeak = std::weak_ptr<Mesh>(planeMesh)](const RenderPass::RenderCallbackInfo& renderInfo)
	{
		const std::shared_ptr<Mesh> plane = planeWeak.lock();
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

		// TODO: Maybe move somewhere else on initialization!
		const std::shared_ptr<FrameBuffer> frameBuffer = renderInfo.renderer->GetRenderPassFrameBuffer(GBuffer);
		pipeline->GetUniformWriter()->WriteTexture("albedoTexture", frameBuffer->GetAttachment(0));
		pipeline->GetUniformWriter()->WriteTexture("normalTexture", frameBuffer->GetAttachment(1));
		pipeline->GetUniformWriter()->WriteTexture("positionTexture", frameBuffer->GetAttachment(2));

		auto pointLightView = renderInfo.scene->GetRegistry().view<PointLight>();
		for (const entt::entity& entity : pointLightView)
		{
			PointLight& pl = renderInfo.scene->GetRegistry().get<PointLight>(entity);
			const Transform& transform = renderInfo.scene->GetRegistry().get<Transform>(entity);
			baseMaterial->SetValue("Light", "color", pl.color);
			glm::vec3 lightPosition = transform.GetPosition();
			baseMaterial->SetValue("Light", "lightPosition", lightPosition);
			glm::vec3 cameraPosition = renderInfo.camera->GetComponent<Transform>().GetPosition();
			baseMaterial->SetValue("Light", "viewPosition", cameraPosition);
			baseMaterial->SetValue("Light", "linear", pl.linear);
			baseMaterial->SetValue("Light", "quadratic", pl.quadratic);
			baseMaterial->SetValue("Light", "constant", pl.constant);

			float use = 1.0f;
			baseMaterial->SetValue("Light", "use", use);
			break;
		}

		if (pointLightView.empty())
		{
			float use = 0.0f;
			baseMaterial->SetValue("Light", "use", use);
		}
		
		pipeline->GetUniformWriter()->Flush();

		std::vector<std::shared_ptr<UniformWriter>> uniformWriters =
		{
			pipeline->GetUniformWriter()
		};

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
