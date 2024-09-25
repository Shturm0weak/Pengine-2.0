#pragma once

#include "Core.h"
#include "Logger.h"

#include <condition_variable>
#include <future>
#include <queue>
#include <mutex>
#include <map>
#include <vector>
#include <thread>


namespace Pengine
{

	class PENGINE_API ThreadPool
	{
		using Task = std::move_only_function<void()>;
	public:

		static ThreadPool& GetInstance();

		ThreadPool(const ThreadPool&) = delete;
		ThreadPool& operator=(const ThreadPool&) = delete;

		void Initialize();

		inline size_t GetThreadsAmount() { return m_Threads.size(); }

		void Shutdown();

		template<typename F, typename ...Args>
		auto EnqueueSync(F&& function, Args&& ...args)
		{
			return std::async(std::launch::async, std::forward<F>(function), std::forward<Args>(args)...);
		}

		template<typename F, typename ...Args>
		void EnqueueAsync(F&& function, Args&& ...args)
		{
			auto task = [
				function = std::move(function),
				...args = std::move(args)]()
			{
				try
				{
					function(args...);
				}
				catch (...)
				{
					std::rethrow_exception(std::current_exception());
				}
			};

			{
				std::unique_lock<std::mutex> lock(m_Mutex);
				m_Tasks.emplace(std::move(task));
			}

			m_RunCondVar.notify_one();
		}

		bool IsMainThread() const { return m_MainId == std::this_thread::get_id(); }

		void WaitIdle();

	private:
		ThreadPool() = default;
		~ThreadPool() = default;

		std::vector<std::thread> m_Threads;
		std::map<std::thread::id, bool> m_IsThreadBusy;
		std::mutex m_Mutex;
		std::thread::id m_MainId = std::this_thread::get_id();
		std::condition_variable m_RunCondVar;
		std::condition_variable m_WaitCondVar;
		std::queue<Task> m_Tasks;
		size_t m_ThreadsAmount = 0;
		bool m_IsStoped = false;
	};

}
