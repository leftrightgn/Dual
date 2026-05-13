#pragma once
#include "Components/IComponent.h"
#include "Components/SkinnedModelComponent.h"
#include "TransformComponent.h" 
#include <string>
#include <unordered_map>
#include <SimpleMath.h>

namespace HEIN
{
    // A struct to define a single socket attachment point
    struct Socket
    {
        std::wstring name;
        std::wstring boneName;
        DirectX::SimpleMath::Vector3 localPosition;
        DirectX::SimpleMath::Vector3 localRotation; // X=Pitch, Y=Yaw, Z=Roll

        Socket()
            : name(L""), boneName(L"")
            , localPosition(DirectX::SimpleMath::Vector3::Zero)
            , localRotation(DirectX::SimpleMath::Vector3::Zero)
        {
        }

        Socket(const std::wstring& n, const std::wstring& bName,
            const DirectX::SimpleMath::Vector3& pos = DirectX::SimpleMath::Vector3::Zero,
            const DirectX::SimpleMath::Vector3& rot = DirectX::SimpleMath::Vector3::Zero)
            : name(n), boneName(bName), localPosition(pos), localRotation(rot)
        {
        }
    };

    class SocketComponent : public IComponent
    {
    private:
        std::unordered_map<std::wstring, Socket> m_sockets;

        SkinnedModelComponent* m_model;
        TransformComponent* m_transform;

    public:
        SocketComponent(Actor* owner)
            : IComponent(owner)
            , m_model(nullptr)
            , m_transform(nullptr)
        {
        }

        void Start() override
        {
      
            m_model = m_owner->GetComponent<SkinnedModelComponent>();
            m_transform = m_owner->GetComponent<TransformComponent>();
        }

        void Update(float /*deltaTime*/) override {}

        void UpdateSocketOffset(const std::wstring& socketName,
            const DirectX::SimpleMath::Vector3& newPos,
            const DirectX::SimpleMath::Vector3& newRot)
        {
            if (HasSocket(socketName))
            {
                m_sockets[socketName].localPosition = newPos;
                m_sockets[socketName].localRotation = newRot;
            }
        }

        void AddSocket(const Socket& socket)
        {
            m_sockets[socket.name] = socket;
        }

        bool HasSocket(const std::wstring& socketName) const
        {
            return m_sockets.find(socketName) != m_sockets.end();
        }

        Socket* GetSocket(const std::wstring& socketName)
        {
            if (HasSocket(socketName))
            {
                return &m_sockets[socketName];
            }
            return nullptr;
        }

        DirectX::SimpleMath::Matrix GetSocketWorldMatrix(const std::wstring& socketName)
        {
           
            SkinnedModelComponent* model = m_owner->GetComponent<SkinnedModelComponent>();
            TransformComponent* transform = m_owner->GetComponent<TransformComponent>();

           
            if (!HasSocket(socketName) || model == nullptr || transform == nullptr)
            {
                if (transform != nullptr)
                {
                    return transform->GetWorldMatrix();
                }
                return DirectX::SimpleMath::Matrix::Identity;
            }

            const Socket& socket = m_sockets[socketName];

            DirectX::SimpleMath::Matrix ownerWorld = transform->GetWorldMatrix();
            DirectX::SimpleMath::Matrix boneWorld = model->GetBoneWorldMatrix(socket.boneName.c_str(), ownerWorld);

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
    };
}