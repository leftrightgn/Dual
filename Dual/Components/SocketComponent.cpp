#include "pch.h"
#include "SocketComponent.h"
#include "Components/SkinnedModelComponent.h"
#include "Components/TransformComponent.h" // Fixed Include Path
#include "Entities/Actor.h"

HEIN::SocketComponent::SocketComponent(Actor* owner)
    : IComponent(owner)
    , m_model(nullptr)
    , m_transform(nullptr)
{
}

void HEIN::SocketComponent::Start()
{
    m_model = m_owner->GetComponent<SkinnedModelComponent>();
    m_transform = m_owner->GetComponent<TransformComponent>();
}

void HEIN::SocketComponent::UpdateSocketOffset(
    const std::wstring& socketName,
    const DirectX::SimpleMath::Vector3& newPos,
    const DirectX::SimpleMath::Vector3& newRot
)
{
    if (HasSocket(socketName))
    {
        m_sockets[socketName].localPosition = newPos;
        m_sockets[socketName].localRotation = newRot;
    }
}

void HEIN::SocketComponent::AddSocket(const Socket& socket)
{
    m_sockets[socket.name] = socket;
}

bool HEIN::SocketComponent::HasSocket(const std::wstring& socketName) const
{
    return m_sockets.find(socketName) != m_sockets.end();
}

HEIN::Socket* HEIN::SocketComponent::GetSocket(const std::wstring& socketName)
{
    if (HasSocket(socketName))
    {
        return &m_sockets[socketName];
    }
    return nullptr;
}

DirectX::SimpleMath::Matrix HEIN::SocketComponent::GetSocketWorldMatrix(const std::wstring& socketName)
{

    if (!HasSocket(socketName) || m_model == nullptr || m_transform == nullptr)
    {
        if (m_transform != nullptr)
        {
            return m_transform->GetWorldMatrix();
        }
        return DirectX::SimpleMath::Matrix::Identity;
    }

    const Socket& socket = m_sockets[socketName];

    DirectX::SimpleMath::Matrix ownerWorld = m_transform->GetWorldMatrix();
    DirectX::SimpleMath::Matrix boneWorld = m_model->GetBoneWorldMatrix(socket.boneName.c_str(), ownerWorld);

    DirectX::SimpleMath::Vector3 extractedScale;
    DirectX::SimpleMath::Quaternion extractedRotation;
    DirectX::SimpleMath::Vector3 extractedTranslation;

    if (boneWorld.Decompose(extractedScale, extractedRotation, extractedTranslation))
    {
        DirectX::SimpleMath::Matrix offsetMatrix =
            DirectX::SimpleMath::Matrix::CreateFromYawPitchRoll(socket.localRotation.y, socket.localRotation.x, socket.localRotation.z) *
            DirectX::SimpleMath::Matrix::CreateTranslation(socket.localPosition);

        DirectX::SimpleMath::Matrix cleanBoneMatrix =
            DirectX::SimpleMath::Matrix::CreateFromQuaternion(extractedRotation) *
            DirectX::SimpleMath::Matrix::CreateTranslation(extractedTranslation);

        return offsetMatrix * cleanBoneMatrix;
    }

    return ownerWorld;
}