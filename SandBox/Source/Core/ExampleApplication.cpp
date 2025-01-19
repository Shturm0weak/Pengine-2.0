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
	//ViewportManager::GetInstance().Create("Main", { 800, 600 });
	//Serializer::DeserializeScene("Scenes/Sponza/Sponza.scene");
}

void ExampleApplication::OnUpdate()
{
}

void ExampleApplication::OnClose()
{
}
