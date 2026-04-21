#pragma once
#include <vector>
#include <memory>
#include <string>
#include "Components/IComponent.h"
#include <utility>

class Actor
{
private:

	// Memory safe Array of Components
	std::vector<std::unique_ptr<IComponent>> m_components;

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

	void Render()
	{
		for (auto& comp : m_components)
		{
			comp->Render();
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
		for (auto& comp : m_components)
		{
			T* target = dynamic_cast<T*>(comp.get());
			if (target)
			{
				return target;
			}
		}
		return nullptr;
	}
};

