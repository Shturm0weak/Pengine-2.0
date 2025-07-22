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
#include "Core/KeyCode.h"
#include "Core/FontManager.h"
#include "Core/ClayManager.h"
#include "Core/WindowManager.h"

#include "Components/Transform.h"
#include "Components/RigidBody.h"

#include "ComponentSystems/PhysicsSystem.h"

#define CLAY_IMPLEMENTATION
#include "Components/Canvas.h"

using namespace Pengine;

void ExampleApplication::OnPreStart()
{
}

void ExampleApplication::OnStart()
{
	/*scene = Serializer::DeserializeScene("Scenes/Examples/RifleWalk/Dance.scene");

	const auto floor = scene->CreateCube();
	floor->GetComponent<Transform>().Scale({ 10.0f, 1.0f, 10.0f });
	auto& floorBox = floor->AddComponent<RigidBody>();
	floorBox.type = RigidBody::Type::Box;
	floorBox.shape.box.halfExtents = { 10.0f, 1.0f, 10.0f };
	floorBox.isStatic = true;*/

	//for (size_t i = 0; i < 5; i++)
	//{
	//	for (size_t j = 0; j < 5; j++)
	//	{
	//		for (size_t k = 0; k < 5; k++)
	//		{
	//			const auto cube = scene->CreateEmpty();
	//			cube->GetComponent<Transform>().Translate({ i, 100.0f + k, j });
	//			auto& cubeBox = cube->AddComponent<RigidBody>();
	//			cubeBox.shape.box.halfExtents = { 1.0f, 1.0f, 1.0f };
	//			cubeBox.type = RigidBody::Type::Box;
	//			//cubeBox.shape.sphere.radius = 1.0f;
	//			cubeBox.isStatic = false;
	//		}
	//	}
	//}

	return;
	ClayManager::GetInstance().scriptsByName["FPS"] = [this](Canvas* canvas, std::shared_ptr<Entity> entity)
	{
		ClayManager::BeginLayout();

		auto font = FontManager::GetInstance().GetFont("Calibri", 72);

		const int scale = 2;

		const std::string fps = "FPS: " + std::to_string(1.0f / Time::GetDeltaTime());

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

		const std::string fps = "FPS: " + std::to_string(1.0f / Time::GetDeltaTime());

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
		const auto font = FontManager::GetInstance().GetFont("Calibri", 72);
		const auto viewport = WindowManager::GetInstance().GetWindowByName("Main")->GetViewportManager().GetViewport("Main");
		const auto mouseRay = viewport->GetMouseRay(viewport->GetMousePosition());
		auto& input = Input::GetInstance(WindowManager::GetInstance().GetWindowByName("Main").get());

		ClayManager::SetPointerState({ std::numeric_limits<float>().max(), std::numeric_limits<float>().max() }, input.IsMouseDown(KeyCode::MOUSE_BUTTON_1));

		Raycast::Hit hit{};
		if (const auto camera = viewport->GetCamera().lock())
		{
			if (viewport->IsHovered())
			{
				if (Raycast::RaycastEntity(entity, camera->GetComponent<Transform>().GetPosition(), mouseRay, 100.0f, hit))
				{
					ClayManager::SetPointerState({ hit.uv.x * canvas->size.x, hit.uv.y * canvas->size.y }, input.IsMouseDown(KeyCode::MOUSE_BUTTON_1));
				}
			}
		}

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

	//scene = Serializer::DeserializeScene("Scenes/Examples/Turret/Turret.scene");
}

#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>

void ExampleApplication::OnUpdate()
{
	/*const auto viewport = WindowManager::GetInstance().GetWindowByName("Main")->GetViewportManager().GetViewport("Main");
	const auto mouseRay = viewport->GetMouseRay(viewport->GetMousePosition());
	auto& input = Input::GetInstance(WindowManager::GetInstance().GetWindowByName("Main").get());

	auto physicsSystem = std::static_pointer_cast<PhysicsSystem>(scene->GetComponentSystem("PhysicsSystem"));
	auto& joltPhysicsSystem = physicsSystem->GetInstance();

	if (viewport->IsHovered() && input.IsMousePressed(KeyCode::MOUSE_BUTTON_1))
	{
		if (const auto camera = viewport->GetCamera().lock())
		{
			const auto sphere = scene->CreateSphere();
			sphere->GetComponent<Transform>().Translate(camera->GetComponent<Transform>().GetPosition());
			sphere->GetComponent<Transform>().Scale(glm::vec3(0.5f));
			auto& rigidBody = sphere->AddComponent<RigidBody>();
			rigidBody.type = RigidBody::Type::Sphere;
			rigidBody.shape.sphere.radius = 0.5f;
			rigidBody.isStatic = false;

			physicsSystem->UpdateBodies(scene);
			joltPhysicsSystem.GetBodyInterface().AddForce(rigidBody.id, GlmVec3ToJoltVec3(mouseRay * 20000.0f));
		}
	}*/

	/*Raycast::Hit hit{};
	if (const auto camera = viewport->GetCamera().lock())
	{
		if (viewport->IsHovered())
		{
			auto& physicsSystem = std::static_pointer_cast<PhysicsSystem>(scene->GetComponentSystem("PhysicsSystem"))->GetInstance();
			
			const JPH::Vec3 origin = GlmVec3ToJoltVec3(camera->GetComponent<Transform>().GetPosition());
			const JPH::Vec3 direction = GlmVec3ToJoltVec3(mouseRay * 100.0f);

			JPH::RayCastResult result;
			JPH::RRayCast ray(origin, direction);

			if (physicsSystem.GetNarrowPhaseQuery().CastRay(
				ray,
				result))
			{
				JPH::Vec3 point = origin + direction * result.mFraction;
				physicsSystem.GetBodyInterface().AddForce(result.mBodyID, direction, point, JPH::EActivation::Activate);
			}
		}
	}*/

	//return;
	//const auto viewport = WindowManager::GetInstance().GetWindowByName("Main")->GetViewportManager().GetViewport("Main");
	//const auto mouseRay = viewport->GetMouseRay(viewport->GetMousePosition());
	//auto& input = Input::GetInstance(WindowManager::GetInstance().GetWindowByName("Main").get());

	//Raycast::Hit hit{};
	//if (const auto camera = viewport->GetCamera().lock())
	//{
	//	if (viewport->IsHovered())
	//	{
	//		const auto hits = Raycast::RaycastScene(scene, camera->GetComponent<Transform>().GetPosition(), mouseRay, 100.0f);
	//		if (!hits.empty())
	//		{
	//			const glm::vec3 target = hits.rbegin()->first.point;
	//			//scene->GetVisualizer().DrawSphere(target, 0.3f, 12, { 1.0f, 0.0f, 0.0f });

	//			auto rotator = scene->FindEntityByName("Rotator");
	//			auto body = scene->FindEntityByName("Body");

	//			const glm::mat3 transformMat4 = glm::lookAt(rotator->GetComponent<Transform>().GetPosition(), target, { 0.0f, 1.0f, 0.0f });
	//			float pitch = asin(-transformMat4[1][2]);
	//			float yaw = atan2(transformMat4[0][2], transformMat4[2][2]);
	//			float roll = atan2(transformMat4[1][0], transformMat4[1][1]);
	//			glm::vec3 rotation = glm::vec3(pitch, yaw, roll);

	//			rotator->GetComponent<Transform>().Rotate({ 0.0f, 0.0f, rotation.y - glm::half_pi<float>() });
	//			body->GetComponent<Transform>().Rotate({ 0.0f, rotation.x, 0.0f });
	//		}
	//	}
	//}

	//auto barrel = scene->FindEntityByName("Barrel");
	//auto& entityAnimator = barrel->GetComponent<EntityAnimator>();
	//if (entityAnimator.animationTrack && !entityAnimator.animationTrack->keyframes.empty())
	//{
	//	if (glm::abs<float>(entityAnimator.animationTrack->keyframes[1].time - entityAnimator.time) <= 0.005f)
	//	{
	//		auto muzzle = scene->FindEntityByName("Muzzle");
	//		auto muzzlePosition = muzzle->GetComponent<Transform>().GetPosition();
	//		auto barrelPosition = barrel->GetComponent<Transform>().GetPosition();
	//		const auto hits = Raycast::RaycastScene(scene, muzzlePosition, glm::normalize(muzzlePosition - barrelPosition), 100.0f);
	//		//for (const auto& hit : hits)
	//		//{
	//		//	scene->GetVisualizer().DrawSphere(hit.first.point, 0.3f, 12, { 1.0f, 0.0f, 0.0f });
	//		//}

	//		if (!hits.empty())
	//		{
	//			scene->GetVisualizer().DrawSphere(hits.rbegin()->first.point, 0.3f, 12, { 1.0f, 0.0f, 0.0f }, 1.0f);
	//		}
	//	}
	//}
}

void ExampleApplication::OnClose()
{
	scene = nullptr;
}
