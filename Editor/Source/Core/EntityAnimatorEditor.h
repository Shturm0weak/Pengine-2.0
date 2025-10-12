#pragma once

#include "Core/Core.h"

#include "Components/EntityAnimator.h"

class  EntityAnimatorEditor
{
public:
	EntityAnimatorEditor();

	void Update(class Editor* editor);

	void DrawTimelineControls();

	void DrawGraphEditor();

	void DrawPropertiesPanel(class Editor* editor);

	void HandleUserInteractions();

	void DrawGrid(ImDrawList* drawList, ImVec2 canvasPosition, ImVec2 canvasSize);

private:
	std::vector<Pengine::EntityAnimator::AnimationTrack> m_Tracks;
	float m_TimelineLength = 10.0f;
	float m_Zoom = 1.0f;
	float m_ScrollX = 0.0f;
	ImVec2 m_GraphOffset = { 100.0f, 50.0f };

	Pengine::EntityAnimator::Keyframe* m_SelectedKeyframe = nullptr;
};

