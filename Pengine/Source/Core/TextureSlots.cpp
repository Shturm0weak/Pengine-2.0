#include "TextureSlots.h"

#include "TextureManager.h"

using namespace Pengine;

TextureSlots::TextureSlots(size_t size)
{
	textures.resize(size);
	ResetTextures();
}

int TextureSlots::BindTexture(std::shared_ptr<Texture> texture, std::optional<int> slot)
{
	for (size_t i = 0; i < textures.size(); i++)
	{
		if (textures[i] == texture)
		{
			return i;
		}
	}

	if (m_CurrentSlot >= textures.size())
	{
		return m_CurrentSlot;
	}

	if (slot)
	{
		textures[*slot] = texture;
		return *slot;
	}
	else
	{
		textures[m_CurrentSlot] = texture;
		m_CurrentSlot++;
		return m_CurrentSlot - 1;
	}
}

void TextureSlots::ResetTextures()
{
	for (size_t i = 0; i < textures.size(); i++)
	{
		textures[i] = nullptr;
	}

	m_CurrentSlot = 0;
}

void TextureSlots::Finish()
{
	std::shared_ptr<Texture> white = TextureManager::GetInstance().GetWhite();
	for (size_t i = m_CurrentSlot; i < textures.size(); i++)
	{
		textures[i] = white;
	}
}
