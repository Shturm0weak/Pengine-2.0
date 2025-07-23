#pragma once

#include "Core/ReflectionSystem.h"

#include <yaml-cpp/yaml.h>

struct TestUserComponent
{
public:
	PROPERTY(float, m_Float, 123.123f);
	PROPERTY(double, m_Double, 123.123);
	PROPERTY(bool, m_Bool, true);
	PROPERTY(int, m_Int, 123);
	PROPERTY(std::string, m_String, "Test String");
	PROPERTY(glm::vec2, m_Vec2, glm::vec2(1, 2));

	struct CustomData
	{
		int customInt = 123;
		float customFloat = 321;
	} customData;

	static void OnSerialize(void* outPtr, void* componentPtr)
	{
		YAML::Emitter& out = *static_cast<YAML::Emitter*>(outPtr);
		TestUserComponent& component = *static_cast<TestUserComponent*>(componentPtr);

		out << YAML::Key << "customInt" << YAML::Value << component.customData.customInt;
		out << YAML::Key << "customFloat" << YAML::Value << component.customData.customFloat;
	}
	SERIALIZE_CALLBACK(TestUserComponent::OnSerialize)

	static void OnDeserialize(void* inPtr, void* componentPtr)
	{
		const YAML::Node& in = *static_cast<YAML::Node*>(inPtr);
		TestUserComponent& component = *static_cast<TestUserComponent*>(componentPtr);

		if (const auto& customIntData = in["customInt"])
		{
			component.customData.customInt = customIntData.as<int>();
		}

		if (const auto& customFloatData = in["customFloat"])
		{
			component.customData.customFloat = customFloatData.as<float>();
		}
	}
	DESERIALIZE_CALLBACK(TestUserComponent::OnDeserialize)
};
REGISTER_CLASS(TestUserComponent)
