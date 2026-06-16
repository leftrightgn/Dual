#include "pch.h"
#include "PhysicsSystem.h"
#include "Entities/Actor.h"
#include "Components/RigidBodyComponent.h"
#include "Components/TransformComponent.h"
#include "Components/ColliderComponent/ColliderComponent.h"

void HEIN::PhysicsSystem::Update(GameContext& gameContext, std::vector<std::unique_ptr<HEIN::Actor>>& actors, float deltaTime)
{
    for (auto& actor : actors)
    {
        HEIN::RigidBodyComponent* rb = actor->GetComponent<HEIN::RigidBodyComponent>();
        if (rb) rb->m_isGrounded = false;  
        std::vector<HEIN::ColliderComponent*> actorColliders = actor->GetComponents<HEIN::ColliderComponent>();
        for (HEIN::ColliderComponent* col : actorColliders)
        {
            col->SetCollidingThisFrame(false);
        }
    }
    // (Apply Gravity and Velocity)
    for (std::unique_ptr<HEIN::Actor>& actor : actors)
    {
        HEIN::RigidBodyComponent* rigidBody = actor->GetComponent<HEIN::RigidBodyComponent>();
        HEIN::TransformComponent* transform = actor->GetComponent<HEIN::TransformComponent>();

       
        if (rigidBody != nullptr && !rigidBody->isKinematic() && transform != nullptr)
        {
            if (rigidBody->UsesGravity() && !rigidBody->m_isGrounded)
            {
                DirectX::SimpleMath::Vector3 gravityForce(0.0f, rigidBody->GRAVITY_FORCE, 0.0f);
                rigidBody->m_acceleration += gravityForce;
            }

            rigidBody->m_velocity += (rigidBody->m_acceleration * deltaTime);

            DirectX::SimpleMath::Vector3 currentPosition = transform->GetPosition();
            currentPosition += (rigidBody->m_velocity * deltaTime);
            transform->SetPosition(currentPosition);

            rigidBody->m_acceleration = DirectX::SimpleMath::Vector3::Zero;
        }
    }

    // GATHER ALL COLLIDERS
    std::vector<HEIN::ColliderComponent*> allColliders;
    for (std::unique_ptr<HEIN::Actor>& actor : actors)
    {
        std::vector<HEIN::ColliderComponent*> actorColliders = actor->GetComponents<HEIN::ColliderComponent>();
        for (HEIN::ColliderComponent* col : actorColliders)
        {
            allColliders.push_back(col);
        }
    }

    // COLLISION DETECTION & RESOLUTION
    for (size_t i = 0; i < allColliders.size(); ++i)
    {
        for (size_t j = i + 1; j < allColliders.size(); ++j)
        {
            HEIN::ColliderComponent* colA = allColliders[i];
            HEIN::ColliderComponent* colB = allColliders[j];
            
            if (colA->GetOwner() == colB->GetOwner())
            {
                continue;
            }

            bool aCanHitB = (colA->GetCollisionMask() & colB->GetCollisionLayer()) != 0;
            bool bCanHitA = (colB->GetCollisionMask() & colA->GetCollisionLayer()) != 0;

            if (!aCanHitB || !bCanHitA)
            {
                continue;
            }

            HEIN::CollisionManifold mainfold = HEIN::CollisionDispatcher::CheckCollision(colA, colB);
            if (mainfold.isColliding)
            {
                bool isATrigger = colA->IsTrigger();
                bool isBTrigger = colB->IsTrigger();

                colA->SetCollidingThisFrame(true);
                colB->SetCollidingThisFrame(true);

                if (isATrigger == false && isBTrigger == false)
                {
                    ResolvePhysicalOverlap(colA, colB, mainfold);
                }
                else
                {
                    HEIN::TriggerEventPayLoad payload;
                    payload.triggerA = colA;
                    payload.triggerB = colB;

                    gameContext.eventManager->DispatchTriggerEvent(payload);
                }
            }
        }
    }
}

void HEIN::PhysicsSystem::ResolvePhysicalOverlap(HEIN::ColliderComponent* colA, HEIN::ColliderComponent* colB, const HEIN::CollisionManifold& manifold)
{
    // Try to get the components for Actor A
    HEIN::Actor* actorA = colA->GetOwner();
    HEIN::RigidBodyComponent* rbA = actorA->GetComponent<HEIN::RigidBodyComponent>();
    HEIN::TransformComponent* transformA = actorA->GetComponent<HEIN::TransformComponent>();

    // Try to get the components for Actor B
    HEIN::Actor* actorB = colB->GetOwner();
    HEIN::RigidBodyComponent* rbB = actorB->GetComponent<HEIN::RigidBodyComponent>();
    HEIN::TransformComponent* transformB = actorB->GetComponent<HEIN::TransformComponent>();

    // ---------------------------------------------------------
    // If Actor A is the falling object (e.g., Player)
    // ---------------------------------------------------------
    if (rbA != nullptr && !rbA->isKinematic() && transformA != nullptr)
    {
        // Push the Transform UP out of the floor
        DirectX::SimpleMath::Vector3 currentPos = transformA->GetPosition();
        currentPos += (manifold.normal * manifold.penetrationDepth);
        transformA->SetPosition(currentPos);

        // Stop gravity from pulling them down further
        DirectX::SimpleMath::Vector3 currentVelocity = rbA->GetVelocity();
        float velocityIntoWall = currentVelocity.Dot(manifold.normal);

        if (velocityIntoWall < 0.0f) // Only remove velocity if falling INTO the floor
        {
            DirectX::SimpleMath::Vector3 fixedVelocity = currentVelocity - (manifold.normal * velocityIntoWall);
            rbA->SetVelocity(fixedVelocity);
            if (manifold.normal.y > 0.5f) rbA->m_isGrounded = true;
        }
    }
    

    // ---------------------------------------------------------
    // If Actor B is the falling object (e.g., Player)
    // ---------------------------------------------------------
    else if (rbB != nullptr && !rbB->isKinematic() && transformB != nullptr)
    {
        
        DirectX::SimpleMath::Vector3 flippedNormal = manifold.normal * -1.0f;

        DirectX::SimpleMath::Vector3 position = transformB->GetPosition();

        position += flippedNormal * manifold.penetrationDepth;

        transformB->SetPosition(position);

        DirectX::SimpleMath::Vector3 currentVelocity = rbB->GetVelocity();

        float velocityIntoWall = currentVelocity.Dot(flippedNormal);

        if (velocityIntoWall < 0.0f)
        {
            DirectX::SimpleMath::Vector3 fixedVelocity = currentVelocity - (flippedNormal * velocityIntoWall);
            rbB->SetVelocity(fixedVelocity);

            if (flippedNormal.y > 0.5f) rbB->m_isGrounded = true;
        }
    }
}

