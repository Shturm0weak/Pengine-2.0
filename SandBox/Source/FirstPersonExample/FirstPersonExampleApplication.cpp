#include "FirstPersonExampleApplication.h"

#include "FirstPersonCharacterSystem.h"

#include "Core/Serializer.h"
#include "Core/SceneManager.h"


void FirstPersonExampleApplication::OnStart()
{
	Pengine::SceneManager::GetInstance().SetComponentSystem<FirstPersonCharacterSystem>("FirstPersonCharacterSystem");
	Pengine::Serializer::DeserializeScene("Scenes/Examples/FirstPerson/FirstPerson.scene");
}

void FirstPersonExampleApplication::OnUpdate()
{
}

void FirstPersonExampleApplication::OnClose()
{
}
