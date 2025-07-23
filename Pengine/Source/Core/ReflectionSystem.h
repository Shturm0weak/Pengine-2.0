#pragma once

#include "Core.h"

namespace Pengine
{

	class PENGINE_API ReflectionSystem
	{
	private:
		ReflectionSystem() = default;
		~ReflectionSystem() = default;
		ReflectionSystem(const ReflectionSystem&) = delete;
		ReflectionSystem& operator=(const ReflectionSystem&)
		{
			return *this;
		}

	public:
		class ReflectionWrapper
		{
		public:
			ReflectionWrapper(const std::function<void()>& callback)
			{
				callback();
			}
		};

		class Property
		{
		public:
			std::string m_Type;
			size_t m_Offset = 0;

			template <typename T> bool IsValue() const
			{
				return m_Type == GetTypeName<T>();
			}

			bool IsVector() const
			{
				return m_Type.find("vector") != std::string::npos;
			}

			template <typename T> T& GetValue(void* owner, const size_t baseOffset)
			{
				T& value = *reinterpret_cast<T*>(static_cast<char*>(owner) + m_Offset + baseOffset);
				return value;
			}

			template <typename T> const T& GetValue(void* owner, const size_t baseOffset) const
			{
				const T& value = *reinterpret_cast<T*>(static_cast<char*>(owner) + m_Offset + baseOffset);
				return value;
			}

			template <typename T> void SetValue(void* owner, const T& value, const size_t baseOffset)
			{
				GetValue<T>(owner, baseOffset) = value;
			}
		};

		class ParentInitializer
		{
		public:
			ParentInitializer(const entt::id_type& child, const entt::id_type& newParent, size_t offset)
			{
				if (child == newParent)
				{
					return;
				}

				if (const auto classByType = GetInstance().m_ClassesByType.find(child);
					classByType != GetInstance().m_ClassesByType.end())
				{
					if (GetInstance().m_ClassesByType.count(newParent) == 0)
					{
						return;
					}

					for (const auto& [parentName, parentOffset] : classByType->second.m_Parents)
					{
						if (parentName == newParent)
						{
							return;
						}
					}

					classByType->second.m_Parents.push_back({newParent, offset});
				}
			}
		};

		struct RegisteredClass
		{
			std::vector<std::function<void(void*, void*)>> m_CopyPropertyCallBacks;
			std::unordered_map<std::string, Property> m_PropertiesByName;
			std::vector<std::pair<entt::id_type, size_t>> m_Parents;
			std::function<void*(entt::registry&, entt::entity)> m_CreateCallback;
			std::function<void(entt::registry&, entt::entity)> m_RemoveCallback;
			std::function<void(void*, void*)> m_SerializeCallback;
			std::function<void(void*, void*)> m_DeserializeCallback;
			entt::type_info m_TypeInfo;

			RegisteredClass(const entt::type_info& typeInfo) : m_TypeInfo(typeInfo) {}
		};

		std::map<entt::id_type, RegisteredClass> m_ClassesByType;

		static ReflectionSystem& GetInstance();

		template <typename T>
		static bool SetValueImpl(const T& value, void* instance, const entt::id_type& classId,
								 const std::string& propertyName, size_t offset)
		{
			if (const auto classByType = GetInstance().m_ClassesByType.find(classId);
				classByType != GetInstance().m_ClassesByType.end())
			{
				for (auto& [name, property] : classByType->second.m_PropertiesByName)
				{
					if (propertyName != name)
					{
						continue;
					}

					if (property.IsValue<T>())
					{
						property.SetValue(instance, value, offset);

						return true;
					}
				}

				for (const auto& [parentName, parentOffset] : classByType->second.m_Parents)
				{
					if (SetValueImpl(value, instance, parentName, propertyName, offset + parentOffset))
					{
						return true;
					}
				}

				return false;
			}

			return false;
		}

		template <typename T>
		static void SetValue(const T& value, void* instance, const entt::id_type& classId,
							 const std::string& propertyName)
		{
			SetValueImpl(value, instance, classId, propertyName, 0);
		}

		template <typename T>
		static T GetValueImpl(void* instance, const entt::id_type& classId, const std::string& propertyName,
							  size_t offset)
		{
			if (const auto classByType = GetInstance().m_ClassesByType.find(classId);
				classByType != GetInstance().m_ClassesByType.end())
			{
				for (auto& [name, property] : classByType->second.m_PropertiesByName)
				{
					if (propertyName != name)
					{
						continue;
					}

					if (property.IsValue<T>())
					{
						return property.GetValue<T>(instance, offset);
					}
				}

				for (const auto& [parentName, parentOffset] : classByType->second.m_Parents)
				{
					return GetValueImpl<T>(instance, parentName, propertyName, offset + parentOffset);
				}

				return T{};
			}

			return T{};
		}

		template <typename T>
		static T GetValue(void* instance, const entt::id_type& classId, const std::string& propertyName)
		{
			return GetValueImpl<T>(instance, classId, propertyName, 0);
		}
	};

#define COM ,

#define RTTR_CAT_IMPL(a, b) a##b
#define RTTR_CAT(a, b) RTTR_CAT_IMPL(a, b)

#define RTTR_REGISTRATION_USER_DEFINED(type)                                                                           \
	static void rttr_auto_register_reflection_function_##type();                                                       \
	namespace                                                                                                          \
	{                                                                                                                  \
		struct rttr__auto__register__##type                                                                            \
		{                                                                                                              \
			rttr__auto__register__##type()                                                                             \
			{                                                                                                          \
				rttr_auto_register_reflection_function_##type();                                                       \
			}                                                                                                          \
		};                                                                                                             \
	}                                                                                                                  \
	static const rttr__auto__register__##type RTTR_CAT(RTTR_CAT(auto_register__, __LINE__), type);                     \
	static void rttr_auto_register_reflection_function_##type()

#define REGISTER_CLASS(_type)                                                                                          \
	RTTR_REGISTRATION_USER_DEFINED(_type)                                                                              \
	{                                                                                                                  \
		Pengine::ReflectionSystem::RegisteredClass registeredClass(entt::type_id<_type>());                            \
		registeredClass.m_CreateCallback = [](entt::registry& registry, entt::entity entity)                           \
		{                                                                                                              \
			return (void*)&registry.emplace<_type>(entity);                                                            \
		};                                                                                                             \
		registeredClass.m_RemoveCallback = [](entt::registry& registry, entt::entity entity)                           \
		{                                                                                                              \
			registry.remove<_type>(entity);                                                                            \
		};                                                                                                             \
		Pengine::ReflectionSystem::GetInstance().m_ClassesByType.emplace(                                              \
			std::make_pair(GetTypeHash<_type>(), registeredClass));                                                    \
	}

#define SERIALIZE_CALLBACK(_callback)                                                                                  \
private:                                                                                                               \
	Pengine::ReflectionSystem::ReflectionWrapper ___ReflectionWrapper_SerializeCallback = Pengine::ReflectionSystem::ReflectionWrapper(\
		[baseClass = this]                                                                                             \
		{                                                                                                              \
			auto classByType =                                                                                         \
				Pengine::ReflectionSystem::GetInstance().m_ClassesByType.find(GetTypeHash<decltype(*baseClass)>());    \
			if (classByType != Pengine::ReflectionSystem::GetInstance().m_ClassesByType.end())                         \
			{                                                                                                          \
				if (classByType->second.m_SerializeCallback)                                                           \
					return;                                                                                            \
				classByType->second.m_SerializeCallback = &_callback;                                                  \
			}                                                                                                          \
		});

#define DESERIALIZE_CALLBACK(_callback)                                                                                \
private:                                                                                                               \
	Pengine::ReflectionSystem::ReflectionWrapper ___ReflectionWrapper_DeserializeCallback = Pengine::ReflectionSystem::ReflectionWrapper(\
		[baseClass = this]                                                                                             \
		{                                                                                                              \
			auto classByType =                                                                                         \
				Pengine::ReflectionSystem::GetInstance().m_ClassesByType.find(GetTypeHash<decltype(*baseClass)>());    \
			if (classByType != Pengine::ReflectionSystem::GetInstance().m_ClassesByType.end())                         \
			{                                                                                                          \
				if (classByType->second.m_DeserializeCallback)                                                         \
					return;                                                                                            \
				classByType->second.m_DeserializeCallback = &_callback;                                                  \
			}                                                                                                          \
		});

#define PROPERTY(_type, _name, _value)                                                                                 \
	_type _name = _value;                                                                                              \
																													   \
private:                                                                                                               \
	Pengine::ReflectionSystem::ReflectionWrapper ___ReflectionWrapper_##_name = Pengine::ReflectionSystem::ReflectionWrapper(\
		[baseClass = this]                                                                                             \
		{                                                                                                              \
			auto classByType =                                                                                         \
				Pengine::ReflectionSystem::GetInstance().m_ClassesByType.find(GetTypeHash<decltype(*baseClass)>());    \
			if (classByType != Pengine::ReflectionSystem::GetInstance().m_ClassesByType.end())                         \
			{                                                                                                          \
				if (classByType->second.m_PropertiesByName.count(#_name) > 0)                                          \
					return;                                                                                            \
				classByType->second.m_PropertiesByName.emplace(                                                        \
					std::string(#_name),                                                                               \
					Pengine::ReflectionSystem::Property{                                                               \
						GetTypeName<_type>(),                                                                          \
						((::size_t) & reinterpret_cast<char const volatile&>((((decltype(baseClass))0)->_name)))});    \
				classByType->second.m_CopyPropertyCallBacks.push_back(                                                 \
					[=](void* a, void* b)                                                                              \
					{ static_cast<decltype(baseClass)>(a)->_name = static_cast<decltype(baseClass)>(b)->_name; });     \
			}                                                                                                          \
		});

#define COPY_PROPERTIES(_component)                                                                                    \
	std::function<void(const entt::id_type&, size_t)> copyProperties =                                                 \
		[this, &_component, &copyProperties](const entt::id_type& typeHash, size_t offset)                             \
	{                                                                                                                  \
		auto classByType = Pengine::ReflectionSystem::GetInstance().m_ClassesByType.find(typeHash);                    \
		if (classByType != Pengine::ReflectionSystem::GetInstance().m_ClassesByType.end())                             \
		{                                                                                                              \
			for (size_t i = 0; i < classByType->second.m_CopyPropertyCallBacks.size(); i++)                            \
			{                                                                                                          \
				classByType->second.m_CopyPropertyCallBacks[i]((char*)this + offset, (char*)&_component + offset);     \
			}                                                                                                          \
		}                                                                                                              \
		for (const auto& parent : classByType->second.m_Parents)                                                       \
		{                                                                                                              \
			copyProperties(parent.first, parent.second);                                                               \
		}                                                                                                              \
	};                                                                                                                 \
	copyProperties(GetTypeHash<decltype(_component)>(), 0);

#define REGISTER_PARENT_CLASS(_type)                                                                                   \
private:                                                                                                               \
	Pengine::ReflectionSystem::ParentInitializer ___ParentInitializer_##_type = Pengine::ReflectionSystem::ParentInitializer(\
		GetTypeHash<decltype(*this)>(), GetTypeHash<_type>(), (size_t)(_type*)(this) - (size_t)this);

}