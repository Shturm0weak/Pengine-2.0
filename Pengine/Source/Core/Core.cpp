#include "Core.h"

using namespace Pengine;

GlobalDataAccessor& GlobalDataAccessor::GetInstance()
{
	static GlobalDataAccessor globalDataAccessor;
	return globalDataAccessor;
}

std::unordered_map<UUID, std::filesystem::path, uuid_hash>& GlobalDataAccessor::GetFilepathByUuid() { return filepathByUuid; }
std::unordered_map<std::filesystem::path, UUID, path_hash>& GlobalDataAccessor::GetUuidByFilepath() { return uuidByFilepath; }

int GlobalDataAccessor::GetDrawCallsCount() const { return drawCallsCount; }
size_t GlobalDataAccessor::GetVertexCount() const { return vertexCount; }
size_t GlobalDataAccessor::GetCurrentFrame() const { return currentFrame; }
int64_t GlobalDataAccessor::GetVramAllocated() const { return vramAllocated; }

uint32_t& GlobalDataAccessor::GetSwapChainImageCount() { return Vk::swapChainImageCount; }
uint32_t& GlobalDataAccessor::GetSwapChainImageIndex() { return Vk::swapChainImageIndex; }

std::mutex& Pengine::GlobalDataAccessor::GetUUIDMutex() { return uuidMutex; }

std::shared_ptr<class Device> GlobalDataAccessor::GetDevice() const { return device; }