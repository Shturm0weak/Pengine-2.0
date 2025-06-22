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
	CreateFinal();
	CreateSSAO();
	CreateSSAOBlur();
	CreateCSM();
	CreateBloom();
	CreateSSR();
	CreateSSRBlur();
	CreateUI();
	CreateRayTracing();
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

#include "Window.h"
#include "Input.h"
#include "Keycode.h"
#include "ThreadPool.h"
#include "Serializer.h"
#include "Timer.h"
#include "RandomGenerator.h"

void RenderPassManager::CreateRayTracing()
{
	glm::vec4 clearColor = { 0.1f, 0.1f, 0.1f, 1.0f };

	RenderPass::AttachmentDescription color{};
	color.textureCreateInfo.format = Format::R8G8B8A8_SRGB;
	color.textureCreateInfo.aspectMask = Texture::AspectMask::COLOR;
	color.textureCreateInfo.channels = 4;
	color.textureCreateInfo.isMultiBuffered = true;
	color.textureCreateInfo.usage = { Texture::Usage::SAMPLED, Texture::Usage::TRANSFER_SRC, Texture::Usage::COLOR_ATTACHMENT };
	color.textureCreateInfo.name = "RayTracingColor";
	color.textureCreateInfo.filepath = color.textureCreateInfo.name;
	color.textureCreateInfo.memoryType = MemoryType::CPU;
	color.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;
	color.load = RenderPass::Load::CLEAR;
	color.store = RenderPass::Store::STORE;

	RenderPass::CreateInfo createInfo{};
	createInfo.type = Pass::Type::GRAPHICS;
	createInfo.name = RayTracing;
	createInfo.clearColors = { clearColor };
	createInfo.attachmentDescriptions = { color };
	createInfo.resizeWithViewport = true;
	//createInfo.resizeViewportScale = { 0.1f, 0.1f };
	createInfo.resizeViewportScale = { 1.0f, 1.0f };

	createInfo.executeCallback = [this](const RenderPass::RenderCallbackInfo& renderInfo)
	{
		PROFILER_SCOPE(RayTracing);

		const std::shared_ptr<Viewport> viewport = renderInfo.window->GetViewportManager().GetViewport("Main");
		if (!viewport || !viewport->GetCamera().lock())
		{
			return;
		}

		if (Input::GetInstance(renderInfo.window.get()).IsKeyReleased(Keycode::KEY_J))
		{
			m_IsRayTracingEnabled = !m_IsRayTracingEnabled;
		}

		if (!m_IsRayTracingEnabled)
		{
			return;
		}

		Timer timer("Timer", false);

		struct DefaultMaterial
		{
			glm::vec4 albedoColor;
			glm::vec4 emissiveColor;
			glm::vec4 uvTransform;
			float metallicFactor;
			float roughnessFactor;
			float aoFactor;
			float emissiveFactor;
			int useNormalMap;
			int useSingleShadingMap;
		};

		std::mutex readyLock;
		std::atomic<size_t> tileFinishedCount;
		std::condition_variable readyConditionalVariable;

		tileFinishedCount.store(0);

		const std::shared_ptr<Entity> camera = viewport->GetCamera().lock();
		Camera& cameraComponent = camera->GetComponent<Camera>();
		Transform& cameraTransform = camera->GetComponent<Transform>();

		const glm::vec3 cameraPosition = cameraTransform.GetPosition();

		auto vec4ToR8G8B8A8 = [](const glm::vec4& color) -> uint32_t
		{
			uint8_t r = static_cast<uint8_t>(color[0] * 255.0f);
			uint8_t g = static_cast<uint8_t>(color[1] * 255.0f);
			uint8_t b = static_cast<uint8_t>(color[2] * 255.0f);
			uint8_t a = static_cast<uint8_t>(color[3] * 255.0f);

			return (a << 24) | (b << 16) | (g << 8) | r;
		};

		auto sampleTexture = [](const std::shared_ptr<Texture>& texture,
			const glm::vec2& uv,
			bool repeat = true) -> glm::vec4
		{
			// Handle texture coordinate wrapping
			glm::vec2 coord = uv;
			if (repeat)
			{
				coord.x = glm::fract(coord.x);
				coord.y = glm::fract(coord.y);
			}
			else
			{
				// Clamp to edge
				coord.x = glm::clamp(coord.x, 0.0f, 1.0f);
				coord.y = glm::clamp(coord.y, 0.0f, 1.0f);
			}

			const glm::ivec3 size = { texture->GetSize().x, texture->GetSize().y, texture->GetChannels() };

			// Convert UV to pixel coordinates
			float x = coord.x * (size.x - 1);
			float y = coord.y * (size.y - 1);

			// Nearest neighbor sampling
			int xi = static_cast<int>(glm::round(x));
			int yi = static_cast<int>(glm::round(y));

			// Ensure coordinates are within bounds
			xi = glm::clamp(xi, 0, size.x - 1);
			yi = glm::clamp(yi, 0, size.y - 1);

			auto subresourceLayout = texture->GetSubresourceLayout();
			uint8_t* data = (uint8_t*)texture->GetData();
			data += subresourceLayout.offset;

			// Calculate index in 1D array
			int index = (yi * size.x + xi) * size.z;

			glm::vec4 color =
			{
				(float)data[index + 0] / 255.0f,
				(float)data[index + 1] / 255.0f,
				(float)data[index + 2] / 255.0f,
				(float)data[index + 3] / 255.0f,
			};

			return color;
		};

		auto fresnelSchlick = [](float HdotV, const glm::vec3& basicReflectivity) -> glm::vec3
		{
			return basicReflectivity + (1.0f - basicReflectivity) * glm::pow<float>(glm::clamp<float>(1.0f - HdotV, 0.0f, 1.0f), 5.0f);
		};

		auto distributionGGX = [](float NdotH, float roughness) -> float
		{
			float a = roughness * roughness;
			float a2 = a * a;
			float denom = NdotH * NdotH * (a2 - 1.0) + 1.0;
			denom = glm::pi<float>() * denom * denom;
			return a2 / glm::max(denom, 0.0000001f);
		};

		auto geometrySmith = [](float NdotV, float NdotL, float roughness) -> float
		{
			float r = roughness + 1.0;
			float k = (r * r) / 8.0;
			float ggx1 = NdotV / (NdotV * (1.0 - k) + k);
			float ggx2 = NdotL / (NdotL * (1.0 - k) + k);

			return ggx1 * ggx2;
		};

		auto calculatePointLight = [&distributionGGX, &geometrySmith, &fresnelSchlick](
			const PointLight& light,
			const glm::vec3& directionNotNormalized,
			const glm::vec3& viewDirection,
			const glm::vec3& normal,
			const glm::vec3& basicReflectivity,
			const glm::vec3& albedo,
			float metallic,
			float roughness,
			float shadow) -> glm::vec3
		{
			glm::vec3 direction = glm::normalize(directionNotNormalized);
			glm::vec3 H = glm::normalize(viewDirection + direction);

			glm::vec3 radiance = light.color * light.intensity;

			float NdotV = glm::max(glm::dot(normal, viewDirection), 0.0000001f);
			float NdotL = glm::max(glm::dot(normal, direction), 0.0000001f);
			float HdotV = glm::max(glm::dot(H, viewDirection), 0.0f);
			float NdotH = glm::max(glm::dot(normal, H), 0.0f);

			float D = distributionGGX(NdotH, roughness);
			float G = geometrySmith(NdotV, NdotL, roughness);
			glm::vec3 F = fresnelSchlick(HdotV, basicReflectivity);

			glm::vec3 specular = D * G * F;
			specular /= 4.0 * NdotV * NdotL;// + 0.0001;

			glm::vec3 kS = F;
			glm::vec3 kD = glm::vec3(1.0f) - kS;

			kD *= 1.0f - metallic;

			float distance = length(directionNotNormalized);
			float attenuation = glm::max(0.0f,
				(1.0f / (distance * distance)) - (1.0f / (light.radius * light.radius)));

			return /*ambient * albedo + */(glm::vec3(1.0f) - shadow) * (kD * albedo / glm::pi<float>() + specular) * radiance * NdotL * attenuation;
		};

		auto calculateDirectionalLight = [&fresnelSchlick, &distributionGGX, &geometrySmith](
			const DirectionalLight& light,
			const glm::vec3& direction,
			const glm::vec3& viewDirection,
			const glm::vec3& normal,
			const glm::vec3& basicReflectivity,
			const glm::vec3& albedo,
			float metallic,
			float roughness,
			float shadow) -> glm::vec3
		{
			glm::vec3 H = glm::normalize(viewDirection + direction);

			glm::vec3 radiance = light.color * light.intensity;
			glm::vec3 ambient = light.ambient * radiance;

			float NdotV = glm::max(glm::dot(normal, viewDirection), 0.0000001f);
			float NdotL = glm::max(glm::dot(normal, direction), 0.0000001f);
			float HdotV = glm::max(glm::dot(H, viewDirection), 0.0f);
			float NdotH = glm::max(glm::dot(normal, H), 0.0f);

			float D = distributionGGX(NdotH, roughness);
			float G = geometrySmith(NdotV, NdotL, roughness);
			glm::vec3 F = fresnelSchlick(HdotV, basicReflectivity);

			glm::vec3 specular = D * G * F;
			specular /= 4.0 * NdotV * NdotL;// + 0.0001;

			glm::vec3 kS = F;
			glm::vec3 kD = glm::vec3(1.0f) - kS;

			kD *= 1.0f - metallic;

			return /*ambient * albedo + */(glm::vec3(1.0f) - shadow) * (kD * albedo / glm::pi<float>() + specular) * radiance * NdotL;
		};

		auto calculateShadow = [&sampleTexture](
			const glm::vec3 hitPoint,
			const glm::vec3 direction,
			const float length,
			const std::shared_ptr<Scene>& scene) -> float
		{
			const glm::vec3 start = hitPoint + (direction * 0.001f);
			auto hits = Raycast::RaycastScene(scene, start, direction, length);
			for (const auto& [hit, entity] : hits)
			{
				const Renderer3D& shadowR3d = entity->GetComponent<Renderer3D>();
				if (!shadowR3d.material)
				{
					continue;
				}

				const std::vector<std::shared_ptr<Texture>> shadowAlbedoTexture = shadowR3d.material->GetUniformWriter(GBuffer)->GetTexture("albedoTexture");

				if (!shadowAlbedoTexture.empty())
				{
					const glm::vec4 albedoTextureColor = sampleTexture(shadowAlbedoTexture.front(), hit.uv);
					if (albedoTextureColor.a < 1.0f)
					{
						continue;
					}
				}

				return 1.0f;
			}

			return 0.0f;
		};

		const auto& directionalLightView = renderInfo.scene->GetRegistry().view<DirectionalLight>();
		const auto& pointLightView = renderInfo.scene->GetRegistry().view<PointLight>();

		std::function<glm::vec3(
			const glm::vec3&,
			const glm::vec3&,
			const std::shared_ptr<Scene>&,
			int&,
			int,
			int,
			int)> calculateFinalColor;

		std::function<glm::vec3(
			const Raycast::Hit&,
			const std::shared_ptr<Scene>&,
			const glm::vec3&,
			int,
			int)> calculateIndirectLighting;

		calculateFinalColor = [&calculateFinalColor, &calculateShadow, &calculateDirectionalLight, &calculatePointLight,
			&cameraPosition, &directionalLightView, &sampleTexture, &calculateIndirectLighting, &pointLightView](
			const glm::vec3& start,
			const glm::vec3& direction,
			const std::shared_ptr<Scene>& scene,
			int& bounceIndex,
			int maxBounceCount = 1,
			int inderectDepth = 1,
			int indirectRayCount = 1) -> glm::vec3
		{
			auto hits = Raycast::RaycastScene(scene, start, direction, 100.0f);
			for (auto& [hit, entity] : hits)
			{
				const Renderer3D& r3d = entity->GetComponent<Renderer3D>();
				if (!r3d.material)
				{
					continue;
				}

				DefaultMaterial& materialData = *(DefaultMaterial*)r3d.material->GetBuffer("GBufferMaterial")->GetData();

				glm::vec3 albedoColor = glm::vec3(materialData.albedoColor);

				const std::vector<std::shared_ptr<Texture>> albedoTexture = r3d.material->GetUniformWriter(GBuffer)->GetTexture("albedoTexture");
				if (!albedoTexture.empty())
				{
					const glm::vec4 albedoTextureColor = sampleTexture(albedoTexture.front(), hit.uv);
					if (albedoTextureColor.a < 1.0f)
					{
						continue;
					}
					albedoColor *= (glm::vec3)albedoTextureColor;
				}

				glm::vec3 finalColor{};

				if (materialData.emissiveFactor > 0.0f)
				{
					glm::vec3 emissiveColor = materialData.emissiveColor * materialData.emissiveFactor * 2.0f;
					const std::vector<std::shared_ptr<Texture>> emissiveTexture = r3d.material->GetUniformWriter(GBuffer)->GetTexture("emissiveTexture");
					if (!emissiveTexture.empty())
					{
						const glm::vec4 emissiveTextureColor = sampleTexture(emissiveTexture.front(), hit.uv);
						if (emissiveTextureColor.a < 1.0f)
						{
							continue;
						}
						emissiveColor *= (glm::vec3)emissiveTextureColor;
					}

					return emissiveColor;
				}

				if (materialData.roughnessFactor == 0.0f)
				{
					if (bounceIndex < maxBounceCount)
					{
						bounceIndex++;
						const glm::vec3 newDirection = glm::reflect(direction, hit.normal);
						finalColor = calculateFinalColor(
							hit.point + newDirection * 0.001f, newDirection,
							scene, bounceIndex, maxBounceCount, inderectDepth, indirectRayCount);

						return glm::clamp(finalColor, 0.0f, 1.0f);
					}
				}

				for (auto handle : pointLightView)
				{
					PointLight& light = scene->GetRegistry().get<PointLight>(handle);
					Transform& lightTransform = scene->GetRegistry().get<Transform>(handle);

					if (glm::distance(hit.point, lightTransform.GetPosition()) > light.radius)
					{
						continue;
					}

					albedoColor *= light.color;

					const glm::vec3 directionNotNormalized = lightTransform.GetPosition() - hit.point;
					const float distance = glm::length(directionNotNormalized) * 0.9f;
					const float shadow = calculateShadow(hit.point, glm::normalize(directionNotNormalized), distance, scene);
					const glm::vec3 basicReflectivity = glm::mix(glm::vec3(0.05f), albedoColor, materialData.metallicFactor);
					finalColor += calculatePointLight(light, directionNotNormalized, glm::normalize(cameraPosition - hit.point), hit.normal, basicReflectivity,
						albedoColor, materialData.metallicFactor, materialData.roughnessFactor, shadow);
				}

				for (auto handle : directionalLightView)
				{
					DirectionalLight& light = scene->GetRegistry().get<DirectionalLight>(handle);
					Transform& lightTransform = scene->GetRegistry().get<Transform>(handle);

					albedoColor *= light.color;

					const float shadow = calculateShadow(hit.point, lightTransform.GetForward(), 1000.0f, scene);
					const glm::vec3 basicReflectivity = glm::mix(glm::vec3(0.05f), albedoColor, materialData.metallicFactor);
					finalColor += calculateDirectionalLight(light, lightTransform.GetForward(), glm::normalize(cameraPosition - hit.point), hit.normal, basicReflectivity,
						albedoColor, materialData.metallicFactor, materialData.roughnessFactor, shadow);
				}
				
				glm::vec3 indirect{};

				if (indirectRayCount > 0)
				{
					for (size_t i = 0; i < indirectRayCount; i++)
					{
						indirect += calculateIndirectLighting(hit, scene, albedoColor, inderectDepth, indirectRayCount);
					}

					indirect /= indirectRayCount;
				}

				return glm::clamp(indirect + finalColor, 0.0f, 1.0f);
			}

			return {};/*{ 1.0f, 1.0f, 1.0f };*/// { 0.5f, 0.7f, 0.7f };
		};

		auto randomHemisphereDirection = [](const glm::vec3& normal) -> glm::vec3
		{
			static std::mt19937 generator;
			std::uniform_real_distribution<float> distribution(0.0f, 1.0f);

			// Create orthogonal coordinate system
			glm::vec3 tangent = glm::normalize(glm::cross(
				normal,
				glm::abs(normal.x) > 0.9f ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0)
			));
			glm::vec3 bitangent = glm::cross(normal, tangent);

			// Cosine-weighted hemisphere sampling
			float r1 = distribution(generator);
			float r2 = distribution(generator);
			float sinTheta = sqrtf(1.0f - r1);
			float phi = 2.0f * glm::pi<float>() * r2;

			glm::vec3 localDir(
				sinTheta * cosf(phi),
				sqrtf(r1), // cos(theta)
				sinTheta * sinf(phi)
			);

			// Transform to world space
			return localDir.x * tangent + localDir.y * normal + localDir.z * bitangent;
		};

		calculateIndirectLighting = [&randomHemisphereDirection, &calculateFinalColor](
			const Raycast::Hit& hit,
			const std::shared_ptr<Scene>& scene,
			const glm::vec3& albedo,
			int depth,
			int indirectRayCount) -> glm::vec3
		{
			if (depth <= 0) return glm::vec3(0.0f);

			// Russian Roulette for termination
			const float continueProbability = 0.8f;
			if (RandomGenerator::GetInstance().Get<float>(0.0f, 1.0f) > continueProbability)
			{
				return glm::vec3(0.0f);
			}

			// Generate random direction on hemisphere
			glm::vec3 randomDirection = randomHemisphereDirection(hit.normal);

			// Offset origin to prevent self-intersection
			glm::vec3 start = hit.point + hit.normal * 0.01f;
			
			// Recursive ray cast
			int bounceIndex = 0;
			const glm::vec3 incomingRadiance = calculateFinalColor(
				start, randomDirection, scene, bounceIndex, 0, depth - 1, indirectRayCount);

			float cosTheta = glm::dot(hit.normal, randomDirection);
			glm::vec3 brdf = albedo / glm::pi<float>();

			return (brdf * incomingRadiance * cosTheta) / ((1.0f / glm::pi<float>()) * continueProbability);
		};

		const std::string renderPassName = renderInfo.renderPass->GetName();

		const std::shared_ptr<Texture> outColor = renderInfo.renderView->GetFrameBuffer(renderPassName)->GetAttachment(0);
		auto subresourceLayout = outColor->GetSubresourceLayout();
		uint8_t* data = (uint8_t*)outColor->GetData();
		data += subresourceLayout.offset;

		const glm::ivec3 size = { outColor->GetSize().x, outColor->GetSize().y, outColor->GetChannels() };
		const size_t threadCount = ThreadPool::GetInstance().GetThreadsAmount();
		const size_t tileSize = 64;
		const size_t tileCount = glm::ceil((float)size.y / (float)tileSize) * glm::ceil((float)size.x / (float)tileSize);
		for (size_t y = 0; y < size.y; y+=tileSize)
		{
			for (size_t x = 0; x < size.x; x+=tileSize)
			{
				ThreadPool::GetInstance().EnqueueAsync([
					xStart = x, yStart = y, tileSize, data, size, subresourceLayout,
					viewport, cameraPosition, &directionalLightView, &renderInfo, &tileCount,
					&tileFinishedCount, &readyConditionalVariable,
					&vec4ToR8G8B8A8, &sampleTexture, &calculateDirectionalLight,
					&calculateShadow, &calculateFinalColor, &timer]()
				{
					for (int y = yStart; y < yStart + tileSize && y < size.y; y++)
					{
						uint32_t* row = reinterpret_cast<uint32_t*>(data + y * subresourceLayout.rowPitch);
						for (int x = xStart; x < xStart + tileSize && x < size.x; x++)
						{
							const glm::vec2 normalizedPixelPosition = { (float)x / (float)size.x, (float)y / (float)size.y };
							const glm::vec3 mouseRay = viewport->GetMouseRay(normalizedPixelPosition * (glm::vec2)renderInfo.viewportSize);

							int bounceIndex = 0;
							const glm::vec3 finalColor = calculateFinalColor(
								cameraPosition, mouseRay, renderInfo.scene, bounceIndex, 10, 2, 24);

							row[x] = vec4ToR8G8B8A8({ glm::clamp(finalColor, 0.0f, 1.0f), 1.0f });
						}
					}

					Logger::Log(std::format("{:.3f}% {:.3f} sec", (((float)tileFinishedCount.load() + 1.0f) / tileCount) * 100, timer.Stop() / 1000.0f));
					tileFinishedCount.fetch_add(1);

					//readyConditionalVariable.notify_one();
				});
			}
		}

		/*for (uint32_t t = 0; t < threadCount; ++t)
		{
			const uint32_t startY = t * rowsPerThread;
			const uint32_t endY = (t == threadCount - 1) ? size.y : (t + 1) * rowsPerThread;

			ThreadPool::GetInstance().EnqueueAsync([
				startY, endY, data, size, t, subresourceLayout,
				viewport, cameraPosition, &directionalLightView, &renderInfo,
				&threadFinishedCount, &readyConditionalVariable,
				&vec4ToR8G8B8A8, &sampleTexture, &calculateDirectionalLight,
				&calculateShadow, &calculateFinalColor]()
			{
				for (uint32_t y = startY; y < endY; ++y)
				{
					uint32_t* row = reinterpret_cast<uint32_t*>(data + y * subresourceLayout.rowPitch);
					for (uint32_t x = 0; x < size.x; ++x)
					{
						const glm::vec2 normalizedPixelPosition = { (float)x / (float)size.x, (float)y / (float)size.y };
						const glm::vec3 mouseRay = viewport->GetMouseRay(normalizedPixelPosition * (glm::vec2)renderInfo.viewportSize);
						
						int bounceIndex = 0;
						const glm::vec3 finalColor = calculateFinalColor(
							cameraPosition, mouseRay, renderInfo.scene, bounceIndex, 10, 5, 2);

						row[x] = vec4ToR8G8B8A8({ finalColor, 1.0f });
					}

					Logger::Log(std::format("{} {}", t, (float)(y - startY) / (float)(endY - startY)));
				}

				threadFinishedCount.fetch_add(1);
				readyConditionalVariable.notify_one();
			});
		}*/

		volatile size_t i = 0;
		while (tileFinishedCount.load() != tileCount)
		{
			i++;
		}

		Logger::Warning("Finished");

		bool isLoaded = false;
		Serializer::SerializeTexture("RayTracing.png", outColor, &isLoaded);
		
		i = 0;
		while (isLoaded == false)
		{
			i++;
		}

		//std::unique_lock<std::mutex> lock(readyLock);
		//readyConditionalVariable.wait(lock, [&threadFinishedCount, threadCount]()
		//{
		//	return threadFinishedCount.load() == threadCount;
		//});
	};

	CreateRenderPass(createInfo);
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

			if (scene->GetSettings().m_DrawBoundingBoxes)
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
	normal.textureCreateInfo.usage = { Texture::Usage::SAMPLED, Texture::Usage::TRANSFER_SRC, Texture::Usage::COLOR_ATTACHMENT };
	normal.textureCreateInfo.name = "GBufferNormal";
	normal.textureCreateInfo.filepath = normal.textureCreateInfo.name;
	normal.layout = Texture::Layout::COLOR_ATTACHMENT_OPTIMAL;
	normal.load = RenderPass::Load::CLEAR;
	normal.store = RenderPass::Store::STORE;

	RenderPass::AttachmentDescription shading{};
	shading.textureCreateInfo.format = Format::R8G8B8A8_SRGB;
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
	emissive.textureCreateInfo.format = Format::R16G16B16A16_SFLOAT;
	emissive.textureCreateInfo.aspectMask = Texture::AspectMask::COLOR;
	emissive.textureCreateInfo.channels = 4;
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
				renderInfo.scene->GetVisualizer().DrawSphere(transform.GetPosition(), pl.radius, 10, color, 0.0f);
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

				std::vector<glm::vec4> shadowCascadeLevels(maxCascadeCount, {});
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
		
		std::vector<std::shared_ptr<UniformWriter>> uniformWriters;

		// TODO: Maybe fix, very slow.
		// Here we need Transition to flush uniform writers, because to update storage image the layout should be GENERAL.
		// And before and after the dispatch we need Transition again, because this command buffer will be executed later, so we don't know what can happen before.
		{
			PROFILER_SCOPE("Deferred Color Output Transition Layout");

			void* transitionFrame = device->CreateFrame();

			device->BeginFrame(transitionFrame);
			renderInfo.renderer->BeginCommandLabel("Layout Transition", topLevelRenderPassDebugColor, transitionFrame);
			colorTexture->Transition(Texture::Layout::GENERAL, transitionFrame);
			emissiveTexture->Transition(Texture::Layout::GENERAL, transitionFrame);
			renderInfo.renderer->EndCommandLabel(transitionFrame);
			device->EndFrame(transitionFrame);

			uniformWriters = GetUniformWriters(pipeline, baseMaterial, nullptr, renderInfo);
			FlushUniformWriters(uniformWriters);

			device->BeginFrame(transitionFrame);
			renderInfo.renderer->BeginCommandLabel("Layout Transition", topLevelRenderPassDebugColor, transitionFrame);
			colorTexture->Transition(Texture::Layout::SHADER_READ_ONLY_OPTIMAL, transitionFrame);
			emissiveTexture->Transition(Texture::Layout::SHADER_READ_ONLY_OPTIMAL, transitionFrame);
			renderInfo.renderer->EndCommandLabel(transitionFrame);
			device->EndFrame(transitionFrame);
			
			device->DestroyFrame(transitionFrame);
		}

		renderInfo.renderer->BeginCommandLabel(passName, topLevelRenderPassDebugColor, renderInfo.frame);

		colorTexture->Transition(Texture::Layout::GENERAL, renderInfo.frame);
		emissiveTexture->Transition(Texture::Layout::GENERAL, renderInfo.frame);

		glm::uvec2 groupCount = renderInfo.viewportSize / glm::ivec2(16, 16);
		groupCount += glm::uvec2(1, 1);
		renderInfo.renderer->Dispatch(pipeline, { groupCount.x, groupCount.y, 1 }, uniformWriters, renderInfo.frame);

		colorTexture->Transition(Texture::Layout::SHADER_READ_ONLY_OPTIMAL, renderInfo.frame);
		emissiveTexture->Transition(Texture::Layout::SHADER_READ_ONLY_OPTIMAL, renderInfo.frame);

		renderInfo.renderer->EndCommandLabel(renderInfo.frame);
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
		const std::shared_ptr<Scene> scene = renderInfo.scene;
		entt::registry& registry = scene->GetRegistry();
		const auto r3dView = registry.view<Renderer3D>();
		for (const entt::entity& entity : r3dView)
		{
			Renderer3D& r3d = registry.get<Renderer3D>(entity);
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
	color.textureCreateInfo.format = Format::R8G8B8A8_SRGB;
	color.textureCreateInfo.aspectMask = Texture::AspectMask::COLOR;
	color.textureCreateInfo.channels = 4;
	color.textureCreateInfo.isMultiBuffered = true;
	color.textureCreateInfo.usage = { Texture::Usage::SAMPLED, Texture::Usage::TRANSFER_SRC, Texture::Usage::COLOR_ATTACHMENT };
	color.textureCreateInfo.name = "FinalColor";
	color.textureCreateInfo.filepath = color.textureCreateInfo.name;
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
		PROFILER_SCOPE(Final);

		const std::shared_ptr<Mesh> plane = MeshManager::GetInstance().LoadMesh("FullScreenQuad");

		const std::string renderPassName = renderInfo.renderPass->GetName();

		const std::shared_ptr<BaseMaterial> baseMaterial = MaterialManager::GetInstance().LoadBaseMaterial("Materials/Final.basemat");
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
		baseMaterial->WriteToBuffer(postProcessBuffer, "PostProcessBuffer", "toneMapperIndex", graphicsSettings.postProcess.toneMapper);
		baseMaterial->WriteToBuffer(postProcessBuffer, "PostProcessBuffer", "gamma", graphicsSettings.postProcess.gamma);
		baseMaterial->WriteToBuffer(postProcessBuffer, "PostProcessBuffer", "viewportSize", viewportSize);
		baseMaterial->WriteToBuffer(postProcessBuffer, "PostProcessBuffer", "fxaa", fxaa);

		const int isSSREnabled = graphicsSettings.ssr.isEnabled;
		baseMaterial->WriteToBuffer(postProcessBuffer, "PostProcessBuffer", "isSSREnabled", isSSREnabled);

		postProcessBuffer->Flush();

		WriteRenderViews(renderInfo.renderView, renderInfo.scene->GetRenderView(), pipeline, renderUniformWriter);

		const std::vector<std::shared_ptr<UniformWriter>> uniformWriters = GetUniformWriters(pipeline, baseMaterial, nullptr, renderInfo);
		FlushUniformWriters(uniformWriters);

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

void RenderPassManager::FlushUniformWriters(const std::vector<std::shared_ptr<UniformWriter>>& uniformWriters)
{
	PROFILER_SCOPE(__FUNCTION__);

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
