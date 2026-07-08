#include "pch.h"
#include "Messenger.h"

std::unique_ptr<Messenger> Messenger::s_messenger = nullptr;

Messenger::Messenger()
    :
    m_elapsedTime{},
    m_objects{},
    m_delayedMessages{}
{
}

IObserver* Messenger::GetObject(int actorID)
{
    std::unordered_map<int, IObserver*>::iterator it = m_objects.find(actorID);

    if (it != m_objects.end())
    {
        return it->second;
    }
    return nullptr;
}

Messenger* Messenger::GetInstance()
{
    if (s_messenger == nullptr)
    {
        s_messenger = std::unique_ptr<Messenger>(new Messenger());
    }
    return s_messenger.get();
}

void Messenger::DestroyInstance()
{
    s_messenger.reset();
}

void Messenger::Register(int actorID, IObserver* observer)
{
    m_objects.emplace(actorID, observer);
}

void Messenger::UnRegister(int actorID)
{
    m_objects.erase(actorID);
}

void Messenger::Notify(int actorID, Message::MessageID messageID)
{
    std::unordered_map<int, IObserver*>::iterator it = m_objects.find(actorID);

    if (it != m_objects.end())
    {
        it->second->OnMessageAccepted(messageID);
    }
}

void Messenger::NotifyAfterDelay(int actorID, Message::MessageID messageID, float delaySeconds)
{
    m_delayedMessages.push_back({ actorID, messageID, delaySeconds });
}

void Messenger::UpdateDelayedMessage(float elapsedTime)
{
    for (std::vector<DelayedMessage>::iterator it = m_delayedMessages.begin();
        it != m_delayedMessages.end();)
    {
        it->delayTime -= elapsedTime;
        
        if (it->delayTime <= 0.0f)
        {
            Notify(it->ActorID, it->messageID);
            it = m_delayedMessages.erase(it);
        }
        else
        {
            it++;
        }
    }
}

void Messenger::Update(float elapsedTime)
{
    m_elapsedTime = elapsedTime;
    UpdateDelayedMessage(elapsedTime);
}
