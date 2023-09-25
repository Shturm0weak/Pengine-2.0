#include "SrBuffer.h"

using namespace Pengine;

SrBuffer::SrBuffer(size_t size)
{
	m_Size = size;
}

SrBuffer::~SrBuffer()
{
	delete[] m_Data;
	m_Size = 0;
}

void SrBuffer::WriteToBuffer(void* data, size_t size, size_t offset)
{
	m_Data = new uint8_t[size];
	memcpy(m_Data, data, size);
}
