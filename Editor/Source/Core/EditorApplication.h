#pragma once

#include "Core/Core.h"
#include "Core/Application.h"

#include "Editor.h"

class EditorApplication : public Pengine::Application
{
public:
	virtual void OnStart() override;

	virtual void OnUpdate() override;

	virtual void OnImGuiUpdate() override;

	virtual void OnClose() override;

private:
	std::unique_ptr<Editor> m_Editor;
};
