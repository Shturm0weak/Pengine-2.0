#pragma once

#include "Core/Application.h"

class FirstPersonExampleApplication : public Pengine::Application
{
public:
	virtual void OnStart() override;

	virtual void OnUpdate() override;

	virtual void OnClose() override;
};
