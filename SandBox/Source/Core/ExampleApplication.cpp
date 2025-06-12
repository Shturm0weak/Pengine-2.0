#include "Core/Core.h"
#include "ExampleApplication.h"

#include "Core/MeshManager.h"
#include "Core/TextureManager.h"
#include "Core/Serializer.h"
#include "Core/ViewportManager.h"
#include "Core/RenderPassManager.h"
#include "Core/Viewport.h"
#include "Core/SceneManager.h"
#include "Core/Time.h"

#include "Components/Transform.h"

#define CLAY_IMPLEMENTATION
#include "Components/Canvas.h"

#include "Core/FontManager.h"
#include "Core/ClayScriptManager.h"

using namespace Pengine;

void ExampleApplication::OnPreStart()
{
}

void ExampleApplication::OnStart()
{
	ClayScriptManager::GetInstance().scriptsByName["FPS"] = [this](Canvas* canvas, std::shared_ptr<Entity> entity)
	{
		CANVAS_BEGIN(canvas)

		auto font = FontManager::GetInstance().GetFont("Calibri", 72);

		const int scale = 2;

		fps = "FPS: " + std::to_string(1.0f / Time::GetDeltaTime());

		CLAY(
			{
				.layout =
					{
						.sizing = {.width = CLAY_SIZING_FIXED(1024), .height = CLAY_SIZING_FIXED(1024) },
						.childAlignment =
							{
								.x = Clay_LayoutAlignmentX::CLAY_ALIGN_X_CENTER,
								.y = Clay_LayoutAlignmentY::CLAY_ALIGN_Y_CENTER,
							},
					}
			}
		)
		{
			CLAY(
				{
					.id = CLAY_ID("OuterContainer"),
					.layout =
						{
							.sizing = {.width = CLAY_SIZING_FIXED(1024), .height = CLAY_SIZING_FIXED(150) },
							.padding = { 8 * scale, 8 * scale, 8 * scale, 8 * scale },
							.childGap = 8 * scale,
							.childAlignment =
							{
								.x = Clay_LayoutAlignmentX::CLAY_ALIGN_X_CENTER,
								.y = Clay_LayoutAlignmentY::CLAY_ALIGN_Y_CENTER,
							},
							.layoutDirection = Clay_LayoutDirection::CLAY_TOP_TO_BOTTOM,
						},
					.backgroundColor = { 0.9f, 0.695f, 0.726f, 1.0f },
					.cornerRadius = { 8 * scale, 8 * scale, 8 * scale, 8 * scale },
				}
				)
				{
					Clay_String fpsClayString{};
					fpsClayString.length = fps.size();
					fpsClayString.chars = fps.c_str();

					CLAY_TEXT(
						fpsClayString,
						CLAY_TEXT_CONFIG(
							{
								.textColor = { 1.0f, 0.965f, 1.0f, 0.953f },
								.fontId = font->id,
								.fontSize = font->size,
								.textAlignment = Clay_TextAlignment::CLAY_TEXT_ALIGN_CENTER,
								//.letterSpacing = 12,
								//.lineHeight = 12 * scale,
							}
						)
					);
				}
		}
	};

	ClayScriptManager::GetInstance().scriptsByName["Test UI"] = [this](Canvas* canvas, std::shared_ptr<Entity> entity)
	{
		CANVAS_BEGIN(canvas)

		auto font = FontManager::GetInstance().GetFont("Calibri", 72);

		struct Item
		{
			Clay_String name;
			std::shared_ptr<Texture> texture;
		};

		std::vector<Item> items;
		items.emplace_back(Item{ CLAY_STRING("File"), TextureManager::GetInstance().Load("Editor/Images/FileIcon.png") });
		items.emplace_back(Item{ CLAY_STRING("Folder"), TextureManager::GetInstance().Load("Editor/Images/FolderIcon.png") });
		items.emplace_back(Item{ CLAY_STRING("Material"), TextureManager::GetInstance().Load("Editor/Images/MaterialIcon.png") });

		const int scale = 2;

		fps = "FPS: " + std::to_string(1.0f / Time::GetDeltaTime());

		CLAY(
			{
				.layout =
					{
						.sizing = { .width = CLAY_SIZING_GROW(), .height = CLAY_SIZING_GROW() },
						.childAlignment =
							{
								.x = Clay_LayoutAlignmentX::CLAY_ALIGN_X_CENTER,
								.y = Clay_LayoutAlignmentY::CLAY_ALIGN_Y_CENTER,
							},
					}
			}
		)
		{
		CLAY(
			{
				.id = CLAY_ID("OuterContainer"),
				.layout =
					{
						.sizing = { .width = CLAY_SIZING_FIT(), .height = CLAY_SIZING_FIT() },
						.padding = { 8 * scale, 8 * scale, 8 * scale, 8 * scale },
						.childGap = 8 * scale,
						.layoutDirection = Clay_LayoutDirection::CLAY_TOP_TO_BOTTOM,
					},
				.backgroundColor = { 0.9f, 0.695f, 0.726f, 1.0f },
				.cornerRadius = { 8 * scale, 8 * scale, 8 * scale, 8 * scale },
			}
		)
		{
			for (const auto& item : items)
			{
				CLAY(
					{
						.layout =
							{
								.sizing = { .width = CLAY_SIZING_GROW(), .height = CLAY_SIZING_GROW() },
								.padding = { 16 * scale, 12 * scale, 12 * scale, 12 * scale },
								.childAlignment = { .x = Clay_LayoutAlignmentX::CLAY_ALIGN_X_LEFT, .y = Clay_LayoutAlignmentY::CLAY_ALIGN_Y_CENTER },
							},
						.backgroundColor = { 0.976f, 0.8125f, 0.765625f, 1.0f },
						.cornerRadius = { 8 * scale, 8 * scale, 8 * scale, 8 * scale },
					}
				)
				{
					CLAY_TEXT(
						item.name,
						CLAY_TEXT_CONFIG(
							{
								.textColor = { 1.0f, 1.0f, 0.0f, 1.0f },//{ 1.0f, 0.965f, 1.0f, 0.953f },
								.fontId = font->id,
								.fontSize = font->size,
							}
						)
					);
					CLAY({ .layout = { .sizing = { .width = CLAY_SIZING_GROW(), .height  = CLAY_SIZING_GROW() } } });
					CLAY(
						{
							.layout =
								{
									.sizing = { .width = CLAY_SIZING_FIXED(32 * scale), .height = CLAY_SIZING_FIXED(32 * scale) },
								},
							.backgroundColor = { 1.0f, 1.0f, 1.0f, 1.0f },
							.image =
								{
									.imageData = item.texture.get(),
									.sourceDimensions = { .width = (float)item.texture->GetSize().x, .height = (float)item.texture->GetSize().y },
								},
						}
					);
				}
			}
		}
		}
	};
}

void ExampleApplication::OnUpdate()
{
}

void ExampleApplication::OnClose()
{
}
