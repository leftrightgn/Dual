#include "pch.h"
#include "ActorFactory.h"
#include <Components/TransformComponent.h>
#include <Components/ColliderComponent/CapsuleColliderComponent.h>
#include <Components/TwoBoneLinkComponent.h>
#include <Components/SocketComponent.h>
#include <BlackBoard/CombatBlackBoard.h>
#include <Components/PlayerInputComponent.h>
#include <Components/CharacterMovementComponent.h>
#include <Components/CombatStateMachineComponent.h>
#include <Components/StaticModelComponent.h>
#include <Components/ColliderComponent/OBBColliderComponent.h>
#include <Components/SocketAttachmentComponent.h>
#include <Components/BoneLinkComponent.h>

HEIN::PlayerSpawnData HEIN::ActorFactory::CreateKnight(
    GameContext& gameContext, 
    CameraController* cameraController,
    DirectX::SimpleMath::Vector3* targetCameraOut
)
{
    HEIN::PlayerSpawnData spawnData;

    spawnData.playerActor = std::make_unique<HEIN::Actor>(L"Player");

    HEIN::TransformComponent* ptransform = spawnData.playerActor->AddComponent<HEIN::TransformComponent>();
    ptransform->SetPosition(DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f));
    ptransform->SetScale(DirectX::SimpleMath::Vector3(0.1f));

    // ThirdPersonCamera model
    spawnData.tpsModel = spawnData.playerActor->AddComponent<HEIN::SkinnedModelComponent>();
    spawnData.tpsModel->Initialize(gameContext,
        L"Resources/Models/knight/knight.sdkmesh", // normal model
        L"Resources/Models/knight");
    spawnData.tpsModel->LoadAnimation("Idle", L"Resources/Models/knight/idle.sdkmesh_anim");
    spawnData.tpsModel->LoadAnimation("Walk", L"Resources/Models/knight/running.sdkmesh_anim");
    spawnData.tpsModel->LoadAnimation("OneHand", L"Resources/Models/knight/swing.sdkmesh_anim");

    // FirstPersonCamera model
    spawnData.fpsModel = spawnData.playerActor->AddComponent<HEIN::SkinnedModelComponent>();
    spawnData.fpsModel->Initialize(gameContext,
        L"Resources/Models/knight/knight.sdkmesh", // headless/arms model
        L"Resources/Models/knight");
    spawnData.fpsModel->LoadAnimation("Idle", L"Resources/Models/knight/idle.sdkmesh_anim");
    spawnData.fpsModel->LoadAnimation("Walk", L"Resources/Models/knight/running.sdkmesh_anim");
    spawnData.fpsModel->LoadAnimation("OneHand", L"Resources/Models/knight/swing.sdkmesh_anim");


    // Head Collider
    HEIN::CapsuleColliderComponent* HeadCapsule = spawnData.playerActor->AddComponent<HEIN::CapsuleColliderComponent>();
    HeadCapsule->Initialize(1.5f, 1.0f);
    HEIN::BoneLinkComponent* HeadLink = spawnData.playerActor->AddComponent<HEIN::BoneLinkComponent>();
    HeadLink->Initialize(spawnData.tpsModel, L"mixamorig:Head");
    HeadLink->LinkTo(HeadCapsule);

    // Body Collider
    HEIN::CapsuleColliderComponent* BodyCapsule = spawnData.playerActor->AddComponent<HEIN::CapsuleColliderComponent>();
    BodyCapsule->Initialize(2.0f, 0.0f);
    HEIN::TwoBoneLinkComponent* BodyLink = spawnData.playerActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    BodyLink->Initialize(spawnData.tpsModel, L"mixamorig:Spine2", L"mixamorig:Hips");
    BodyLink->LinkTo(BodyCapsule);

    // Right Arm Collider
    HEIN::CapsuleColliderComponent* RightarmCapsule = spawnData.playerActor->AddComponent<HEIN::CapsuleColliderComponent>();
    RightarmCapsule->Initialize(1.0f, 1.0f);
    HEIN::TwoBoneLinkComponent* RightarmLink = spawnData.playerActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    RightarmLink->Initialize(spawnData.tpsModel, L"mixamorig:RightArm", L"mixamorig:RightForeArm");
    RightarmLink->LinkTo(RightarmCapsule);
    HEIN::CapsuleColliderComponent* RightforearmCapsule = spawnData.playerActor->AddComponent<HEIN::CapsuleColliderComponent>();
    RightforearmCapsule->Initialize(0.6f, 1.0f);
    HEIN::TwoBoneLinkComponent* RightforearmLink = spawnData.playerActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    RightforearmLink->Initialize(spawnData.tpsModel, L"mixamorig:RightForeArm", L"mixamorig:RightHand");
    RightforearmLink->LinkTo(RightforearmCapsule);


    // Left Arm Collider
    HEIN::CapsuleColliderComponent* LeftarmCapsule = spawnData.playerActor->AddComponent<HEIN::CapsuleColliderComponent>();
    LeftarmCapsule->Initialize(1.0f, 1.0f);
    HEIN::TwoBoneLinkComponent* LeftarmLink = spawnData.playerActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    LeftarmLink->Initialize(spawnData.tpsModel, L"mixamorig:LeftArm", L"mixamorig:LeftForeArm");
    LeftarmLink->LinkTo(LeftarmCapsule);
    HEIN::CapsuleColliderComponent* LeftforearmCapsule = spawnData.playerActor->AddComponent<HEIN::CapsuleColliderComponent>();
    LeftforearmCapsule->Initialize(0.6f, 1.0f);
    HEIN::TwoBoneLinkComponent* LeftforearmLink = spawnData.playerActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    LeftforearmLink->Initialize(spawnData.tpsModel, L"mixamorig:LeftForeArm", L"mixamorig:LeftHand");
    LeftforearmLink->LinkTo(LeftforearmCapsule);

    // Right Leg Collider

    // Left Leg Collider

    // Socket
    HEIN::SocketComponent* socketComp = spawnData.playerActor->AddComponent<HEIN::SocketComponent>();
    HEIN::Socket weaponSocket(
        L"WeaponSocket",
        L"mixamorig:RightHandThumb4",
        DirectX::SimpleMath::Vector3(-0.770f, -1.280f, -0.550f),
        DirectX::SimpleMath::Vector3(2.172f, 0.670f, 1.280f)
    );
    socketComp->AddSocket(weaponSocket);

    // Connect
    spawnData.playerActor->AddComponent<HEIN::CombatBlackBoard>();
    spawnData.playerActor->AddComponent<HEIN::PlayerInputComponent>(cameraController); // Requires the camera controller
    spawnData.playerActor->AddComponent<HEIN::CharacterMovementComponent>();
    spawnData.playerActor->AddComponent<HEIN::CombatStateMachineComponent>();

    spawnData.playerActor->Start();
    return spawnData;
}

std::unique_ptr<HEIN::Actor> HEIN::ActorFactory::CreateSword(GameContext& gameContext, HEIN::SocketComponent* targetPlayerSocket)
{
    std::unique_ptr<HEIN::Actor> sword = std::make_unique<HEIN::Actor>(L"Sword");
    HEIN::TransformComponent* swordTransform = sword->AddComponent<HEIN::TransformComponent>();

    swordTransform->SetScale(DirectX::SimpleMath::Vector3(2.0f));

    HEIN::StaticModelComponent* swordModel = sword->AddComponent<HEIN::StaticModelComponent>();
    swordModel->Initialize(
        gameContext,
        L"Resources/Models/knight/sword.sdkmesh",
        L"Resources/Models/knight"
    );
    HEIN::OBBColliderComponent* swordHitBox = sword->AddComponent<HEIN::OBBColliderComponent>();

    swordHitBox->Initialize(DirectX::SimpleMath::Vector3(0.3f, 0.1f, 2.5f));
    swordHitBox->SetOffset(DirectX::SimpleMath::Vector3(0.0f, 0.0f, -3.3f));
    swordHitBox->SetRotationOffset(
        DirectX::SimpleMath::Vector3(
            0.0f,
            0.0f,
            0.0f
        )
    );

    if (targetPlayerSocket != nullptr)
    {
        HEIN::SocketAttachmentComponent* socketAttachment = sword->AddComponent<HEIN::SocketAttachmentComponent>();
        socketAttachment->Initialize(targetPlayerSocket, L"WeaponSocket");
    }
    sword->Start();
    return sword;
}
