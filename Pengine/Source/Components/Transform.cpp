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
}

void Transform::CopyGlobal(const Transform& transform)
{
	if (!m_Copyable) return;

	Translate(transform.GetPosition());
	Rotate(transform.GetRotation());
	Scale(transform.GetScale());
	m_FollowOwner = transform.m_FollowOwner;
}

void Transform::SetEntity(std::shared_ptr<Entity> entity)
{
	m_Entity = entity;

	Translate(GetPosition());
	Rotate(GetRotation());
	Scale(GetScale());
}

glm::mat4 Transform::GetPositionMat4(const System system) const
{
	switch (system)
	{
	case System::LOCAL:
	{
		return m_PositionMat4;
	}
	case System::GLOBAL:
	{
		glm::mat4 positionMat4 = m_PositionMat4;
		if (m_Entity && m_Entity->HasParent())
		{
			const Transform& parent = m_Entity->GetParent()->GetComponent<Transform>();
			positionMat4 *= parent.GetPositionMat4();
		}
		return positionMat4;
	}
	default:
		return glm::mat4(1.0f);
	}
}

glm::mat4 Transform::GetRotationMat4(const System system) const
{
	switch (system)
	{
	case System::LOCAL:
	{
		return m_RotationMat4;
	}
	case System::GLOBAL:
	{
		glm::mat4 rotationMat4 = m_RotationMat4;
		if (m_Entity && m_Entity->HasParent())
		{
			const Transform& parent = m_Entity->GetParent()->GetComponent<Transform>();
			rotationMat4 *= parent.GetRotationMat4();
		}
		return rotationMat4;
	}
	default:
		return glm::mat4(1.0f);
	}
}

glm::mat4 Transform::GetScaleMat4(const System system) const
{
	switch (system)
	{
	case System::LOCAL:
	{
		return m_ScaleMat4;
	}
	case System::GLOBAL:
	{
		glm::mat4 scaleMat4 = m_ScaleMat4;
		if (m_Entity && m_Entity->HasParent())
		{
			const Transform& parent = m_Entity->GetParent()->GetComponent<Transform>();
			scaleMat4 *= parent.GetScaleMat4();
		}
		return scaleMat4;
	}
	default:
		return glm::mat4(1.0f);
	}
}

glm::vec3 Transform::GetPreviousPosition(const System system) const
{
	switch (system)
	{
	case System::LOCAL:
	{
		return m_PreviousPosition;
	}
	case System::GLOBAL:
	{
		glm::vec3 previousPosition = m_PreviousPosition;
		if (m_Entity && m_Entity->HasParent())
		{
			const Transform& parent = m_Entity->GetParent()->GetComponent<Transform>();
			previousPosition = parent.GetTransform() * glm::vec4(previousPosition, 1.0f);
		}
		return previousPosition;
	}
	default:
		return {};
	}
}

glm::vec3 Transform::GetPositionDelta(const System system) const
{
	switch (system)
	{
	case System::LOCAL:
	{
		return m_PositionDelta;
	}
	case System::GLOBAL:
	{
		glm::vec3 positionDelta = m_PositionDelta;
		if (m_Entity && m_Entity->HasParent())
		{
			const Transform& parent = m_Entity->GetParent()->GetComponent<Transform>();
			positionDelta = parent.GetTransform() * glm::vec4(positionDelta, 1.0f);
		}
		return positionDelta;
	}
	default:
		return {};
	}
}

glm::vec3 Transform::GetPosition(const System system) const
{
	switch (system)
	{
	case System::LOCAL:
	{
		return Utils::GetPosition(m_PositionMat4);
	}
	case System::GLOBAL:
	{
		return Utils::GetPosition(GetTransform(system));
	}
	default:
		return {};
	}
}

glm::vec3 Transform::GetRotation(const System system) const
{
	switch (system)
	{
	case System::LOCAL:
	{
		return m_Rotation;
	}
	case System::GLOBAL:
	{
		glm::vec3 rotation = m_Rotation;
		if (m_Entity && m_Entity->HasParent())
		{
			const Transform& parent = m_Entity->GetParent()->GetComponent<Transform>();
			rotation += parent.GetRotation();
		}
		return rotation;
	}
	default:
		return {};
	}
}

glm::vec3 Transform::GetScale(const System system) const
{
	switch (system)
	{
	case System::LOCAL:
	{
		return Utils::GetScale(m_ScaleMat4);
	}
	case System::GLOBAL:
	{
		glm::vec3 scale = Utils::GetScale(m_ScaleMat4);
		if (m_Entity && m_Entity->HasParent())
		{
			const Transform& parent = m_Entity->GetParent()->GetComponent<Transform>();
			scale *= parent.GetScale();
		}
		return scale;
	}
	default:
		return {};
	}
}

glm::mat4 Transform::GetTransform(const System system) const
{
	switch (system)
	{
	case System::LOCAL:
	{
		return m_TransformMat4;
	}
	case System::GLOBAL:
	{
		glm::mat4 transformMat4 = m_TransformMat4;
		if (m_Entity && m_Entity->HasParent())
		{
			const Transform& parent = m_Entity->GetParent()->GetComponent<Transform>();
			transformMat4 = parent.GetTransform() * transformMat4;
		}
		return transformMat4;
	}
	default:
		return glm::mat4(1.0f);
	}
}

glm::mat3 Transform::GetInverseTransform(const System system) const
{
	return glm::inverse(GetTransform(system));
}

void Transform::Move(Transform&& transform) noexcept
{
	m_Rotation = transform.m_Rotation;
	m_PositionMat4 = transform.m_PositionMat4;
	m_ScaleMat4 = transform.m_ScaleMat4;
	m_RotationMat4 = transform.m_RotationMat4;
	m_FollowOwner = transform.m_FollowOwner;
}

void Transform::UpdateVectors()
{
	const float cosPitch = cos(m_Rotation.x);
	m_Back.z = cos(m_Rotation.y) * cosPitch;
	m_Back.x = sin(m_Rotation.y) * cosPitch;
	m_Back.y = sin(m_Rotation.x);
	m_Back = glm::normalize(m_Back);

	const glm::mat3 inverseTransformMat3 = GetInverseTransform(System::LOCAL);
	m_Up = glm::normalize(glm::vec3(inverseTransformMat3[0][1],
		inverseTransformMat3[1][1], inverseTransformMat3[2][1]));
}

void Transform::UpdateTransforms()
{
	m_TransformMat4 = m_PositionMat4 * m_RotationMat4 * m_ScaleMat4;
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
	m_IsDirty = true;

	m_PreviousPosition = GetPosition(System::LOCAL);
	m_PositionDelta = position - m_PreviousPosition;
	m_PositionMat4 = glm::translate(glm::mat4(1.0f), position);

	UpdateTransforms();

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

		for (const std::shared_ptr<Entity>& child : transform.m_Entity->GetChilds())
		{
			translationCallbacks(child->GetComponent<Transform>());
		}
	};

	translationCallbacks(*this);
}

void Transform::Rotate(const glm::vec3& rotation)
{
	m_IsDirty = true;

	m_PreviousRotation = GetRotation(System::LOCAL);
	m_Rotation = rotation;
	m_RotationDelta = m_Rotation - m_PreviousRotation;
	m_RotationMat4 = glm::toMat4(glm::quat(m_Rotation));

	UpdateTransforms();
	UpdateVectors();

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

		for (const std::shared_ptr<Entity>& child : transform.m_Entity->GetChilds())
		{
			rotationCallbacks(child->GetComponent<Transform>());
		}
	};

	rotationCallbacks(*this);
}

void Transform::Scale(const glm::vec3& scale)
{
	m_IsDirty = true;

	m_PreviousScale = GetScale(System::LOCAL);
	m_ScaleDelta = scale - m_PreviousScale;
	m_ScaleMat4 = glm::scale(glm::mat4(1.0f), scale);

	UpdateTransforms();

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

		for (const std::shared_ptr<Entity>& child : transform.m_Entity->GetChilds())
		{
			scaleCallbacks(child->GetComponent<Transform>());
		}
	};

	scaleCallbacks(*this);
}
