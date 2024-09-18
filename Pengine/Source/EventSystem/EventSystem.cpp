#include "EventSystem.h"

#include "NextFrameEvent.h"

using namespace Pengine;

void EventSystem::DispatchEvent(std::shared_ptr<Event> event)
{
	if (event->GetType() == Event::Type::OnNextFrame)
	{
		std::dynamic_pointer_cast<NextFrameEvent>(event)->Run();
		return;
	}

	auto range = m_Database.equal_range(event->GetType());
	for (auto client = range.first; client != range.second; ++client)
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

bool EventSystem::AlreadySended(const std::shared_ptr<Event> event)
{
	const auto foundEvent = std::find_if(m_CurrentEvents.begin(), m_CurrentEvents.end(),
		[=](const std::shared_ptr<Event> currentEvent)
	{
		return event->GetType() == currentEvent->GetType();
	});

	return foundEvent != m_CurrentEvents.end();
}

bool EventSystem::AlreadyRegistered(const Event::Type type, const void* client)
{
	bool alreadyRegistered = false;

	auto range = m_Database.equal_range(type);

	for (auto clientInfo = range.first;
		clientInfo != range.second; ++clientInfo)
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

void EventSystem::RegisterClient(Event::Type type, const ClientCreateInfo& info)
{
	if (!info.client || AlreadyRegistered(type, info.client))
	{
		return;
	}

	m_Database.insert(std::make_pair(type, info));
}

void EventSystem::UnregisterClient(const Event::Type type, const void* client)
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

void EventSystem::UnregisterAll(const void* client)
{
	if (!m_Database.empty())
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
				++clientInfo;
			}
		}
	}
}

void EventSystem::SendEvent(std::shared_ptr<Event> event)
{
	if (m_IsProcessingEvents == false)
	{
		return;
	}

	if (event->GetSendedOnce())
	{
		if (AlreadySended(event))
		{
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

	while (!m_CurrentEvents.empty())
	{
		std::shared_ptr<Event> currentEvent = m_CurrentEvents.front();
		m_CurrentEvents.pop_front();
		DispatchEvent(currentEvent);
	}

	m_IsDispatchingEvents = false;
}

void EventSystem::ClearEvents()
{
	m_CurrentEvents.clear();
}
