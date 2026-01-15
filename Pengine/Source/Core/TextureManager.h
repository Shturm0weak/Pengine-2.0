#pragma once

#include "Core.h"

#include "../Graphics/Texture.h"

#include <mutex>
#include <stack>

namespace Pengine
{

	class PENGINE_API TextureManager
	{
	public:
		static TextureManager& GetInstance();

		TextureManager(const TextureManager&) = delete;
		TextureManager& operator=(const TextureManager&) = delete;

		std::shared_ptr<Texture> Create(const Texture::CreateInfo& createInfo);

		std::shared_ptr<Texture> Load(const std::filesystem::path& filepath, bool flip = true);

		std::vector<std::shared_ptr<Texture>> LoadFromFolder(const std::filesystem::path& directory, bool flip = true);

		std::shared_ptr<Texture> GetTexture(const std::filesystem::path& filepath) const;

		const std::unordered_map<std::filesystem::path, std::shared_ptr<Texture>, path_hash>& GetTextures() const { return m_TexturesByFilepath; }

		std::shared_ptr<Texture> GetWhite() const;

		std::shared_ptr<Texture> GetWhiteLayered() const;

		std::shared_ptr<Texture> GetBlack() const;

		std::shared_ptr<Texture> GetPink() const;

		std::shared_ptr<class UniformWriter> GetBindlessUniformWriter() const { return m_BindlessUniformWriter; }

		void CreateDefaultResources();

		void Delete(const std::filesystem::path& filepath);

		void Delete(std::shared_ptr<Texture>& texture);

		int BindTextureToBindlessUniformWriter(const std::shared_ptr<Texture>& texture);

		void UnBindTextureFromBindlessUniformWriter(const std::shared_ptr<Texture>& texture);

		std::shared_ptr<Texture> GetBindlessTexture(const int index);

		void ShutDown();

	private:
		TextureManager() = default;
		~TextureManager() = default;

		std::unordered_map<std::filesystem::path, std::shared_ptr<Texture>, path_hash> m_TexturesByFilepath;

		std::shared_ptr<Texture> m_White;
		std::shared_ptr<Texture> m_Black;
		std::shared_ptr<Texture> m_Pink;
		std::shared_ptr<Texture> m_WhiteLayered;

		std::shared_ptr<class UniformWriter> m_BindlessUniformWriter;

		mutable std::mutex m_MutexTexture;

		class SlotManager
		{
		private:
			std::stack<int> m_FreeSlots;
			std::vector<bool> m_InUse;
			
		public:
			SlotManager(int slotCount) : m_InUse(slotCount, false)
			{
				for (int i = slotCount - 1; i >= 0; i--)
				{
					m_FreeSlots.push(i);
				}
			}
			
			int TakeSlot()
			{
				if (m_FreeSlots.empty()) return 0;
				
				int slot = m_FreeSlots.top();
				m_FreeSlots.pop();
				m_InUse[slot] = true;
				return slot;
			}
			
			void FreeSlot(int index)
			{
				if (index < 0 || index >= m_InUse.size()) return;
				if (!m_InUse[index]) return;
				
				m_InUse[index] = false;
				m_FreeSlots.push(index);
			}
			
			bool IsSlotFree(int index) const
			{
				return !m_InUse[index];
			}
			
			int FreeCount() const
			{
				return m_FreeSlots.size();
			}
		};

		SlotManager m_SlotManager = SlotManager(10000);

		std::unordered_map<int, std::weak_ptr<Texture>> m_TexturesByIndex;
	};

}