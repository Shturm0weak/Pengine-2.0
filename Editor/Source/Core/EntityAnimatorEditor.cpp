#include "EntityAnimatorEditor.h"

#include "Editor.h"

#include "Core/FileFormatNames.h"
#include "Core/Serializer.h"

using namespace Pengine;

EntityAnimatorEditor::EntityAnimatorEditor()
{
}

void EntityAnimatorEditor::Update(Editor* editor)
{
	if (editor->m_EntityAnimatorEditorOpened)
	{
		DrawTimelineControls();
		DrawGraphEditor();
		DrawPropertiesPanel(editor);
	}
}

void EntityAnimatorEditor::DrawTimelineControls()
{
	ImGui::Begin("Timeline Controls");

	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSETS_BROWSER_ITEM"))
		{
			std::wstring filepath((const wchar_t*)payload->Data);
			filepath.resize(payload->DataSize / sizeof(wchar_t));

			if (FileFormats::Track() == Utils::GetFileFormat(filepath))
			{
				const auto animationTrack = Serializer::DeserializeAnimationTrack(filepath);
				if (animationTrack)
				{
					if (m_Tracks.empty())
					{
						m_Tracks.emplace_back(*animationTrack);
					}
					else
					{
						m_Tracks[0] = *animationTrack;
					}
				}
			}
		}

		ImGui::EndDragDropTarget();
	}

	ImGui::SliderFloat("Zoom", &m_Zoom, 0.1f, 5.0f);
	ImGui::InputFloat("Length", &m_TimelineLength);

	if (ImGui::Button("Save"))
	{
		if (!m_Tracks.empty())
		{
			Serializer::SerializeAnimationTrack(m_Tracks[0].GetFilepath(), m_Tracks[0]);
		}
	}

	ImGui::End();
}

void EntityAnimatorEditor::DrawGraphEditor()
{
	ImGui::Begin("Animation Graph");

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	ImVec2 canvasPosition = ImGui::GetCursorScreenPos();
	ImVec2 canvasSize = ImGui::GetContentRegionAvail();

	DrawGrid(drawList, canvasPosition, canvasSize);

	constexpr float trackNameWidth = 100.0f;
	constexpr float trackHeight = 50.0f;

	uint32_t trackIndex = 0;
	for (auto& track : m_Tracks)
	{
		if (!track.visible) continue;

		const float yPosition = canvasPosition.y + m_GraphOffset.y + (trackIndex * trackHeight) + (trackHeight * 0.5f);

		const ImVec2 textSize = ImGui::CalcTextSize(track.GetName().c_str());
		drawList->AddText(
			ImVec2(canvasPosition.x + 5.0f, yPosition - textSize.y * 0.5f),
			IM_COL32(255, 255, 255, 255),
			track.GetName().c_str());

		for (auto& keyframe : track.keyframes)
		{
			ImVec2 position = canvasPosition;
			position.x += m_GraphOffset.x + (keyframe.time * m_Zoom * 50.0f) - m_ScrollX;
			position.y += m_GraphOffset.y + (trackIndex * 50.0f);

			ImU32 color = keyframe.selected ? IM_COL32(255, 0, 0, 255) : IM_COL32(0, 255, 0, 255);
			drawList->AddCircleFilled(position, 5.0f, color);

			if (&keyframe != &track.keyframes.back())
			{
				auto& next = *(&keyframe + 1);
				ImVec2 next_pos = canvasPosition;
				next_pos.x += m_GraphOffset.x + (next.time * m_Zoom * 50.0f) - m_ScrollX;
				next_pos.y += m_GraphOffset.y + (trackIndex * 50.0f);
				drawList->AddLine(position, next_pos, IM_COL32(255, 255, 255, 128), 1.0f);
			}
		}

		++trackIndex;
	}

	HandleUserInteractions();

	ImGui::End();
}

void EntityAnimatorEditor::DrawPropertiesPanel(Editor* editor)
{
	ImGui::Begin("Key Frame Properties", &editor->m_EntityAnimatorEditorOpened);

	if (m_SelectedKeyframe)
	{
		ImGui::Text("Keyframe at %.2fs", m_SelectedKeyframe->time);

		ImGui::DragFloat3("Translation", &m_SelectedKeyframe->translation.x, 0.1f);

		glm::vec3 euler = glm::degrees(m_SelectedKeyframe->rotation);
		if (ImGui::DragFloat3("Rotation", &euler.x, 1.0f))
		{
			m_SelectedKeyframe->rotation = glm::radians(euler);
		}

		ImGui::DragFloat3("Scale", &m_SelectedKeyframe->scale.x, 0.1f);

		const char* interpTypes[] = { "Linear", "Bezier", "Step" };
		ImGui::Combo("Interpolation", (int*)&m_SelectedKeyframe->interpType, interpTypes, 3);
	
		if (ImGui::Button("Copy"))
		{
			editor->m_CopyTransform.position = m_SelectedKeyframe->translation;
			editor->m_CopyTransform.rotation = m_SelectedKeyframe->rotation;
			editor->m_CopyTransform.scale = m_SelectedKeyframe->scale;
		}

		ImGui::SameLine();

		if (ImGui::Button("Paste"))
		{
			m_SelectedKeyframe->translation = editor->m_CopyTransform.position;
			m_SelectedKeyframe->rotation = editor->m_CopyTransform.rotation;
			m_SelectedKeyframe->scale = editor->m_CopyTransform.scale;
		}
	}
	else
	{
		ImGui::Text("No keyframe selected");
	}

	ImGui::End();
}

void EntityAnimatorEditor::HandleUserInteractions()
{
	if (!ImGui::IsWindowHovered())
	{
		return;
	}

	const ImGuiIO& io = ImGui::GetIO();
	const ImVec2 canvasPosition = ImGui::GetCursorScreenPos();
	constexpr float trackHeight = 50.0f;

	if (io.MouseWheel != 0.0f)
	{
		m_Zoom = glm::clamp(m_Zoom + io.MouseWheel * 0.1f, 0.1f, 5.0f);
	}

	if (ImGui::IsMouseDragging(ImGuiMouseButton_Right))
	{
		m_ScrollX -= io.MouseDelta.x;
	}

	if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
	{
		const ImVec2 mousePosition = io.MousePos;

		m_SelectedKeyframe = nullptr;
		for (auto& track : m_Tracks)
		{
			for (auto& keyframe : track.keyframes)
			{
				keyframe.selected = false;

				ImVec2 keyPosition = canvasPosition;
				keyPosition.x += m_GraphOffset.x + (keyframe.time * m_Zoom * 50.0f) - m_ScrollX;
				keyPosition.y += m_GraphOffset.y + (&track - &m_Tracks[0]) * 50.0f;

				if (glm::distance(glm::vec2(mousePosition.x, mousePosition.y),
					glm::vec2(keyPosition.x, keyPosition.y)) < 5.0f)
				{
					keyframe.selected = true;
					m_SelectedKeyframe = &keyframe;
				}
			}
		}

		if (!m_SelectedKeyframe)
		{
			const float relativeY = mousePosition.y - canvasPosition.y - m_GraphOffset.y;
			const int trackIndex = static_cast<int>(relativeY / trackHeight);

			if (trackIndex >= 0 && trackIndex < m_Tracks.size())
			{
				float clickedTime = (mousePosition.x - canvasPosition.x - m_GraphOffset.x + m_ScrollX) / (m_Zoom * 50.0f);

				if (io.KeyCtrl)
				{
					constexpr float gridSnap = 0.25f;
					clickedTime = roundf(clickedTime / gridSnap) * gridSnap;
				}

				if (clickedTime >= 0 && clickedTime <= m_TimelineLength)
				{
					m_Tracks[trackIndex].keyframes.push_back({
						clickedTime,
						glm::vec3(0.0f),
						glm::vec3(0.0f, 0.0f, 0.0f),
						glm::vec3(1.0f)
						});
					std::sort(m_Tracks[trackIndex].keyframes.begin(), m_Tracks[trackIndex].keyframes.end(),
						[](const EntityAnimator::Keyframe& a, const EntityAnimator::Keyframe& b) { return a.time < b.time; });
				}
			}
		}
	}

	if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
	{
		const ImVec2 mousePosition = io.MousePos;

		for (auto& track : m_Tracks)
		{
			auto foundKeyFrame = std::remove_if(track.keyframes.begin(), track.keyframes.end(),
				[&](const EntityAnimator::Keyframe& keyframe)
				{
					ImVec2 keyPosition = canvasPosition;
					keyPosition.x += m_GraphOffset.x + (keyframe.time * m_Zoom * 50.0f) - m_ScrollX;
					keyPosition.y += m_GraphOffset.y + (&track - &m_Tracks[0]) * 50.0f;

					const bool clicked = glm::distance(glm::vec2(mousePosition.x, mousePosition.y),
						glm::vec2(keyPosition.x, keyPosition.y)) < 5.0f;

					if (clicked && m_SelectedKeyframe == &keyframe)
					{
						m_SelectedKeyframe = nullptr;
					}

					return clicked;
				});

			if (foundKeyFrame != track.keyframes.end())
			{
				track.keyframes.erase(foundKeyFrame, track.keyframes.end());
			}
		}
	}

	if (m_SelectedKeyframe && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
	{
		const ImVec2 mousePosition = io.MousePos;
		const float newTime = (mousePosition.x - canvasPosition.x - m_GraphOffset.x + m_ScrollX) / (m_Zoom * 50.0f);
		m_SelectedKeyframe->time = glm::clamp(newTime, 0.0f, m_TimelineLength);

		std::sort(m_Tracks[0].keyframes.begin(), m_Tracks[0].keyframes.end(),
			[](const EntityAnimator::Keyframe& a, const EntityAnimator::Keyframe& b) { return a.time < b.time; });
	}
}

void EntityAnimatorEditor::DrawGrid(ImDrawList* drawList, ImVec2 canvasPosition, ImVec2 canvasSize)
{
	const float timeStep = 1.0f;
	const float minorStep = 0.2f;

	for (float t = 0.0f; t <= m_TimelineLength; t += timeStep)
	{
		float x = canvasPosition.x + m_GraphOffset.x + (t * m_Zoom * 50.0f) - m_ScrollX;

		if (x >= canvasPosition.x && x <= canvasPosition.x + canvasSize.x)
		{
			drawList->AddLine(
				ImVec2(x, canvasPosition.y + m_GraphOffset.y),
				ImVec2(x, canvasPosition.y + canvasSize.y),
				IM_COL32(100, 100, 100, 100), 1.0f);

			char label[32];
			snprintf(label, sizeof(label), "%.1fs", t);
			ImVec2 text_size = ImGui::CalcTextSize(label);
			drawList->AddText(
				ImVec2(x - text_size.x * 0.5f, canvasPosition.y + 5.0f),
				IM_COL32(255, 255, 255, 255),
				label);
		}
	}

	for (float t = 0.0f; t <= m_TimelineLength; t += minorStep)
	{
		float x = canvasPosition.x + m_GraphOffset.x + (t * m_Zoom * 50.0f) - m_ScrollX;

		if (x >= canvasPosition.x && x <= canvasPosition.x + canvasSize.x)
		{
			if (fmodf(t, timeStep) > 0.01f)
			{
				drawList->AddLine(
					ImVec2(x, canvasPosition.y + m_GraphOffset.y),
					ImVec2(x, canvasPosition.y + canvasSize.y),
					IM_COL32(70, 70, 70, 50), 0.5f);
			}
		}
	}

	for (size_t i = 0; i < m_Tracks.size(); ++i)
	{
		float y = canvasPosition.y + m_GraphOffset.y + (i * 50.0f);
		drawList->AddLine(
			ImVec2(canvasPosition.x, y),
			ImVec2(canvasPosition.x + canvasSize.x, y),
			IM_COL32(100, 100, 100, 50), 1.0f);
	}
}
