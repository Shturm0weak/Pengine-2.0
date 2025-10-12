#pragma once

#include "../Core/Core.h"

#include <clay/clay.h>

namespace Pengine
{

	class PENGINE_API Canvas
	{
	public:

		struct Script
		{
			/**
			 * A function that contains clay functions for ui.
			 * CANVAS_BEGIN macro has to be called firstly inside of this function.
			 */
			std::function<Clay_RenderCommandArray(Canvas*, std::shared_ptr<class Entity>)> callback;
			std::string name;

			Clay_Context* context{};
		};

		std::vector<Script> scripts;

		std::vector<Clay_RenderCommandArray> commands{};

		Clay_Dimensions (*measureText)(Clay_StringSlice text, Clay_TextElementConfig *config, void *userData);
		Clay_Vector2 (*queryScrollOffset)(uint32_t elementId, void *userData);

		bool drawInMainViewport = false;
		glm::ivec2 size{};

		std::shared_ptr<class FrameBuffer> frameBuffer;
	};

}
