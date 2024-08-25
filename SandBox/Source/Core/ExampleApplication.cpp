#include "ExampleApplication.h"

#include "Core/Input.h"
#include "Core/MaterialManager.h"
#include "Core/ViewportManager.h"
#include "Core/Viewport.h"
#include "Components/Camera.h"
#include "Components/Transform.h"

#include "Core/Serializer.h"

using namespace Pengine;

void ExampleApplication::OnPreStart()
{
}

void ExampleApplication::OnStart()
{
	Texture::CreateInfo whiteTextureCreateInfo;
	whiteTextureCreateInfo.aspectMask = Texture::AspectMask::COLOR;
	whiteTextureCreateInfo.channels = 4;
	whiteTextureCreateInfo.filepath = "White";
	whiteTextureCreateInfo.name = "White";
	whiteTextureCreateInfo.format = Format::R8G8B8A8_SRGB;
	whiteTextureCreateInfo.size = { 1, 1 };
	whiteTextureCreateInfo.usage = { Texture::Usage::SAMPLED, Texture::Usage::TRANSFER_DST };
	const std::vector<uint8_t> pixels = {
		255,
		255,
		255,
		255
	};
	whiteTextureCreateInfo.data = pixels;
	TextureManager::GetInstance().Create(whiteTextureCreateInfo);

	//Serializer::DeserializeScene("Scenes/Bistro.scene");
}

void ExampleApplication::OnUpdate()
{
}

void ExampleApplication::OnClose()
{
}