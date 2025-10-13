#pragma once

#include "Core/Core.h"

#include "ComponentSystems/ComponentSystem.h"

namespace Pengine
{
	class Scene;
	class Entity;
	class Material;
}

class PENGINE_API CharacterSystem : public Pengine::ComponentSystem
{
public:
	CharacterSystem();
	virtual ~CharacterSystem() override;

	virtual void OnUpdate(const float deltaTime, std::shared_ptr<class Pengine::Scene> scene) override;

	virtual void OnPrePhysicsUpdate(const float deltaTime, std::shared_ptr<class Pengine::Scene> scene) override;

	virtual void OnPostPhysicsUpdate(const float deltaTime, std::shared_ptr<class Pengine::Scene> scene) override;

	virtual std::map<std::string, std::function<void(std::shared_ptr<Pengine::Entity>)>> GetRemoveCallbacks() override { return m_RemoveCallbacks; }

	std::shared_ptr<Pengine::Entity> CreateDecal(std::shared_ptr<Pengine::Scene> scene, const glm::vec3& position, const glm::vec3& normal);

private:
	std::weak_ptr<class Pengine::Scene> m_WeakScene;

	std::vector<std::shared_ptr<Pengine::Material>> m_BloodDecalMaterails;
	std::map<std::string, std::function<void(std::shared_ptr<Pengine::Entity>)>> m_RemoveCallbacks;
};
