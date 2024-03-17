#pragma once

#include "../Core/Core.h"
#include "Event.h"

namespace Pengine
{

	class PENGINE_API ResizeEvent final : public Event
	{
	public:
		[[nodiscard]] glm::ivec2 GetSize() const { return m_Size; }

		[[nodiscard]] std::string GetViewportName() const { return m_ViewportName; }

		ResizeEvent(const glm::ivec2& size, std::string viewportName, const Type type,
			void* sender = nullptr, const bool sendedOnce = false)
			: Event(type, sender, sendedOnce)
			, m_Size(size)
			, m_ViewportName(std::move(viewportName))
		{
		}

	private:
		glm::ivec2 m_Size;
		std::string m_ViewportName;
	};

}