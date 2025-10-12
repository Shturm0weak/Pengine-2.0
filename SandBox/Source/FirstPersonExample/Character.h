#pragma once

#include "Core/ReflectionSystem.h"
#include "Core/TextureManager.h"
#include "Core/ClayManager.h"
#include "Graphics/SkeletalAnimation.h"
#include "StateMachine.h"

#define CLAY_IMPLEMENTATION
#include "Components/Canvas.h"

namespace JPH
{
	class Character;
}

struct Enemy
{
	PROPERTY(float, health, 100.0f);
};
REGISTER_CLASS(Enemy);

struct FirstPersonCharacter
{
	std::map<State, std::shared_ptr<Pengine::SkeletalAnimation>> animations;
	std::map<State, std::shared_ptr<CharacterState>> states;
	
	std::shared_ptr<class JPH::Character> joltCharacter;
	float height = 2.0f;
	float radius = 0.5f;

	float shotTimer = 0.0f;

	std::string ammoUI;

	CharacterState* currentState;

	bool isJumpPressed = false;
	bool isMovePressed = false;
	bool isShotPressed = false;

	public: PROPERTY(int, currentAmmo, 90);
	public: PROPERTY(int, currentMagazine, 30);
	public: PROPERTY(int, maxMagazine, 30);
	public: PROPERTY(float, speed, 1.0f);
	public: PROPERTY(float, jump, 1.0f);
	public: PROPERTY(float, bulletForce, 100.0f);
	public: PROPERTY(float, damage, 30.0f);
};
REGISTER_CLASS(FirstPersonCharacter);

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

#include "Core/FontManager.h"
#include "Core/Entity.h"

REGISTER_CLAY_SCRIPT(Ammo)
{
	Pengine::ClayManager::GetInstance().scriptsByName["Ammo"] = [](Pengine::Canvas* canvas, std::shared_ptr<Pengine::Entity> entity)
		{
			Pengine::ClayManager::BeginLayout();

			if (!entity->HasComponent<FirstPersonCharacter>())
			{
				Pengine::ClayManager::EndLayout();
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
