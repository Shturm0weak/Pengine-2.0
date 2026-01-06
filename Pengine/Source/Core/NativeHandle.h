#pragma once

namespace Pengine
{

	class NativeHandle
	{
	public:
		static NativeHandle Invalid() { return NativeHandle(); }

		NativeHandle() = default;
		explicit NativeHandle(size_t handle) : m_Handle(handle) {}

		bool operator==(const NativeHandle& other) const noexcept { return m_Handle == other.m_Handle; }
		bool operator!=(const NativeHandle& other) const noexcept { return !(*this == other); }

		explicit operator bool() const noexcept { return m_Handle != 0; }

		size_t Get() const noexcept { return m_Handle; }

		bool operator<(const NativeHandle& other) const noexcept { return m_Handle < other.m_Handle; }
		bool operator<=(const NativeHandle& other) const noexcept { return m_Handle <= other.m_Handle; }
		bool operator>(const NativeHandle& other) const noexcept { return m_Handle > other.m_Handle; }
		bool operator>=(const NativeHandle& other) const noexcept { return m_Handle >= other.m_Handle; }

	private:
		size_t m_Handle = 0;
	};

}
