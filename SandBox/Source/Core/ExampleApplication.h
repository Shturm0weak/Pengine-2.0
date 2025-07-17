#pragma once

#include "Core/Application.h"
#include "Core/Scene.h"

class ExampleApplication : public Pengine::Application
{
public:

	virtual void OnPreStart() override;

	virtual void OnStart() override;

	virtual void OnUpdate() override;

	virtual void OnClose() override;

	std::shared_ptr<Pengine::Scene> scene;

};
