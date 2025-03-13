#pragma once

#include "Core/Application.h"

class ExampleApplication : public Pengine::Application
{
public:

	virtual void OnPreStart() override;

	virtual void OnStart() override;

	virtual void OnUpdate() override;

	virtual void OnClose() override;

	std::string fps;
};
