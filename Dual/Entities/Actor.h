#pragma once
#include <vector>
#include <memory>
#include <string>
#include "Components/IComponent.h"
#include <utility>
#include <Components/TransfromComponent.h>

namespace HEIN
{

	class Actor
	{
	private:

		// Memory safe Array of Components
		std::vector<std::unique_ptr<HEIN::IComponent>> m_components;

		std::wstring m_tag;

	public:

		Actor(const std::wstring& tag = L"Actor")
			: m_tag(tag)
		{

		}

		~Actor() = default;

		void Update(float deltaTime)
		{
			for (auto& comp : m_components)
			{
				comp->Update(deltaTime);
			}
		}

		void Draw(GameContext& gameContext, const DirectX::SimpleMath::Matrix& view, const DirectX::SimpleMath::Matrix& proj)
		{
			TransformComponent* transform = GetComponent<TransformComponent>();
			if (!transform) return;

			DirectX::SimpleMath::Matrix world = transform->GetWorldMatrix();

			
			for (auto& component : m_components)
			{
				component->Draw(gameContext, world, view, proj);
			}
		}

		void Start()
		{
			// Initialize all attached components
			for (auto& comp : m_components)
			{
				comp->Start();
			}
		}


		// Template  Components
		// Creates a component, adds it to the Actor, and returns a pointer to it
		template <typename T, typename... TArgs>
		T* AddComponent(TArgs&&... mArgs)
		{
			// Create the new component, passing 'this' as the owner, plus any other arguments
			std::unique_ptr<T> newComponent = std::make_unique<T>(this, std::forward<TArgs>(mArgs)...);
			T* result = newComponent.get();

			m_components.push_back(std::move(newComponent));
			return result;
		}

		// Searches the Actor for a specific component type
		template <typename T>
		T* GetComponent()
		{
			for (std::unique_ptr<HEIN::IComponent>& comp : m_components)
			{
				T* target = dynamic_cast<T*>(comp.get());
				if (target != nullptr)
				{
					return target;
				}
			}
			return nullptr;
		}

		template <typename T>
		std::vector<T*> GetComponents()
		{
			std::vector<T*> result;
			for (std::unique_ptr<HEIN::IComponent>& comp : m_components)
			{
				T* target = dynamic_cast<T*>(comp.get());
				if (target != nullptr)
				{
					result.push_back(target);
				}
			}
			return result;
		}
	};

}