#include "BindlessUniformWriter.h"

#include "../Graphics/Texture.h"
#include "../Graphics/UniformLayout.h"
#include "../Graphics/UniformWriter.h"

using namespace Pengine;

BindlessUniformWriter& BindlessUniformWriter::GetInstance()
{
	static BindlessUniformWriter bindlessUniformWriter;
	return bindlessUniformWriter;
}

void BindlessUniformWriter::Initialize()
{
	std::vector<ShaderReflection::ReflectDescriptorSetBinding> bindings;
	ShaderReflection::ReflectDescriptorSetBinding& binding = bindings.emplace_back();
	binding.stage = ShaderReflection::Stage::ALL;
	binding.type = ShaderReflection::Type::COMBINED_IMAGE_SAMPLER;
	binding.binding = 0;
	binding.count = 10000;
	binding.name = "BindlessTextures";
	
	const auto uniformLayout = UniformLayout::Create(bindings);
	m_BindlessUniformWriter = UniformWriter::Create(uniformLayout, false);
}

void BindlessUniformWriter::ShutDown()
{
	m_TexturesByIndex.clear();
	m_BindlessUniformWriter = nullptr;
}

int BindlessUniformWriter::BindTexture(const std::shared_ptr<Texture>& texture)
{
	if (texture->GetBindlessIndex() > 0)
	{
		return texture->GetBindlessIndex();
	}

	const int index = m_SlotManager.TakeSlot();
	texture->SetBindlessIndex(index);

	m_TexturesByIndex[index] = std::weak_ptr<Texture>(texture);

	// Note: Slot 0 is supposed to be pink texture and always taken!
	if (index > 0)
	{
		m_BindlessUniformWriter->WriteTexture(0, texture, index);
	}

	return index;
}

void BindlessUniformWriter::UnBindTexture(const std::shared_ptr<Texture>& texture)
{
	const int index = texture->GetBindlessIndex();
	if (index == 0)
	{
		return;
	}

	m_SlotManager.FreeSlot(index);
	texture->SetBindlessIndex(0);

	m_TexturesByIndex.erase(index);
}

std::shared_ptr<Texture> BindlessUniformWriter::GetBindlessTexture(const int index)
{
	auto textureByIndex = m_TexturesByIndex.find(index);
	if (textureByIndex != m_TexturesByIndex.end())
	{
		return textureByIndex->second.lock();
	}

	return 0;
}

void BindlessUniformWriter::Flush()
{
	m_BindlessUniformWriter->Flush();
}
