#pragma once

#include "../Core/Core.h"
#include "Event.h"

namespace Pengine
{

	class PENGINE_API ResizeEvent : public Event
	{
	public:
		glm::ivec2 GetSize() const { return m_Size; }

		std::string GetViewportName() const { return m_ViewportName; }

		ResizeEvent(const glm::ivec2& size, const std::string& viewportName, Event::Type type,
			void* sender = nullptr, bool sendedOnce = false)
			: Event(type, sender, sendedOnce)
			, m_Size(size)
			, m_ViewportName(viewportName)
		{
		}

	private:
		glm::ivec2 m_Size;
		std::string m_ViewportName;
	};

}