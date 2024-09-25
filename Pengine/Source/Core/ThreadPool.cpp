#include "ThreadPool.h"

using namespace Pengine;

void ThreadPool::Initialize()
{
	m_ThreadsAmount = std::thread::hardware_concurrency() - 1;

	for (size_t i = 0; i < m_ThreadsAmount; i++)
	{
		m_Threads.emplace_back([=]
		{
			while (true)
			{
				if (m_MainId == std::this_thread::get_id())
				{
					return;
				}

				const auto isThreadBusy = m_IsThreadBusy.find(std::this_thread::get_id());

				Task task;
				{
					std::unique_lock<std::mutex> lock(m_Mutex);
					m_RunCondVar.wait(lock, [=]
					{
						return m_IsStoped || !m_Tasks.empty();
					});

					if (m_Tasks.empty() && m_IsStoped)
					{
						break;
					}

					task = std::move(m_Tasks.front());
					m_Tasks.pop();

					if (isThreadBusy != m_IsThreadBusy.end())
					{
						isThreadBusy->second = true;
					}
				}

				task();

				if (isThreadBusy != m_IsThreadBusy.end())
				{
					isThreadBusy->second = false;

					m_WaitCondVar.notify_all();
				}
			}
		});

		m_IsThreadBusy.insert(std::make_pair(m_Threads.back().get_id(), false));
	}
}

ThreadPool& ThreadPool::GetInstance()
{
	static ThreadPool threadPool;
	return threadPool;
}

void ThreadPool::Shutdown()
{
	WaitIdle();

	{
		std::unique_lock<std::mutex> lock(m_Mutex);
		m_IsStoped = true;
	}

	m_RunCondVar.notify_all();

	for (std::thread& thread : m_Threads)
	{
		thread.detach();
	}
}

void ThreadPool::WaitIdle()
{
	std::unique_lock<std::mutex> lock(m_Mutex);
	m_WaitCondVar.wait(lock, [=]
	{
		bool b = false;
		for (const auto& [id, busy] : m_IsThreadBusy)
		{
			b += busy;
		}

		return !b;
	});
}
