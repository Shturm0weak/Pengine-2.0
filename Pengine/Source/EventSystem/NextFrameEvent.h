#pragma once

#include "../Core/Core.h"
#include "Event.h"

namespace Pengine
{

	class PENGINE_API NextFrameEvent : public Event
	{
	public:
		NextFrameEvent(const std::function<void()>& callback, Event::Type type,
			void* sender = nullptr, bool sendedOnce = false)
			: Event(type, sender, sendedOnce)
			, m_Callback(callback)
		{
		}

		void Run()
		{
			if (m_Callback)
			{
				m_Callback();
			}
		}

	private:
		std::function<void()> m_Callback;
	};

}