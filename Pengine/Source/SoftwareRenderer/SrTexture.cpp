#include "SrTexture.h"

using namespace Pengine;

SrTexture::SrTexture(TextureCreateInfo textureCreateInfo)
	: Texture(textureCreateInfo)
{
}

glm::ivec4 SrTexture::Sample(glm::vec2 uv) const
{
    uv = glm::clamp(uv, glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 1.0f));
    glm::ivec2 pos = uv * glm::vec2(m_Size - 1);
    int position = (pos.x + pos.y * m_Size.x) * m_Channels;
    uint8_t* texel = &((uint8_t*)m_Data)[position];
    glm::ivec4 color = glm::vec4(0);
    for (size_t i = 0; i < m_Channels; i++)
    {
        color[i] = texel[i];
    }

    return color;
}
