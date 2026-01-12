#pragma once

#include "Core/ReflectionSystem.h"
#include "Core/TextureManager.h"
#include "Core/FontManager.h"
#include "Core/Entity.h"

#include "Core/ClayManager.h"

#define CLAY_IMPLEMENTATION
#include "Components/Canvas.h"

REGISTER_CLAY_SCRIPT(Aim)
{
	Pengine::ClayManager::GetInstance().scriptsByName["Aim"] = [](Pengine::Canvas* canvas, std::shared_ptr<Pengine::Entity> entity)
		{
			Pengine::ClayManager::BeginLayout();

			Pengine::ClayManager::OpenElement();
			Pengine::ClayManager::ConfigureOpenElement(
				{
					.layout =
						{
							.sizing = {.width = CLAY_SIZING_GROW(), .height = CLAY_SIZING_GROW() },
							.childAlignment =
								{
									.x = Clay_LayoutAlignmentX::CLAY_ALIGN_X_CENTER,
									.y = Clay_LayoutAlignmentY::CLAY_ALIGN_Y_CENTER,
								},
						}
				});
			{
				Pengine::ClayManager::OpenElement();
				Pengine::ClayManager::ConfigureOpenElement(
					{
						.id = CLAY_ID("CrossHair"),
						.layout =
							{
								.sizing = {.width = CLAY_SIZING_FIXED(64), .height = CLAY_SIZING_FIXED(64) },
								.childAlignment =
								{
									.x = Clay_LayoutAlignmentX::CLAY_ALIGN_X_CENTER,
									.y = Clay_LayoutAlignmentY::CLAY_ALIGN_Y_CENTER,
								},
								.layoutDirection = Clay_LayoutDirection::CLAY_TOP_TO_BOTTOM,
							},
						.backgroundColor = { 1.0f, 0.0f, 0.0f, 1.0f },
						.image =
						{
							.imageData = Pengine::TextureManager::GetInstance().Load("Examples/FirstPerson/Assets/textures/CrossHair.png").get(),
						},
					});
				Pengine::ClayManager::CloseElement();
			}

			Pengine::ClayManager::CloseElement();

			return Pengine::ClayManager::EndLayout();
		};
}

REGISTER_CLAY_SCRIPT(Ammo)
{
	Pengine::ClayManager::GetInstance().scriptsByName["Ammo"] = [](Pengine::Canvas* canvas, std::shared_ptr<Pengine::Entity> entity)
		{
			Pengine::ClayManager::BeginLayout();

			if (!entity->HasComponent<FirstPersonCharacter>())
			{
				return Pengine::ClayManager::EndLayout();
			}

			FirstPersonCharacter& character = entity->GetComponent<FirstPersonCharacter>();

			auto font = Pengine::FontManager::GetInstance().GetFont("Calibri", 72);

			character.ammoUI = std::format("Ammo: {}/{}", character.currentMagazine, character.currentAmmo);

			Pengine::ClayManager::OpenElement();
			Pengine::ClayManager::ConfigureOpenElement(
				{
					.layout =
						{
							.sizing = {.width = CLAY_SIZING_GROW(), .height = CLAY_SIZING_GROW() },
							.childAlignment =
								{
									.x = Clay_LayoutAlignmentX::CLAY_ALIGN_X_RIGHT,
									.y = Clay_LayoutAlignmentY::CLAY_ALIGN_Y_BOTTOM,
								},
						}
				});
			{
				Pengine::ClayManager::OpenElement();
				Pengine::ClayManager::ConfigureOpenElement(
					{
						.layout =
							{
								.sizing = {.width = CLAY_SIZING_FIT(), .height = CLAY_SIZING_FIT() },
								.padding = { 20, 20, 20, 20 },
							}
					});
				{
					Clay_String ammoClayString{};
					ammoClayString.length = character.ammoUI.size();
					ammoClayString.chars = character.ammoUI.c_str();

					Pengine::ClayManager::OpenTextElement(
						ammoClayString,
						{
							.textColor = { 1.0f, 1.0f, 1.0f, 1.0f },
							.fontId = font->id,
							.fontSize = font->size,
						}
						);
				}

				Pengine::ClayManager::CloseElement();
			}

			Pengine::ClayManager::CloseElement();

			return Pengine::ClayManager::EndLayout();
		};
}
