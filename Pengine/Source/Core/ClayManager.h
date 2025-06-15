#pragma once

#include "Core.h"

#include <clay/clay.h>

namespace Pengine
{

	class PENGINE_API ClayManager
	{
	public:
		static ClayManager& GetInstance();

		ClayManager(const ClayManager&) = delete;
		ClayManager& operator=(const ClayManager&) = delete;

		static void Init(class Canvas* canvas);

		static void BeginLayout();

		static Clay_RenderCommandArray EndLayout();

		static void OpenElement();

		static void ConfigureOpenElement(const Clay_ElementDeclaration& config);

		static void CloseElement();

		static void OpenTextElement(const Clay_String& text, const Clay_TextElementConfig& config);

		static void SetPointerState(const Clay_Vector2& position, bool isPointerDown);

		static bool IsHovered();

		static bool IsPointerOver(Clay_ElementId elementId);

		std::unordered_map<std::string, std::function<Clay_RenderCommandArray(class Canvas* canvas, std::shared_ptr<class Entity>)>> scriptsByName;

	private:
		ClayManager() = default;
		~ClayManager() = default;
	};

}
