#pragma once

#include "Core.h"

#include <condition_variable>
#include <queue>
#include <mutex>
#include <map>
#include <vector>
#include <thread>


namespace Pengine
{

	class PENGINE_API ThreadPool
	{
		using Task = std::function<void()>;
	public:

		static ThreadPool& GetInstance();

		ThreadPool(const ThreadPool&) = delete;
		ThreadPool& operator=(const ThreadPool&) = delete;

		void Initialize();

		inline uint32_t GetThreadsAmount() { return m_Threads.size(); }

		void Shutdown();

		void Enqueue(Task task);

		bool IsMainThread() const { return m_MainId == std::this_thread::get_id(); }

	private:
		ThreadPool() = default;
		~ThreadPool() = default;

		std::vector<std::thread> m_Threads;
		std::map<std::thread::id, bool> m_IsThreadBusy;
		std::mutex m_Mutex;
		std::thread::id m_MainId = std::this_thread::get_id();
		std::condition_variable m_CondVar;
		std::queue<Task> m_Tasks;
		int m_ThreadsAmount = 0;
		bool m_IsStoped = false;
	};

}