#include "VulkanWindow.h"

#include "../Core/Input.h"
#include "../Core/Logger.h"
#include "../Core/WindowManager.h"
#include "../Utils/Utils.h"
#include "../Vulkan/VulkanDevice.h"
#include "../Vulkan/VulkanDescriptors.h"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>

#include <locale>

using namespace Pengine;
using namespace Vk;

VulkanWindow::VulkanWindow(const std::string& name, const glm::ivec2& size)
	: Window(name, size)
{
	if (!glfwInit())
	{
		FATAL_ERROR("Failed to initialize GLFW!")
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	m_Window = glfwCreateWindow(size.x, size.y, name.c_str(), NULL, NULL);

	if (!m_Window)
	{
		glfwTerminate();

		FATAL_ERROR("Failed to create window!")
	}

	glfwMakeContextCurrent(m_Window);

	glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* rawWindow, int width, int height)
	{
		std::vector<std::shared_ptr<Window>> windows = WindowManager::GetInstance().GetWindows();
		for (std::shared_ptr<Window> window : windows)
		{
			std::shared_ptr<VulkanWindow> vulkanWindow = std::static_pointer_cast<VulkanWindow>(window);
			if (vulkanWindow && vulkanWindow->GetRawWindow() == rawWindow)
			{
				vulkanWindow->Resize({ width, height });
			}
		}
	});

	device = std::make_unique<VulkanDevice>(m_Window, name);
	descriptorPool = VulkanDescriptorPool::Builder()
		.SetPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
		.SetMaxSets(1000 * 2)
		.AddPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000)
		.AddPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000)
		.Build();

	InitializeImGui();

	Resize(size);

	auto keyCallback = [](GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		Input::KeyCallback(key, scancode, action, mods);
		ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
	};
	glfwSetKeyCallback(m_Window, keyCallback);

	auto mouseButtonCallback = [](GLFWwindow* window, int button, int action, int mods)
	{
		Input::MouseButtonCallback(button, action, mods);
		ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
	};
	glfwSetMouseButtonCallback(m_Window, mouseButtonCallback);

	auto cursorPositionCallback = [](GLFWwindow* window, double x, double y)
	{
		Input::MousePositionCallback(x, y);
		ImGui_ImplGlfw_CursorPosCallback(window, x, y);
	};
	glfwSetCursorPosCallback(m_Window, cursorPositionCallback);

	auto isKeyDownCallback = [this](int keycode)
	{
		return glfwGetKey(m_Window, keycode);
	};
	Input::SetIsKeyDownCallback(isKeyDownCallback);

	auto isMouseDownCallback = [this](int keycode)
	{
		return glfwGetMouseButton(m_Window, keycode);
	};
	Input::SetIsMouseDownCallback(isMouseDownCallback);
}

VulkanWindow::~VulkanWindow()
{
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	ImGui_ImplVulkanH_DestroyWindow(device->GetInstance(), device->GetDevice(), &m_VulkanWindow, nullptr);
	vkDestroyDescriptorPool(device->GetDevice(), g_DescriptorPool, nullptr);
	descriptorPool.reset();
	device.reset();
	glfwDestroyWindow(m_Window);
	glfwTerminate();
}

void VulkanWindow::Update()
{
}

bool VulkanWindow::Resize(const glm::ivec2& size)
{
	if (!Window::Resize(size))
	{
		return false;
	}

	SwapChainSupportDetails swapChainSupport = device->GetSwapChainSupport();
	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 &&
		imageCount > swapChainSupport.capabilities.maxImageCount)
	{
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	ImGui_ImplVulkanH_CreateOrResizeWindow(
		device->GetInstance(),
		device->GetPhysicalDevice(),
		device->GetDevice(),
		&m_VulkanWindow,
		device->GetGraphicsFamilyIndex(),
		nullptr,
		m_Size.x,
		m_Size.y,
		imageCount);
	ImGui_ImplVulkan_SetMinImageCount(imageCount);

	m_VulkanWindow.FrameIndex = 0;

	return true;
}

void VulkanWindow::NewFrame()
{
	Input::ResetInput();
	glfwPollEvents();
	SetIsRunning(!glfwWindowShouldClose(m_Window));
}

void VulkanWindow::EndFrame()
{
}

void VulkanWindow::Clear(const glm::vec4& color)
{
}

void VulkanWindow::Present(std::shared_ptr<Texture> texture)
{
}

void VulkanWindow::ShutDownPrepare()
{
	vkDeviceWaitIdle(device->GetDevice());
}

void VulkanWindow::ImGuiBegin()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	static bool dockspaceOpen = true;
	static bool optFullscreenPersistant = true;
	bool optFullscreen = optFullscreenPersistant;
	static ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_None;

	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking;

	if (optFullscreen)
	{
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->Pos);
		ImGui::SetNextWindowSize(viewport->Size);
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
	}

	if (dockspaceFlags & ImGuiDockNodeFlags_PassthruCentralNode)
	{
		windowFlags |= ImGuiWindowFlags_NoBackground;
	}

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("DockSpace", &dockspaceOpen, windowFlags);
	ImGui::PopStyleVar();

	if (optFullscreen)
	{
		ImGui::PopStyleVar(2);
	}

	ImGuiIO& io = ImGui::GetIO();
	ImGuiStyle& style = ImGui::GetStyle();
	const float minWinSizeX = style.WindowMinSize.x;
	style.WindowMinSize.x = 370.0f;
	if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
	{
		ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspaceFlags);
	}

	style.WindowMinSize.x = minWinSizeX;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0.0f, 0.0f });
}

void VulkanWindow::ImGuiEnd()
{
	ImGui::PopStyleVar();
	ImGui::End();
}

void* VulkanWindow::BeginFrame()
{
	VkSemaphore imageAcquiredSemaphore = m_VulkanWindow.FrameSemaphores[m_VulkanWindow.SemaphoreIndex].ImageAcquiredSemaphore;

	VkResult result = vkAcquireNextImageKHR(
		device->GetDevice(),
		m_VulkanWindow.Swapchain,
		(std::numeric_limits<uint64_t>::max)(),
		imageAcquiredSemaphore,
		VK_NULL_HANDLE,
		&m_VulkanWindow.FrameIndex);

	swapChainImageIndex = m_VulkanWindow.FrameIndex;

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		FATAL_ERROR("Failed to recreate swap chain image!")

			return nullptr;
	}

	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		FATAL_ERROR("Failed to acquire swap chain image!")
	}

	ImGui_ImplVulkanH_Frame* frame =
		&(m_VulkanWindow.Frames[m_VulkanWindow.FrameIndex]);

	if (vkWaitForFences(device->GetDevice(), 1, &frame->Fence, VK_TRUE, UINT64_MAX))
	{
		FATAL_ERROR("Failed to wait for fences!")
	}

	if (vkResetFences(device->GetDevice(), 1, &frame->Fence))
	{
		FATAL_ERROR("Failed to reset fences!")
	}

	if (vkResetCommandPool(device->GetDevice(), frame->CommandPool, 0))
	{
		FATAL_ERROR("Failed to reset command pool!")
	}

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	if (vkBeginCommandBuffer(frame->CommandBuffer, &beginInfo) != VK_SUCCESS)
	{
		FATAL_ERROR("Failed to begin recording command buffer!")
	}

	return frame;
}

void VulkanWindow::EndFrame(void* frame)
{
	ImGui_ImplVulkanH_Frame* imGuiFrame = static_cast<ImGui_ImplVulkanH_Frame*>(frame);

	VkCommandBuffer commandBuffer = imGuiFrame->CommandBuffer;

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
	{
		FATAL_ERROR("Failed to record command buffer!")
	}

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &m_VulkanWindow.FrameSemaphores[m_VulkanWindow.SemaphoreIndex].ImageAcquiredSemaphore;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &imGuiFrame->CommandBuffer;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &m_VulkanWindow.FrameSemaphores[m_VulkanWindow.SemaphoreIndex].RenderCompleteSemaphore;

	if (vkQueueSubmit(device->GetGraphicsQueue(), 1, &submitInfo,
		imGuiFrame->Fence) != VK_SUCCESS)
	{
		FATAL_ERROR("Failed to submit draw command buffer!")
	}

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &m_VulkanWindow.FrameSemaphores[m_VulkanWindow.SemaphoreIndex].RenderCompleteSemaphore;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_VulkanWindow.Swapchain;
	presentInfo.pImageIndices = &m_VulkanWindow.FrameIndex;

	auto result = vkQueuePresentKHR(device->GetPresentQueue(), &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		Resize(GetSize());
	}
	else if (result != VK_SUCCESS)
	{
		FATAL_ERROR("Failed to present swap chain image!")
	}

	m_VulkanWindow.SemaphoreIndex = (m_VulkanWindow.SemaphoreIndex + 1) % m_VulkanWindow.ImageCount;
}

void VulkanWindow::ImGuiRenderPass(void* frame)
{
	ImGui_ImplVulkanH_Frame* imGuiFrame = static_cast<ImGui_ImplVulkanH_Frame*>(frame);

	Vk::device->CommandBeginLabel("ImGui", imGuiFrame->CommandBuffer, { 0.5f, 1.0f, 0.5f, 1.0f });

	VkRenderPassBeginInfo info{};
	info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	info.renderPass = m_VulkanWindow.RenderPass;
	info.framebuffer = imGuiFrame->Framebuffer;
	info.renderArea.extent.width = m_VulkanWindow.Width;
	info.renderArea.extent.height = m_VulkanWindow.Height;
	info.clearValueCount = 1;
	info.pClearValues = &m_VulkanWindow.ClearValue;
	vkCmdBeginRenderPass(imGuiFrame->CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport{};
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = static_cast<float>(m_VulkanWindow.Width);
	viewport.height = static_cast<float>(m_VulkanWindow.Height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	VkRect2D scissor{ { 0, 0}, { (uint32_t)m_VulkanWindow.Width, (uint32_t)m_VulkanWindow.Height } };
	vkCmdSetViewport(imGuiFrame->CommandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(imGuiFrame->CommandBuffer, 0, 1, &scissor);

	ImGui::Render();
	ImDrawData* drawData = ImGui::GetDrawData();
	if (drawData)
	{
		ImGui_ImplVulkan_RenderDrawData(drawData, imGuiFrame->CommandBuffer);
	}

	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}

	vkCmdEndRenderPass(imGuiFrame->CommandBuffer);

	Vk::device->CommandEndLabel(imGuiFrame->CommandBuffer);
}

void VulkanWindow::DisableCursor()
{
	glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void VulkanWindow::ShowCursor()
{
	glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void VulkanWindow::HideCursor()
{
	glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
}

VkExtent2D VulkanWindow::GetExtent() const
{
	return { static_cast<uint32_t>(m_Size.x), static_cast<uint32_t>(m_Size.y) };
}

void VulkanWindow::InitializeImGui()
{
	// Create Descriptor Pool
	{
		std::vector<VkDescriptorPoolSize> poolSizes =
		{
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
		};
		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		poolInfo.maxSets = 1000 * poolSizes.size();
		poolInfo.poolSizeCount = (uint32_t)poolSizes.size();
		poolInfo.pPoolSizes = poolSizes.data();
		if (vkCreateDescriptorPool(device->GetDevice(), &poolInfo, nullptr, &g_DescriptorPool) !=
			VK_SUCCESS)
		{
			FATAL_ERROR("Failed to create descriptor pool for ImGui!");
		}
	}

	// Create Framebuffers
	m_VulkanWindow.Surface = device->GetSurface();
	const VkFormat requestSurfaceImageFormat[] = 
	{ 
		VK_FORMAT_B8G8R8A8_UNORM,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_FORMAT_B8G8R8_UNORM,
		VK_FORMAT_R8G8B8_UNORM
	};
	const VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
	m_VulkanWindow.SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(
		device->GetPhysicalDevice(),
		m_VulkanWindow.Surface, requestSurfaceImageFormat,
		(size_t)IM_ARRAYSIZE(requestSurfaceImageFormat),
		requestSurfaceColorSpace);

#define UNLIMITED_FRAME_RATE
#ifdef UNLIMITED_FRAME_RATE
	VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR };
#else
	VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_FIFO_KHR };
#endif

	m_VulkanWindow.PresentMode = ImGui_ImplVulkanH_SelectPresentMode(device->GetPhysicalDevice(), m_VulkanWindow.Surface, &present_modes[0], IM_ARRAYSIZE(present_modes));
	
	SwapChainSupportDetails swapChainSupport = device->GetSwapChainSupport();
	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 &&
		imageCount > swapChainSupport.capabilities.maxImageCount)
	{
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	swapChainImageCount = imageCount;

	ImGui_ImplVulkanH_CreateOrResizeWindow(
		device->GetInstance(),
		device->GetPhysicalDevice(),
		device->GetDevice(),
		&m_VulkanWindow,
		device->GetGraphicsFamilyIndex(),
		nullptr,
		m_Size.x,
		m_Size.y,
		imageCount);

	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->AddFontFromFileTTF("../Vendor/imgui/misc/fonts/Calibri.ttf", 16);
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForVulkan(m_Window, true);
	ImGui_ImplVulkan_InitInfo initializeInfo{};
	initializeInfo.Instance = device->GetInstance();
	initializeInfo.PhysicalDevice = device->GetPhysicalDevice();
	initializeInfo.Device = device->GetDevice();
	initializeInfo.QueueFamily = device->GetGraphicsFamilyIndex();
	initializeInfo.Queue = device->GetGraphicsQueue();
	initializeInfo.DescriptorPool = g_DescriptorPool;
	initializeInfo.MinImageCount = imageCount;
	initializeInfo.ImageCount = m_VulkanWindow.ImageCount;
	initializeInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	initializeInfo.UseDynamicRendering = false;
	ImGui_ImplVulkan_Init(&initializeInfo, m_VulkanWindow.RenderPass);
	ImGui_ImplVulkan_SetMinImageCount(imageCount);

	// Upload Fonts
	{
		// Use any command queue
		VkCommandPool commandPool = m_VulkanWindow.Frames[m_VulkanWindow.FrameIndex].CommandPool;
		VkCommandBuffer commandBuffer = m_VulkanWindow.Frames[m_VulkanWindow.FrameIndex].CommandBuffer;

		if (vkResetCommandPool(device->GetDevice(), commandPool, 0) != VK_SUCCESS)
		{
			FATAL_ERROR("Failed to reset command pool!");
		}

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
		{
			FATAL_ERROR("Failed to begin command pool!");
		}

		ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);

		VkSubmitInfo endInfo{};
		endInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		endInfo.commandBufferCount = 1;
		endInfo.pCommandBuffers = &commandBuffer;
		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
		{
			FATAL_ERROR("Failed to end command pool!");
		}
		if (vkQueueSubmit(device->GetGraphicsQueue(), 1, &endInfo, VK_NULL_HANDLE) != VK_SUCCESS)
		{
			FATAL_ERROR("Failed to submit queue!");
		}

		vkDeviceWaitIdle(device->GetDevice());
		ImGui_ImplVulkan_DestroyFontUploadObjects();
	}
}
