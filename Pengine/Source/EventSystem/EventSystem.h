#pragma once

#include "../Core/Core.h"
#include "Event.h"

#include <deque>
#include <map>
#include <unordered_map>
#include <vector>

namespace Pengine
{
	class PENGINE_API EventSystem
	{
	public:
		struct ClientCreateInfo
		{
			void* client;
			std::function<void(Event* event)> callback;
		};

		static EventSystem& GetInstance();
		
		bool AlreadySended(Event* event);
		
		bool AlreadyRegistered(Event::Type type, void* client);
		
		void SetProcessingEventsEnabled(bool enabled);
		
		void RegisterClient(Event::Type type, ClientCreateInfo info);

		void UnregisterClient(Event::Type type, void* client);
		
		void UnregisterAll(void* client);
		
		void SendEvent(Event* event);

		void ProcessEvents();
		
		void ClearEvents();

	private:
		EventSystem() = default;
		EventSystem(const EventSystem&) = delete;
		EventSystem& operator=(const EventSystem&) { return *this; }
		~EventSystem();

		void DispatchEvent(Event* event);

		std::multimap<Event::Type, ClientCreateInfo> m_Database;
		std::deque<Event*> m_CurrentEvents;

		bool m_IsProcessingEvents = true;
		bool m_IsDispatchingEvents = false;
	};

}
