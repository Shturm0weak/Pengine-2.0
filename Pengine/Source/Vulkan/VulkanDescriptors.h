#pragma once

#include "../Core/Core.h"

#include <vulkan/vulkan.h>

namespace Pengine::Vk
{

	class VulkanDescriptorSetLayout
	{
	public:
		class Builder
		{
		public:
			Builder& AddBinding(
				uint32_t binding,
				VkDescriptorType descriptorType,
				VkShaderStageFlags stageFlags,
				uint32_t count = 1);

			std::unique_ptr<VulkanDescriptorSetLayout> Build() const;

		private:
			std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> m_Bindings{};
		};

		explicit VulkanDescriptorSetLayout(
			const std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding>& bindings);
		~VulkanDescriptorSetLayout();
		VulkanDescriptorSetLayout(const VulkanDescriptorSetLayout&) = delete;
		VulkanDescriptorSetLayout& operator=(const VulkanDescriptorSetLayout&) = delete;

		VkDescriptorSetLayout GetDescriptorSetLayout() const { return m_DescriptorSetLayout; }

	private:
		VkDescriptorSetLayout m_DescriptorSetLayout{};
		std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> m_Bindings;

		friend class VulkanDescriptorWriter;
	};

	class VulkanDescriptorPool
	{
	public:
		class Builder
		{
		public:
			Builder& AddPoolSize(VkDescriptorType descriptorType, uint32_t count);
			Builder& SetPoolFlags(VkDescriptorPoolCreateFlags flags);
			Builder& SetMaxSets(uint32_t count);
			[[nodiscard]] std::shared_ptr<VulkanDescriptorPool> Build(VkDevice device) const;

		private:
			std::vector<VkDescriptorPoolSize> m_PoolSizes{};
			uint32_t m_MaxSets = 1000;
			VkDescriptorPoolCreateFlags m_PoolFlags = 0;
		};

		VulkanDescriptorPool(
			uint32_t maxSets,
			VkDescriptorPoolCreateFlags poolFlags,
			const std::vector<VkDescriptorPoolSize>& poolSizes,
			VkDevice device);
		~VulkanDescriptorPool();
		VulkanDescriptorPool(const VulkanDescriptorPool&) = delete;
		VulkanDescriptorPool& operator=(const VulkanDescriptorPool&) = delete;

		bool AllocateDescriptorSet(
			const VkDescriptorSetLayout& descriptorSetLayout,
			VkDescriptorSet& descriptor) const;

		bool AllocateDescriptorSets(
			const VkDescriptorSetLayout& descriptorSetLayout,
			std::vector<VkDescriptorSet>& descriptors) const;

		void FreeDescriptors(const std::vector<VkDescriptorSet>& descriptors) const;

		void ResetPool() const;

	private:
		VkDescriptorPool m_DescriptorPool{};

		friend class VulkanDescriptorWriter;
	};

	class VulkanDescriptorWriter
	{
	public:
		VulkanDescriptorWriter(VulkanDescriptorSetLayout& setLayout, VulkanDescriptorPool& pool);

		VulkanDescriptorWriter& WriteBuffer(uint32_t binding, const VkDescriptorBufferInfo* bufferInfo);
		VulkanDescriptorWriter& WriteImage(uint32_t binding, const VkDescriptorImageInfo* imageInfo);

		bool Build(VkDescriptorSet& set);
		void Overwrite(const VkDescriptorSet& set);

	private:
		VulkanDescriptorSetLayout& m_SetLayout;
		VulkanDescriptorPool& m_Pool;
		std::vector<VkWriteDescriptorSet> m_Writes;
	};

}