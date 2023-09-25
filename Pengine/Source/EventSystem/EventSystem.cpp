#include "EventSystem.h"

#include "NextFrameEvent.h"

using namespace Pengine;

void EventSystem::DispatchEvent(Event* event)
{
	if (event->GetType() == Event::Type::OnNextFrame)
	{
		static_cast<NextFrameEvent*>(event)->Run();
		return;
	}

	auto range = m_Database.equal_range(event->GetType());
	for (auto client = range.first; client != range.second; client++)
	{
		(*client).second.callback(event);
	}
}

EventSystem::~EventSystem()
{
	m_Database.clear();
	ClearEvents();
}

EventSystem& EventSystem::GetInstance()
{
	static EventSystem eventSystem;
	return eventSystem;
}

bool EventSystem::AlreadySended(Event* event)
{
	auto currentEvent = std::find_if(m_CurrentEvents.begin(), m_CurrentEvents.end(),
		[=](Event* currentEvent)
	{
		return event->GetType() == currentEvent->GetType();
	}
	);

	return currentEvent != m_CurrentEvents.end();
}

bool EventSystem::AlreadyRegistered(Event::Type type, void* client)
{
	bool alreadyRegistered = false;

	auto range = m_Database.equal_range(type);

	for (auto clientInfo = range.first;
		clientInfo != range.second; clientInfo++)
	{
		if ((*clientInfo).second.client == client)
		{
			alreadyRegistered = true;
			break;
		}
	}

	return alreadyRegistered;
}

void EventSystem::SetProcessingEventsEnabled(bool enabled)
{
	m_IsProcessingEvents = enabled;
}

void EventSystem::RegisterClient(Event::Type type, ClientCreateInfo info)
{
	if (!info.client || AlreadyRegistered(type, info.client))
	{
		return;
	}

	m_Database.insert(std::make_pair(type, info));
}

void EventSystem::UnregisterClient(Event::Type type, void* client)
{
	auto range = m_Database.equal_range(type);

	for (auto clientInfo = range.first; clientInfo != range.second; clientInfo++)
	{
		if ((*clientInfo).second.client == client)
		{
			m_Database.erase(clientInfo);
			return;
		}
	}
}

void EventSystem::UnregisterAll(void* client)
{
	if (m_Database.size() > 0)
	{
		auto clientInfo = m_Database.begin();
		while (clientInfo != m_Database.end())
		{
			if ((*clientInfo).second.client == client)
			{
				clientInfo = m_Database.erase(clientInfo);
			}
			else
			{
				clientInfo++;
			}
		}
	}
}

void EventSystem::SendEvent(Event* event)
{
	if (m_IsProcessingEvents == false)
	{
		return;
	}

	if (event->GetSendedOnce())
	{
		if (AlreadySended(event))
		{
			delete event;
			return;
		}
	}

	m_CurrentEvents.push_back(event);
}

void EventSystem::ProcessEvents()
{
	if (m_IsProcessingEvents == false)
	{
		return;
	}

	m_IsDispatchingEvents = true;

	while (m_CurrentEvents.size() > 0)
	{
		Event* currentEvent = m_CurrentEvents.front();
		m_CurrentEvents.pop_front();
		DispatchEvent(currentEvent);
		delete currentEvent;
	}

	m_IsDispatchingEvents = false;
}

void EventSystem::ClearEvents()
{
	m_CurrentEvents.clear();
}
