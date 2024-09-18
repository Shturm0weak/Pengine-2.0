#pragma once

#include "../Core/Core.h"
#include "Event.h"

#include <deque>

namespace Pengine
{
	class PENGINE_API EventSystem
	{
	public:
		struct ClientCreateInfo
		{
			void* client;
			std::function<void(std::shared_ptr<Event> event)> callback;
		};

		static EventSystem& GetInstance();

		EventSystem(const EventSystem&) = delete;

		bool AlreadySended(const std::shared_ptr<Event> event);
		
		bool AlreadyRegistered(Event::Type type, const void* client);
		
		void SetProcessingEventsEnabled(bool enabled);
		
		void RegisterClient(Event::Type type, const ClientCreateInfo& info);

		void UnregisterClient(Event::Type type, const void* client);
		
		void UnregisterAll(const void* client);
		
		void SendEvent(std::shared_ptr<Event> event);

		void ProcessEvents();
		
		void ClearEvents();

	private:
		EventSystem() = default;
		EventSystem& operator=(const EventSystem&) { return *this; }
		~EventSystem();

		void DispatchEvent(std::shared_ptr<Event> event);

		std::multimap<Event::Type, ClientCreateInfo> m_Database;
		std::deque<std::shared_ptr<Event>> m_CurrentEvents;

		bool m_IsProcessingEvents = true;
		bool m_IsDispatchingEvents = false;
	};

}
