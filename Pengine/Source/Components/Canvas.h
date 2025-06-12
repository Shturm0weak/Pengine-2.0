#pragma once

#include "../Core/Core.h"

#include <clay/clay.h>

namespace Pengine
{

	class PENGINE_API Canvas
	{
	public:

		/**
		 * A function that contains clay functions for ui.
		 * CANVAS_BEGIN macro has to be called firstly inside of this function.
		 */
		std::function<void(Canvas* canvas, std::shared_ptr<class Entity>)> script;
		std::string scriptName;

		Clay_Context* context{};
		Clay_RenderCommandArray commands{};

		Clay_Dimensions (*measureText)(Clay_StringSlice text, Clay_TextElementConfig *config, void *userData);
		Clay_Vector2 (*queryScrollOffset)(uint32_t elementId, void *userData);

		bool drawInMainViewport = false;
		glm::ivec2 size{};

		std::shared_ptr<class FrameBuffer> frameBuffer;
	};

#define CANVAS_BEGIN(canvas)                                          \
	Clay_SetCurrentContext(canvas->context);                  \
	Clay_SetMeasureTextFunction(canvas->measureText, nullptr);\
	Clay_BeginLayout();                                       \

}
