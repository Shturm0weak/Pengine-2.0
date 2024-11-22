#include "ExampleApplication.h"

#include "Core/MeshManager.h"
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

	{
		std::vector<float> vertices =
		{
			-1.0f, -1.0f,
			 1.0f, -1.0f,
			 1.0f,  1.0f,
			-1.0f,  1.0f
		};

		std::vector<uint32_t> indices =
		{
			0, 1, 2, 2, 3, 0
		};

		MeshManager::GetInstance().CreateMesh("FullScreenQuad", "FullScreenQuad", sizeof(glm::vec2), vertices, indices);
	}

	{
		std::vector<uint32_t> indices =
		{
			//Top
			2, 6, 7,
			2, 3, 7,

			//Bottom
			0, 4, 5,
			0, 1, 5,

			//Left
			0, 2, 6,
			0, 4, 6,

			//Right
			1, 3, 7,
			1, 5, 7,

			//Front
			0, 2, 3,
			0, 1, 3,

			//Back
			4, 6, 7,
			4, 5, 7
		};


		std::vector<float> vertices =
		{
			-1.0f, -1.0f,  1.0f, //0
			 1.0f, -1.0f,  1.0f, //1
			-1.0f,  1.0f,  1.0f, //2
			 1.0f,  1.0f,  1.0f, //3
			-1.0f, -1.0f, -1.0f, //4
			 1.0f, -1.0f, -1.0f, //5
			-1.0f,  1.0f, -1.0f, //6
			 1.0f,  1.0f, -1.0f  //7
		};

		MeshManager::GetInstance().CreateMesh("SkyBoxCube", "SkyBoxCube", sizeof(glm::vec3), vertices, indices);
	}

	//ViewportManager::GetInstance().Create("Main", { 2560, 1440 });
	//Serializer::DeserializeScene("Scenes\\Sponza.scene");
}

void ExampleApplication::OnUpdate()
{
}

void ExampleApplication::OnClose()
{
}