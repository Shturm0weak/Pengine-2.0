#pragma once

#include "../Core/Core.h"
#include "Event.h"

namespace Pengine
{

	class PENGINE_API NextFrameEvent final : public Event
	{
	public:
		NextFrameEvent(const std::function<void()>& callback, const Type type,
			void* sender = nullptr, const bool sendedOnce = false)
			: Event(type, sender, sendedOnce)
			, m_Callback(callback)
		{
		}

		void Run() const
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