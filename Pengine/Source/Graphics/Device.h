#pragma once

#include "../Core/Core.h"

namespace Pengine
{

	class PENGINE_API Device
	{
	public:
		static std::shared_ptr<Device> Create(const std::string& applicationName);

		Device(const Device&) = delete;
		Device& operator=(const Device&) = delete;

		virtual const std::string& GetName() const = 0;

		virtual void ShutDown() = 0;

		virtual void WaitIdle() const = 0;

		virtual void FlushDeletionQueue(bool immediate = false) = 0;

		virtual void* Begin() = 0;

		virtual void End(void* frame) = 0;

	protected:
		Device() = default;
		virtual ~Device() = default;
	};

}
