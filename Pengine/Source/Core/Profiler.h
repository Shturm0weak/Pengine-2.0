#pragma once

#include <chrono>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <thread>
#include <vector>
#include <atomic>
#include <sstream>
#include <cctype>

class ChromeTracingProfiler
{
public:

	static ChromeTracingProfiler& GetInstance()
	{
		static ChromeTracingProfiler chromeTracingProfiler;
		return chromeTracingProfiler;
	}

	ChromeTracingProfiler(const std::string& filepath = "Trace.json")
		: m_Filepath(filepath), m_IsStopped(true)
	{
		Start();
	}

	~ChromeTracingProfiler()
	{
		Stop();
	}

	void Start()
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		m_Events.clear();
		m_Events.reserve(5'000'000);
		m_BaseTime = std::chrono::high_resolution_clock::now();
		m_IsStopped = false;
	}

	void Stop()
	{
		if (m_IsStopped) return;
		{
			std::lock_guard<std::mutex> lock(m_Mutex);
			m_IsStopped = true;
		}
		WriteFile();
	}

	void RecordEvent(const std::string& name,
		std::chrono::high_resolution_clock::time_point start,
		std::chrono::high_resolution_clock::time_point end)
	{
		if (m_IsStopped) return;

		std::lock_guard<std::mutex> lock(m_Mutex);
		if (m_BaseTime == time_point())
		{
			m_BaseTime = start;
		}

		auto ts = std::chrono::duration_cast<std::chrono::microseconds>(
			start - m_BaseTime).count();
		auto dur = std::chrono::duration_cast<std::chrono::microseconds>(
			end - start).count();

		uint64_t tid = std::hash<std::thread::id>{}(std::this_thread::get_id());

		m_Events.push_back({
			.name = name,
			.category = "",
			.phase = 'X',
			.timestamp = ts,
			.duration = dur,
			.pid = 0,
			.tid = tid
			});
	}

	void RecordEvent(const std::string& name,
		std::chrono::high_resolution_clock::time_point start)
	{
		RecordEvent(name, start, std::chrono::high_resolution_clock::now());
	}

	class ScopedEvent {
	public:
		ScopedEvent(ChromeTracingProfiler* profiler, const std::string& name)
			: m_Profiler(profiler), m_Name(name),
			m_Start(std::chrono::high_resolution_clock::now())
		{
		}

		~ScopedEvent()
		{
			if (m_Profiler)
			{
				m_Profiler->RecordEvent(m_Name, m_Start);
			}
		}

		ScopedEvent(ScopedEvent&& other) noexcept
			: m_Profiler(other.m_Profiler), m_Name(std::move(other.m_Name)),
			m_Start(other.m_Start)
		{
			other.m_Profiler = nullptr;
		}

		ScopedEvent(const ScopedEvent&) = delete;
		ScopedEvent& operator=(const ScopedEvent&) = delete;

	private:
		ChromeTracingProfiler* m_Profiler;
		std::string m_Name;
		std::chrono::high_resolution_clock::time_point m_Start;
	};

	ScopedEvent CreateScopedEvent(const std::string& name)
	{
		return ScopedEvent(this, name);
	}

private:
	using time_point = std::chrono::high_resolution_clock::time_point;

	struct Event
	{
		std::string name;
		std::string category;
		char phase;
		int64_t timestamp;
		int64_t duration;
		int pid;
		uint64_t tid;
	};

	std::string EscapeJson(const std::string& s)
	{
		std::ostringstream o;
		for (auto c : s)
		{
			switch (c)
			{
			case '"': o << "\\\""; break;
			case '\\': o << "\\\\"; break;
			case '\b': o << "\\b"; break;
			case '\f': o << "\\f"; break;
			case '\n': o << "\\n"; break;
			case '\r': o << "\\r"; break;
			case '\t': o << "\\t"; break;
			default:
				if (static_cast<unsigned char>(c) <= 0x1f)
				{
					o << "\\u" << std::hex << std::setw(4) << std::setfill('0')
						<< static_cast<int>(c);
				}
				else
				{
					o << c;
				}
			}
		}
		return o.str();
	}

	void WriteFile()
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		std::ofstream out(m_Filepath);
		out << "{\"traceEvents\": [\n";

		bool first = true;
		for (const auto& event : m_Events)
		{
			if (!first) out << ",\n";
			first = false;

			out << "{"
				<< "\"name\": \"" << EscapeJson(event.name) << "\", "
				<< "\"cat\": \"" << event.category << "\", "
				<< "\"ph\": \"" << event.phase << "\", "
				<< "\"ts\": " << event.timestamp << ", "
				<< "\"dur\": " << event.duration << ", "
				<< "\"pid\": " << event.pid << ", "
				<< "\"tid\": " << event.tid
				<< "}";
		}

		out << "\n]}";
	}

	std::string m_Filepath;
	time_point m_BaseTime;
	std::vector<Event> m_Events;
	std::mutex m_Mutex;
	bool m_IsStopped;
};

#ifdef TRACE
	#define PROFILER_SCOPE(name) auto _profile_guard_##__LINE__ = ChromeTracingProfiler::GetInstance().CreateScopedEvent(name)
	#define PROFILER_START() ChromeTracingProfiler::GetInstance().Start()
	#define PROFILER_STOP() ChromeTracingProfiler::GetInstance().Stop()
#else
	#define PROFILER_SCOPE(name)
	#define PROFILER_START()
	#define PROFILER_STOP()
#endif
