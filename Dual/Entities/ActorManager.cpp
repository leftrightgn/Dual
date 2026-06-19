#include "pch.h"
#include "ActorManager.h"

HEIN::Actor* HEIN::ActorManager::CreateActor(const std::wstring& tag)
{
    ActorID newID = m_nextID++;
    auto newActor = std::make_unique<Actor>(newID, tag);

    Actor* ptr = newActor.get();
    m_actors[newID] = std::move(newActor);

    return ptr;
}

void HEIN::ActorManager::DestroyID(ActorID id)
{
    m_actors.erase(id);
}

HEIN::Actor* HEIN::ActorManager::GetActor(ActorID id)
{
    auto it = m_actors.find(id);
    if (it != m_actors.end())
    {
        return it->second.get();
    }
    return nullptr;
}

void HEIN::ActorManager::UpdateAll(float deltaTime)
{
    for (auto& pair : m_actors)
    {
        pair.second->Update(deltaTime);
    }
}
