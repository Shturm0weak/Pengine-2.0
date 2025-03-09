#include "ComputePass.h"

using namespace Pengine;

ComputePass::ComputePass(const CreateInfo& createInfo)
	: Pass(createInfo.type, createInfo.name, createInfo.executeCallback, createInfo.createCallback)
{
}

void ComputePass::Execute(const RenderCallbackInfo& renderInfo) const
{
	if (m_ExecuteCallback)
	{
		m_ExecuteCallback(renderInfo);
	}
}
