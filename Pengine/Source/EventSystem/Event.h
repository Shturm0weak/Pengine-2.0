#pragma once

#include "../Core/Core.h"

namespace Pengine
{

	class PENGINE_API Event
	{
	public:
		enum class Type
		{
			OnStart,
			OnUpdate,
			OnClose,
			OnResize,
			OnMainThreadProcess,
			OnNextFrame,
			OnSetScroll
		};

		Type GetType() const { return m_Type; }

		void* GetSender() const { return m_Sender; }

		bool GetSendedOnce() const { return m_SendedOnce; }

		Event(Type type, void* sender, bool sendedOnce = false)
			: m_Type(type)
			, m_Sender(sender)
			, m_SendedOnce(sendedOnce)
		{
		}

		virtual ~Event() = default;

	private:
		void* m_Sender = nullptr;
		Type m_Type = Type::OnStart;
		bool m_SendedOnce = false;
	};

}