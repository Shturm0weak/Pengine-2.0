#include "Transform.h"

#include "../Utils/Utils.h"

using namespace Pengine;

void Transform::Copy(const Transform& transform)
{
	if (!m_Copyable) return;

	Translate(transform.GetPosition(System::LOCAL));
	Rotate(transform.GetRotation(System::LOCAL));
	Scale(transform.GetScale(System::LOCAL));
	m_FollowOwner = transform.m_FollowOwner;
	m_IsDirty = transform.m_IsDirty;
}

void Transform::CopyGlobal(const Transform& transform)
{
	if (!m_Copyable) return;

	Translate(transform.GetPosition());
	Rotate(transform.GetRotation());
	Scale(transform.GetScale());
	m_FollowOwner = transform.m_FollowOwner;
	m_IsDirty = transform.m_IsDirty;
}

void Transform::SetEntity(std::shared_ptr<Entity> entity)
{
	m_Entity = entity;

	Translate(GetPosition());
	Rotate(GetRotation());
	Scale(GetScale());
}

glm::mat4 Transform::GetPositionMat4(System system) const
{
	switch (system)
	{
	case System::LOCAL:
	{
		return m_LocalTransformData.m_PositionMat4;
	}
	case System::GLOBAL:
	{
		if (IsDirty() & DirtyFlagBits::TranslateMat4)
		{
			m_GlobalTransformData.m_PositionMat4 = m_LocalTransformData.m_PositionMat4;
			if (GetEntity() && GetEntity()->HasParent())
			{
				m_GlobalTransformData.m_PositionMat4 =
					GetEntity()->GetParent()->GetComponent<Transform>().GetTransform(system) * m_GlobalTransformData.m_PositionMat4;
			}

			SetIsDirty(IsDirty() & ~DirtyFlagBits::TranslateMat4);

			return m_GlobalTransformData.m_PositionMat4;
		}
		else
		{
			return m_GlobalTransformData.m_PositionMat4;
		}
	}
	default:
		return glm::mat4(1.0f);
	}
}

glm::mat4 Transform::GetRotationMat4(System system) const
{
	switch (system)
	{
	case System::LOCAL:
	{
		return m_LocalTransformData.m_RotationMat4;
	}
	case System::GLOBAL:
	{
		if (IsDirty() & DirtyFlagBits::RotationMat4)
		{
			m_GlobalTransformData.m_RotationMat4 = m_LocalTransformData.m_RotationMat4;
			if (GetEntity() && GetEntity()->HasParent())
			{
				m_GlobalTransformData.m_RotationMat4 =
					GetEntity()->GetParent()->GetComponent<Transform>().GetRotationMat4(system) * m_GlobalTransformData.m_RotationMat4;
			}

			SetIsDirty(IsDirty() & ~DirtyFlagBits::RotationMat4);

			return m_GlobalTransformData.m_RotationMat4;
		}
		else
		{
			return m_GlobalTransformData.m_RotationMat4;
		}
	}
	default:
		return glm::mat4(1.0f);
	}
}

glm::mat4 Transform::GetScaleMat4(System system) const
{
	switch (system)
	{
	case System::LOCAL:
	{
		return m_LocalTransformData.m_ScaleMat4;
	}
	case System::GLOBAL:
	{
		if (IsDirty() & DirtyFlagBits::ScaleMat4)
		{
			m_GlobalTransformData.m_ScaleMat4 = m_LocalTransformData.m_ScaleMat4;
			if (GetEntity() && GetEntity()->HasParent())
			{
				m_GlobalTransformData.m_ScaleMat4 =
					GetEntity()->GetParent()->GetComponent<Transform>().GetScaleMat4(system) * m_GlobalTransformData.m_ScaleMat4;
			}

			SetIsDirty(IsDirty() & ~DirtyFlagBits::ScaleMat4);

			return m_GlobalTransformData.m_ScaleMat4;
		}
		else
		{
			return m_GlobalTransformData.m_ScaleMat4;
		}
	}
	default:
		return glm::mat4(1.0f);
	}
}

glm::vec3 Transform::GetPosition(System system) const
{
	return Utils::GetPosition(GetPositionMat4(system));
}

glm::vec3 Transform::GetRotation(System system) const
{
	switch (system)
	{
	case System::LOCAL:
	{
		return m_LocalTransformData.m_Rotation;
	}
	case System::GLOBAL:
	{
		if (IsDirty() & DirtyFlagBits::RotationVec3)
		{
			m_GlobalTransformData.m_Rotation = m_LocalTransformData.m_Rotation;
			if (GetEntity() && GetEntity()->HasParent())
			{
				m_GlobalTransformData.m_Rotation =
					GetEntity()->GetParent()->GetComponent<Transform>().GetRotation(system) + m_GlobalTransformData.m_Rotation;
			}

			SetIsDirty(IsDirty() & ~DirtyFlagBits::RotationVec3);

			return m_GlobalTransformData.m_Rotation;
		}
		else
		{
			return m_GlobalTransformData.m_Rotation;
		}
	}
	default:
		return glm::vec3(0.0f);
	}
}

glm::vec3 Transform::GetScale(System system) const
{
	return Utils::GetScale(GetScaleMat4(system));
}

glm::mat4 Transform::GetTransform(System system) const
{
	switch (system)
	{
	case System::LOCAL:
	{
		return m_LocalTransformData.m_TransformMat4;
	}
	case System::GLOBAL:
	{
		if (IsDirty() & DirtyFlagBits::TransformMat4)
		{
			m_GlobalTransformData.m_TransformMat4 = m_LocalTransformData.m_TransformMat4;
			if (GetEntity() && GetEntity()->HasParent())
			{
				m_GlobalTransformData.m_TransformMat4 =
					GetEntity()->GetParent()->GetComponent<Transform>().GetTransform(system) * m_GlobalTransformData.m_TransformMat4;
			}

			SetIsDirty(IsDirty() & ~DirtyFlagBits::TransformMat4);

			return m_GlobalTransformData.m_TransformMat4;
		}
		else
		{
			return m_GlobalTransformData.m_TransformMat4;
		}
	}
	default:
		return glm::mat4(1.0f);
	}
}

glm::mat3 Transform::GetInverseTransform(const System system) const
{
	return GetInverseTransformMat4(system);
}

glm::mat4 Transform::GetInverseTransformMat4(const System system) const
{
	return glm::inverse(GetTransform(system));
}

void Transform::Move(Transform&& transform) noexcept
{
	m_Entity = std::move(transform.m_Entity);
	m_LocalTransformData = std::move(transform.m_LocalTransformData);
	m_GlobalTransformData = std::move(transform.m_GlobalTransformData);
	m_FollowOwner = std::move(transform.m_FollowOwner);
	m_IsDirty = std::move(transform.m_IsDirty);

	transform.m_Entity = nullptr;
}

void Transform::UpdateVectors()
{
	const float cosPitch = cos(m_LocalTransformData.m_Rotation.x);
	m_Back.z = cos(m_LocalTransformData.m_Rotation.y) * cosPitch;
	m_Back.x = sin(m_LocalTransformData.m_Rotation.y) * cosPitch;
	m_Back.y = sin(m_LocalTransformData.m_Rotation.x);
	m_Back = glm::normalize(m_Back);

	const glm::mat3 inverseTransformMat3 = GetInverseTransform(System::LOCAL);
	m_Up = glm::normalize(glm::vec3(inverseTransformMat3[0][1],
		inverseTransformMat3[1][1], inverseTransformMat3[2][1]));
}

void Transform::UpdateTransforms()
{
	m_LocalTransformData.m_TransformMat4 =
		m_LocalTransformData.m_PositionMat4 * m_LocalTransformData.m_RotationMat4 * m_LocalTransformData.m_ScaleMat4;
}

Transform& Transform::operator=(const Transform& transform)
{
	if (this != &transform)
	{
		Copy(transform);
	}

	return *this;
}

Transform& Transform::operator=(Transform&& transform) noexcept
{
	Move(std::move(transform));
	return *this;
}

Transform::Transform(const Transform& transform)
{
	Copy(transform);
}

Transform::Transform(Transform&& transform) noexcept
{
	Move(std::move(transform));
}

Transform::Transform(
	std::shared_ptr<Entity> entity,
	const glm::vec3& position,
	const glm::vec3& scale,
	const glm::vec3& rotation)
	: m_Entity(std::move(entity))
{
	Translate(position);
	Rotate(rotation);
	Scale(scale);
}

void Transform::RemoveOnRotationCallback(const std::string& label)
{
	if (const auto callback = m_OnRotationCallbacks.find(label); callback != m_OnRotationCallbacks.end())
	{
		m_OnRotationCallbacks.erase(callback);
	}
}

void Transform::RemoveOnTranslationCallback(const std::string& label)
{
	if (const auto callback = m_OnTranslationCallbacks.find(label); callback != m_OnTranslationCallbacks.end())
	{
		m_OnTranslationCallbacks.erase(callback);
	}
}

void Transform::RemoveOnScaleCallback(const std::string& label)
{
	if (const auto callback = m_OnScaleCallbacks.find(label); callback != m_OnScaleCallbacks.end())
	{
		m_OnScaleCallbacks.erase(callback);
	}
}

void Transform::Translate(const glm::vec3& position)
{
	m_LocalTransformData.m_PositionMat4 = glm::translate(glm::mat4(1.0f), position);

	UpdateTransforms();

	SetIsDirty(IsDirty() | DirtyFlagBits::TranslateMat4 | DirtyFlagBits::TransformMat4);

	std::function<void(Transform&)> translationCallbacks = [&translationCallbacks](const Transform& transform)
	{
		for (const auto& [name, callback] : transform.m_OnTranslationCallbacks)
		{
			callback();
		}

		if (!transform.m_Entity)
		{
			return;
		}

		for (const std::weak_ptr<Entity> weakChild : transform.m_Entity->GetChilds())
		{
			if (const std::shared_ptr<Entity> child = weakChild.lock())
			{
				Transform& childTransform = child->GetComponent<Transform>();
				childTransform.SetIsDirty(childTransform.IsDirty() | DirtyFlagBits::TranslateMat4 | DirtyFlagBits::TransformMat4);

				translationCallbacks(childTransform);
			}
		}
	};

	translationCallbacks(*this);
}

void Transform::Rotate(const glm::vec3& rotation)
{
	m_LocalTransformData.m_Rotation = rotation;
	m_LocalTransformData.m_RotationMat4 = glm::toMat4(glm::quat(m_LocalTransformData.m_Rotation));

	UpdateTransforms();
	UpdateVectors();

	SetIsDirty(IsDirty() | DirtyFlagBits::RotationVec3
		| DirtyFlagBits::RotationMat4 | DirtyFlagBits::TransformMat4);

	std::function<void(Transform&)> rotationCallbacks = [&rotationCallbacks](const Transform& transform)
	{
		for (const auto& [name, callback] : transform.m_OnRotationCallbacks)
		{
			callback();
		}

		if (!transform.m_Entity)
		{
			return;
		}

		for (const std::weak_ptr<Entity> weakChild : transform.m_Entity->GetChilds())
		{
			if (const std::shared_ptr<Entity> child = weakChild.lock())
			{
				Transform& childTransform = child->GetComponent<Transform>();
				childTransform.SetIsDirty(childTransform.IsDirty() | DirtyFlagBits::RotationVec3
					| DirtyFlagBits::RotationMat4 | DirtyFlagBits::TransformMat4);

				rotationCallbacks(childTransform);
			}
		}
	};

	rotationCallbacks(*this);
}

void Transform::Scale(const glm::vec3& scale)
{
	m_LocalTransformData.m_ScaleMat4 = glm::scale(glm::mat4(1.0f), scale);

	UpdateTransforms();

	SetIsDirty(IsDirty() | DirtyFlagBits::ScaleMat4 | DirtyFlagBits::TransformMat4);

	std::function<void(Transform&)> scaleCallbacks = [&scaleCallbacks](const Transform& transform)
	{
		for (auto& [name, callback] : transform.m_OnScaleCallbacks)
		{
			callback();
		}

		if (!transform.m_Entity)
		{
			return;
		}

		for (const std::weak_ptr<Entity> weakChild : transform.m_Entity->GetChilds())
		{
			if (const std::shared_ptr<Entity> child = weakChild.lock())
			{
				Transform& childTransform = child->GetComponent<Transform>();
				childTransform.SetIsDirty(childTransform.IsDirty() | DirtyFlagBits::ScaleMat4 | DirtyFlagBits::TransformMat4);

				scaleCallbacks(childTransform);
			}
		}
	};

	scaleCallbacks(*this);
}

void Transform::SetTransform(const glm::mat4& transformMat4)
{
	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale;
	Utils::DecomposeTransform(transformMat4, position, rotation, scale);

	m_LocalTransformData.m_PositionMat4 = glm::translate(glm::mat4(1.0f), position);
	m_LocalTransformData.m_ScaleMat4 = glm::scale(glm::mat4(1.0f), scale);
	m_LocalTransformData.m_Rotation = rotation;
	m_LocalTransformData.m_RotationMat4 = glm::toMat4(glm::quat(m_LocalTransformData.m_Rotation));

	UpdateTransforms();
	UpdateVectors();

	SetIsDirty(IsDirty() | DirtyFlagBits::AllTransform);

	std::function<void(Transform&)> callbacks = [&callbacks](const Transform& transform)
		{
			for (auto& [name, callback] : transform.m_OnTranslationCallbacks)
			{
				callback();
			}

			for (auto& [name, callback] : transform.m_OnRotationCallbacks)
			{
				callback();
			}

			for (auto& [name, callback] : transform.m_OnScaleCallbacks)
			{
				callback();
			}

			if (!transform.m_Entity)
			{
				return;
			}

			for (const std::weak_ptr<Entity> weakChild : transform.m_Entity->GetChilds())
			{
				if (const std::shared_ptr<Entity> child = weakChild.lock())
				{
					Transform& childTransform = child->GetComponent<Transform>();
					childTransform.SetIsDirty(childTransform.IsDirty() | DirtyFlagBits::AllTransform);

					callbacks(childTransform);
				}
			}
		};

	callbacks(*this);
}
