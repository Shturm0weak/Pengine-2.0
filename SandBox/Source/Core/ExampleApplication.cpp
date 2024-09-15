#include "ExampleApplication.h"

#include "Core/TextureManager.h"
#include "Core/Serializer.h"
#include "Core/ViewportManager.h"
#include "Core/Viewport.h"

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
	std::vector<uint8_t> pixels = {
		255,
		255,
		255,
		255
	};
	whiteTextureCreateInfo.data = pixels.data();
	TextureManager::GetInstance().Create(whiteTextureCreateInfo);

	//ViewportManager::GetInstance().Create("Main", { 2560, 1440 });
	//Serializer::DeserializeScene("Scenes\\Sponza.scene");
}

void ExampleApplication::OnUpdate()
{
}

void ExampleApplication::OnClose()
{
}