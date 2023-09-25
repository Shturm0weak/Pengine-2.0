#include "RenderPassManager.h"

#include "Camera.h"
#include "SceneManager.h"
#include "Logger.h"
#include "TextureSlots.h"

#include "../Components/Renderer3D.h"
#include "../Graphics/Material.h"
#include "../Graphics/Mesh.h"
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
	auto renderPassByName = m_RenderPassesByType.find(type);
	if (renderPassByName != m_RenderPassesByType.end())
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
	color.format = Texture::Format::R8G8B8A8_SRGB;
	color.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;

	RenderPass::AttachmentDescription normal{};
	normal.format = Texture::Format::R8G8B8A8_SRGB;
	normal.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;

	RenderPass::AttachmentDescription position{};
	position.format = Texture::Format::R8G8B8A8_SRGB;
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
		float materialIndex;
	};

	struct InstanceData
	{
		glm::mat4 transform;
	};

	createInfo.attributeDescriptions = Vertex::GetDefaultVertexAttributeDescriptions();
	createInfo.attributeDescriptions.emplace_back(Vertex::AttributeDescription{ 1, 3, Texture::Format::R32G32B32A32_SFLOAT, offsetof(InstanceDataExplicit, row1) });
	createInfo.attributeDescriptions.emplace_back(Vertex::AttributeDescription{ 1, 4, Texture::Format::R32G32B32A32_SFLOAT, offsetof(InstanceDataExplicit, row2) });
	createInfo.attributeDescriptions.emplace_back(Vertex::AttributeDescription{ 1, 5, Texture::Format::R32G32B32A32_SFLOAT, offsetof(InstanceDataExplicit, row3) });
	createInfo.attributeDescriptions.emplace_back(Vertex::AttributeDescription{ 1, 6, Texture::Format::R32G32B32A32_SFLOAT, offsetof(InstanceDataExplicit, row4) });
	
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
		std::vector<Buffer::Usage>{ Buffer::Usage::TRANSFER_SRC,
		Buffer::Usage::UNIFORM_BUFFER });

	createInfo.buffersByName["InstanceBuffer"] = nullptr;

	createInfo.createCallback = [](RenderPass& renderPass)
	{
		renderPass.GetUniformWriter()->WriteBuffer("GlobalBuffer", renderPass.GetBuffer("GlobalBuffer"));
		renderPass.GetUniformWriter()->Flush();
	};

	createInfo.renderCallback = [createInfo](RenderPass::RenderCallbackInfo renderInfo)
	{
		GlobalData globalData{};
		globalData.viewProjectionMat4 = renderInfo.camera->GetViewProjectionMat4();
		renderInfo.submitInfo.renderPass->GetBuffer("GlobalBuffer")->WriteToBuffer((void*)&globalData);

		using GameObjectsByMesh = std::unordered_map<std::shared_ptr<Mesh>, std::vector<GameObject*>>;
		using MeshesByMaterial = std::unordered_map<std::shared_ptr<Material>, GameObjectsByMesh>;
		using MaterialByBaseMaterial = std::unordered_map<std::shared_ptr<BaseMaterial>, MeshesByMaterial>;

		MaterialByBaseMaterial materialMeshGameObjects;

		size_t renderableCount = 0;
		std::vector<GameObject*> gameObjects = renderInfo.camera->GetScene()->GetGameObjects();
		for (const auto& gameObject : gameObjects)
		{
			Renderer3D* r3d = gameObject->m_ComponentManager.GetComponent<Renderer3D>();

			if (!r3d || !r3d->mesh || !r3d->material)
			{
				continue;
			}

			materialMeshGameObjects[r3d->material->GetBaseMaterial()][r3d->material][r3d->mesh].emplace_back(gameObject);

			renderableCount++;
		}

		std::shared_ptr<Buffer> instanceBuffer = renderInfo.submitInfo.renderPass->GetBuffer("InstanceBuffer");
		if ((renderableCount != 0 && !instanceBuffer) || (instanceBuffer && renderableCount != 0 && instanceBuffer->GetInstanceCount() != renderableCount))
		{
			instanceBuffer = Buffer::Create(sizeof(InstanceData), gameObjects.size(),
				std::vector<Buffer::Usage>{ Buffer::Usage::TRANSFER_SRC,
				Buffer::Usage::VERTEX_BUFFER });

			renderInfo.submitInfo.renderPass->SetBuffer("InstanceBuffer", instanceBuffer);
		}

		//TODO: revisit gbuffer rendering.
		TextureSlots globalTextureSlots;
		std::vector<InstanceData> instanceDatas;

		// Render all base materials -> materials -> meshes | put gameobjects into the instance buffer.
		for (const auto& [baseMaterial, meshesByMaterial] : materialMeshGameObjects)
		{
			std::shared_ptr<Pipeline> pipeline = baseMaterial->GetPipeline(renderInfo.submitInfo.renderPass->GetType());
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
				std::shared_ptr<UniformWriter> materialUniformWriter = material->GetUniformWriter(renderInfo.submitInfo.renderPass->GetType());
				materialUniformWriter->WriteTexture("albedo", material->GetTexture("albedo"));
				materialUniformWriter->Flush();

				for (const auto& [mesh, gameObjects] : gameObjectsByMeshes)
				{
					const size_t instanceDataOffset = instanceDatas.size();

					for (GameObject* gameObject : gameObjects)
					{
						InstanceData data;
						data.transform = gameObject->m_Transform.GetTransform();
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
						gameObjects.size(),
						uniformWriters,
						renderInfo.submitInfo);
				}
			}
		}

		// Because these are all just commands and will be rendered later we can write the instance buffer
		// just once when all instance data is collected.
		if (instanceBuffer)
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

	createInfo.attributeDescriptions = Vertex::GetDefaultVertexAttributeDescriptions();
	createInfo.bindingDescriptions = Vertex::GetDefaultVertexBindingDescriptions();

	//TODO: Fill the rendering code.

	Create(createInfo);
}
