#include "Application.h"

#include "CharacterSystem.h"

#include "Core/Serializer.h"
#include "Core/SceneManager.h"

void FirstPersonExampleApplication::OnStart()
{
	Pengine::SceneManager::GetInstance().SetComponentSystem<CharacterSystem>("FirstPersonCharacterSystem");
	Pengine::Serializer::DeserializeScene("Examples/FirstPerson/FirstPerson.scene");
}

void FirstPersonExampleApplication::OnUpdate()
{
}

void FirstPersonExampleApplication::OnClose()
{
}
