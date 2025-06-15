#include "ExampleApplication.h"

#include "Core/Core.h"
#include "Core/MeshManager.h"
#include "Core/TextureManager.h"
#include "Core/Serializer.h"
#include "Core/ViewportManager.h"
#include "Core/RenderPassManager.h"
#include "Core/Viewport.h"
#include "Core/SceneManager.h"
#include "Core/Time.h"
#include "Core/Input.h"
#include "Core/Keycode.h"
#include "Core/FontManager.h"
#include "Core/ClayManager.h"
#include "Core/WindowManager.h"

#define CLAY_IMPLEMENTATION
#include "Components/Canvas.h"

using namespace Pengine;

void ExampleApplication::OnPreStart()
{
}

void ExampleApplication::OnStart()
{
	ClayManager::GetInstance().scriptsByName["FPS"] = [this](Canvas* canvas, std::shared_ptr<Entity> entity)
	{
		ClayManager::BeginLayout();

		auto font = FontManager::GetInstance().GetFont("Calibri", 72);

		const int scale = 2;

		fps = "FPS: " + std::to_string(1.0f / Time::GetDeltaTime());

		ClayManager::OpenElement();
		ClayManager::ConfigureOpenElement(
			{
				.layout =
					{
						.sizing = { .width = CLAY_SIZING_FIXED(1024), .height = CLAY_SIZING_FIXED(1024) },
						.childAlignment =
							{
								.x = Clay_LayoutAlignmentX::CLAY_ALIGN_X_CENTER,
								.y = Clay_LayoutAlignmentY::CLAY_ALIGN_Y_CENTER,
							},
					}
			}
		);
		{
			ClayManager::OpenElement();
			ClayManager::ConfigureOpenElement(
				{
					.id = CLAY_ID("OuterContainer"),
					.layout =
						{
							.sizing = { .width = CLAY_SIZING_FIXED(1024), .height = CLAY_SIZING_FIXED(150) },
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
			);
			{
				Clay_String fpsClayString{};
				fpsClayString.length = fps.size();
				fpsClayString.chars = fps.c_str();

				ClayManager::OpenTextElement(
					fpsClayString,
					{
						.textColor = { 1.0f, 0.965f, 1.0f, 0.953f },
						.fontId = font->id,
						.fontSize = font->size,
						.textAlignment = Clay_TextAlignment::CLAY_TEXT_ALIGN_CENTER,
					}
				);
			}

			ClayManager::CloseElement();
		}

		ClayManager::CloseElement();

		return ClayManager::EndLayout();
	};

	ClayManager::GetInstance().scriptsByName["Test UI"] = [this](Canvas* canvas, std::shared_ptr<Entity> entity)
	{
		ClayManager::BeginLayout();

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

		ClayManager::OpenElement();
		ClayManager::ConfigureOpenElement(
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
		);
		{
			ClayManager::OpenElement();
			ClayManager::ConfigureOpenElement(
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
			);
			{
				for (const auto& item : items)
				{
					ClayManager::OpenElement();
					ClayManager::ConfigureOpenElement(
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
					);
					{
						ClayManager::OpenTextElement(
							item.name,
							{
								.textColor = { 1.0f, 1.0f, 0.0f, 1.0f },//{ 1.0f, 0.965f, 1.0f, 0.953f },
								.fontId = font->id,
								.fontSize = font->size,
							}
						);
						ClayManager::OpenElement();
						ClayManager::ConfigureOpenElement({ .layout = { .sizing = { .width = CLAY_SIZING_GROW(), .height  = CLAY_SIZING_GROW() } } });
						ClayManager::CloseElement();

						ClayManager::OpenElement();
						ClayManager::ConfigureOpenElement(
							{
								.layout =
									{
										.sizing = { .width = CLAY_SIZING_FIXED(32 * scale), .height = CLAY_SIZING_FIXED(32 * scale) },
									},
								.backgroundColor = { 1.0f, 1.0f, 1.0f, 1.0f },
								.image =
									{
										.imageData = item.texture.get(),
									},
							}
						);
						ClayManager::CloseElement();
					}

					ClayManager::CloseElement();
				}
			}

			ClayManager::CloseElement();
		}

		ClayManager::CloseElement();

		return ClayManager::EndLayout();
	};

	ClayManager::GetInstance().scriptsByName["Grid"] = [this](Canvas* canvas, std::shared_ptr<Entity> entity)
	{
		auto font = FontManager::GetInstance().GetFont("Calibri", 72);
		auto mousePosition = WindowManager::GetInstance().GetWindowByName("Main")->GetViewportManager().GetViewport("Main")->GetMousePosition();
		auto& input = Input::GetInstance(WindowManager::GetInstance().GetWindowByName("Main").get());
		ClayManager::SetPointerState({ mousePosition.x, mousePosition.y }, input.IsMouseDown(Keycode::MOUSE_BUTTON_1));

		ClayManager::BeginLayout();

		ClayManager::OpenElement();
		ClayManager::ConfigureOpenElement(
			{
				.id = CLAY_ID("OuterContainer"),
				.layout =
					{
						.sizing = { .width = CLAY_SIZING_FIXED((float)canvas->size.x), .height = CLAY_SIZING_FIXED((float)canvas->size.y) },
						.padding = { 16, 16, 16, 16 },
						.childGap = 16,
						.layoutDirection = CLAY_TOP_TO_BOTTOM,
					}
			}
		);
		{
			for (size_t i = 0; i < 8; i++)
			{
				ClayManager::OpenElement();
				ClayManager::ConfigureOpenElement(
					{
						.layout =
							{
								.sizing = { .width = CLAY_SIZING_GROW((float)canvas->size.x), .height = CLAY_SIZING_GROW(64) },
								.childGap = 16,
								.layoutDirection = CLAY_LEFT_TO_RIGHT,
							},
					}
				);
				{
					for (size_t j = 0; j < 8; j++)
					{
						ClayManager::OpenElement();
						ClayManager::ConfigureOpenElement(
							{
								.layout =
									{
										.sizing = { .width = CLAY_SIZING_GROW(64), .height = CLAY_SIZING_GROW(64) },
										.childAlignment = { { CLAY_ALIGN_X_CENTER }, { CLAY_ALIGN_Y_CENTER } },
									},
								.backgroundColor = ClayManager::IsHovered() ? Clay_Color{ 1.0f, 0.0f, 0.0f, 1.0f } : Clay_Color{ 0.5f, 0.5f, 0.5f, 1.0f },
								.cornerRadius = { 16, 16, 16, 16 },
							}
						);
						{
							ClayManager::OpenTextElement(
								CLAY_STRING("O"),
								{
									.textColor = { 1.0f, 1.0f, 1.0f, 1.0f },
									.fontId = font->id,
									.fontSize = font->size,
									.textAlignment = Clay_TextAlignment::CLAY_TEXT_ALIGN_CENTER,
								}
							);
						}

						ClayManager::CloseElement();
					}
				}

				ClayManager::CloseElement();
			}
		}

		ClayManager::CloseElement();

		return ClayManager::EndLayout();
	};
}

void ExampleApplication::OnUpdate()
{
}

void ExampleApplication::OnClose()
{
}
