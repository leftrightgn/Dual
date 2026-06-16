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
#include <Components/ColliderComponent/AABBColliderComponent.h>
#include <Components/RigidBodyComponent.h>

HEIN::PlayerSpawnData HEIN::ActorFactory::CreateKnight(
    GameContext& gameContext, 
    CameraController* cameraController,
    DirectX::SimpleMath::Vector3* targetCameraOut
)
{
    HEIN::PlayerSpawnData spawnData;

    spawnData.playerActor = std::make_unique<HEIN::Actor>(L"Player");

    HEIN::TransformComponent* ptransform = spawnData.playerActor->AddComponent<HEIN::TransformComponent>();
    ptransform->SetPosition(DirectX::SimpleMath::Vector3(0.0f, 4.0f, 0.0f));
    ptransform->SetScale(DirectX::SimpleMath::Vector3(0.10f));

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
    HeadCapsule->SetCollisionLayer(CollisionLayer::Layer_Player);
    HeadCapsule->SetCollisionMask(CollisionLayer::Layer_Enemy | CollisionLayer::Layer_EnemyWeapon);
    HEIN::BoneLinkComponent* HeadLink = spawnData.playerActor->AddComponent<HEIN::BoneLinkComponent>();
    HeadLink->Initialize(spawnData.tpsModel, L"mixamorig:Head");
    HeadLink->LinkTo(HeadCapsule);

    // Body Collider
    HEIN::CapsuleColliderComponent* BodyCapsule = spawnData.playerActor->AddComponent<HEIN::CapsuleColliderComponent>();
    BodyCapsule->Initialize(2.0f, 0.0f);
    BodyCapsule->SetCollisionLayer(CollisionLayer::Layer_Player);
    BodyCapsule->SetCollisionMask(CollisionLayer::Layer_Enemy | CollisionLayer::Layer_EnemyWeapon);
    HEIN::TwoBoneLinkComponent* BodyLink = spawnData.playerActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    BodyLink->Initialize(spawnData.tpsModel, L"mixamorig:Spine2", L"mixamorig:Hips");
    BodyLink->LinkTo(BodyCapsule);

    // Right Arm Collider
    HEIN::CapsuleColliderComponent* RightarmCapsule = spawnData.playerActor->AddComponent<HEIN::CapsuleColliderComponent>();
    RightarmCapsule->Initialize(1.0f, 1.0f);
    RightarmCapsule->SetCollisionLayer(CollisionLayer::Layer_Player);
    RightarmCapsule->SetCollisionMask(CollisionLayer::Layer_Enemy | CollisionLayer::Layer_EnemyWeapon);
    HEIN::TwoBoneLinkComponent* RightarmLink = spawnData.playerActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    RightarmLink->Initialize(spawnData.tpsModel, L"mixamorig:RightArm", L"mixamorig:RightForeArm");
    RightarmLink->LinkTo(RightarmCapsule);
    HEIN::CapsuleColliderComponent* RightforearmCapsule = spawnData.playerActor->AddComponent<HEIN::CapsuleColliderComponent>();
    RightforearmCapsule->Initialize(0.6f, 1.0f);
    RightforearmCapsule->SetCollisionLayer(CollisionLayer::Layer_Player);
    RightforearmCapsule->SetCollisionMask(CollisionLayer::Layer_Enemy | CollisionLayer::Layer_EnemyWeapon);
    HEIN::TwoBoneLinkComponent* RightforearmLink = spawnData.playerActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    RightforearmLink->Initialize(spawnData.tpsModel, L"mixamorig:RightForeArm", L"mixamorig:RightHand");
    RightforearmLink->LinkTo(RightforearmCapsule);


    // Left Arm Collider
    HEIN::CapsuleColliderComponent* LeftarmCapsule = spawnData.playerActor->AddComponent<HEIN::CapsuleColliderComponent>();
    LeftarmCapsule->Initialize(1.0f, 1.0f);
    LeftarmCapsule->SetCollisionLayer(CollisionLayer::Layer_Player);
    LeftarmCapsule->SetCollisionMask(CollisionLayer::Layer_Enemy | CollisionLayer::Layer_EnemyWeapon);
    HEIN::TwoBoneLinkComponent* LeftarmLink = spawnData.playerActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    LeftarmLink->Initialize(spawnData.tpsModel, L"mixamorig:LeftArm", L"mixamorig:LeftForeArm");
    LeftarmLink->LinkTo(LeftarmCapsule);
    HEIN::CapsuleColliderComponent* LeftforearmCapsule = spawnData.playerActor->AddComponent<HEIN::CapsuleColliderComponent>();
    LeftforearmCapsule->Initialize(0.6f, 1.0f);
    LeftforearmCapsule->SetCollisionLayer(CollisionLayer::Layer_Player);
    LeftforearmCapsule->SetCollisionMask(CollisionLayer::Layer_Enemy | CollisionLayer::Layer_EnemyWeapon);
    HEIN::TwoBoneLinkComponent* LeftforearmLink = spawnData.playerActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    LeftforearmLink->Initialize(spawnData.tpsModel, L"mixamorig:LeftForeArm", L"mixamorig:LeftHand");
    LeftforearmLink->LinkTo(LeftforearmCapsule);

    // Right Leg Collider
    HEIN::CapsuleColliderComponent* RightupLegCapsule = spawnData.playerActor->AddComponent<HEIN::CapsuleColliderComponent>();
    RightupLegCapsule->Initialize(1.0f, 0.0f);
    RightupLegCapsule->SetCollisionLayer(CollisionLayer::Layer_Player);
    RightupLegCapsule->SetCollisionMask(CollisionLayer::Layer_Enemy | CollisionLayer::Layer_EnemyWeapon);
    HEIN::TwoBoneLinkComponent* RightupLegLink = spawnData.playerActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    RightupLegLink->Initialize(spawnData.tpsModel, L"mixamorig:RightUpLeg", L"RightLeg");
    RightupLegLink->LinkTo(RightupLegCapsule);
    HEIN::CapsuleColliderComponent* RightLegCapsule = spawnData.playerActor->AddComponent<HEIN::CapsuleColliderComponent>();
    RightLegCapsule->Initialize(0.7f, 0.0f);
    RightLegCapsule->SetCollisionLayer(CollisionLayer::Layer_Player);
    RightLegCapsule->SetCollisionMask(CollisionLayer::Layer_Enemy | CollisionLayer::Layer_EnemyWeapon);
    HEIN::TwoBoneLinkComponent* RightLegLink = spawnData.playerActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    RightLegLink->Initialize(spawnData.tpsModel, L"mixamorig:RightLeg", L"mixamorig:RightFoot");
    RightLegLink->LinkTo(RightLegCapsule);
    /*HEIN::OBBColliderComponent* RightFoot = spawnData.playerActor->AddComponent<HEIN::OBBColliderComponent>();
    RightFoot->Initialize(DirectX::SimpleMath::Vector3(6.0f, 12.0f, 3.0f));
    RightFoot->SetOffset(DirectX::SimpleMath::Vector3(0.0f, -6.0f, -3.0f));
    RightFoot->SetColliderTag(L"RightFoot");
    HEIN::BoneLinkComponent* RightFootLink = spawnData.playerActor->AddComponent<HEIN::BoneLinkComponent>();
    RightFootLink->Initialize(spawnData.tpsModel, L"mixamorig:RightToeBase");
    RightFootLink->LinkTo(RightFoot);*/

    // Left Leg Collider
    HEIN::CapsuleColliderComponent* LeftupLegCapsule = spawnData.playerActor->AddComponent<HEIN::CapsuleColliderComponent>();
    LeftupLegCapsule->Initialize(1.0f, 0.0f);
    LeftupLegCapsule->SetCollisionLayer(CollisionLayer::Layer_Player);
    LeftupLegCapsule->SetCollisionMask(CollisionLayer::Layer_Enemy | CollisionLayer::Layer_EnemyWeapon);
    HEIN::TwoBoneLinkComponent* LeftupLegLink = spawnData.playerActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    LeftupLegLink->Initialize(spawnData.tpsModel, L"mixamorig:LeftUpLeg", L"mixamorig:LeftLeg");
    LeftupLegLink->LinkTo(LeftupLegCapsule);
    HEIN::CapsuleColliderComponent* LeftLegCapsule = spawnData.playerActor->AddComponent<HEIN::CapsuleColliderComponent>();
    LeftLegCapsule->Initialize(0.7f, 0.0f);
    LeftLegCapsule->SetCollisionLayer(CollisionLayer::Layer_Player);
    LeftLegCapsule->SetCollisionMask(CollisionLayer::Layer_Enemy | CollisionLayer::Layer_EnemyWeapon);
    HEIN::TwoBoneLinkComponent* LeftLegLink = spawnData.playerActor->AddComponent < HEIN::TwoBoneLinkComponent>();
    LeftLegLink->Initialize(spawnData.tpsModel, L"mixamorig:LeftLeg", L"mixamorig:LeftFoot");
    LeftLegLink->LinkTo(LeftLegCapsule);
    /*HEIN::OBBColliderComponent* LeftFoot = spawnData.playerActor->AddComponent<HEIN::OBBColliderComponent>();
    LeftFoot->Initialize(DirectX::SimpleMath::Vector3(6.0f, 12.0f, 3.0f));
    LeftFoot->SetOffset(DirectX::SimpleMath::Vector3(0.0f, -6.0f, -3.0f));
    LeftFoot->SetColliderTag(L"LeftFoot");
    HEIN::BoneLinkComponent* LeftFootLink = spawnData.playerActor->AddComponent<HEIN::BoneLinkComponent>();
    LeftFootLink->Initialize(spawnData.tpsModel, L"mixamorig:LeftToeBase");
    LeftFootLink->LinkTo(LeftFoot);*/



    // Socket
    HEIN::SocketComponent* socketComp = spawnData.playerActor->AddComponent<HEIN::SocketComponent>();
    HEIN::Socket weaponSocket(
        L"WeaponSocket",
        L"mixamorig:RightHandThumb4",
        DirectX::SimpleMath::Vector3(-0.770f, -1.280f, -0.550f),
        DirectX::SimpleMath::Vector3(2.172f, 0.670f, 1.280f)
    );
    socketComp->AddSocket(weaponSocket);

    HEIN::RigidBodyComponent* rigidBody = spawnData.playerActor->AddComponent<HEIN::RigidBodyComponent>();
    rigidBody->Initialize(80.0f, true, false);
    HEIN::CapsuleColliderComponent* rootPushbox = spawnData.playerActor->AddComponent<HEIN::CapsuleColliderComponent>();
    rootPushbox->Initialize(3.0f, 12.0f); // Adjust height to match your Knight
    rootPushbox->SetOffset(DirectX::SimpleMath::Vector3(0.0f, 90.0f, 0.0f));
    rootPushbox->SetTrigger(false);      // This one physically hits the floor
    rootPushbox->SetColliderTag(L"PlayerRoot");
   
  

    // SET BONES TO TRIGGERS (So they don't push the floor)
    HeadCapsule->SetTrigger(true);
    BodyCapsule->SetTrigger(true);
    RightarmCapsule->SetTrigger(true);
    RightforearmCapsule->SetTrigger(true);
    LeftarmCapsule->SetTrigger(true);
    LeftforearmCapsule->SetTrigger(true);
    RightupLegCapsule->SetTrigger(true);
    RightLegCapsule->SetTrigger(true);
    //RightFoot->SetTrigger(true);
    LeftupLegCapsule->SetTrigger(true);
    LeftLegCapsule->SetTrigger(true);
    //LeftFoot->SetTrigger(true);

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
    swordHitBox->SetTrigger(true);
    swordHitBox->SetCollisionLayer(CollisionLayer::Layer_PlayerWeapon);
    swordHitBox->SetCollisionMask(
        CollisionLayer::Layer_Enemy |
        CollisionLayer::Layer_EnemyWeapon |
        CollisionLayer::Layer_Environment
    );
    if (targetPlayerSocket != nullptr)
    {
        HEIN::SocketAttachmentComponent* socketAttachment = sword->AddComponent<HEIN::SocketAttachmentComponent>();
        socketAttachment->Initialize(targetPlayerSocket, L"WeaponSocket");
    }
    sword->Start();
    return sword;
}

std::unique_ptr<HEIN::Actor> HEIN::ActorFactory::CreateStage(GameContext& gameContext)
{
    std::unique_ptr<HEIN::Actor> stageActor = std::make_unique<HEIN::Actor>(L"Stage");
    HEIN::TransformComponent* tranStage = stageActor->AddComponent<HEIN::TransformComponent>();

    tranStage->SetPosition(DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f));
    tranStage->SetScale(DirectX::SimpleMath::Vector3(10.0f));

    HEIN::StaticModelComponent* stageModel = stageActor->AddComponent<HEIN::StaticModelComponent>();

    stageModel->Initialize(gameContext, L"Resources/Models/stage/floor.sdkmesh", L"Resources/Models/stage");

    HEIN::AABBColliderComponent* floor = stageActor->AddComponent<HEIN::AABBColliderComponent>();
    floor->InitializeFromModel(stageModel);
    floor->SetExtents(DirectX::SimpleMath::Vector3(10.4f, 0.06f, 10.4f));
    floor->SetOffset(DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f));
    floor->SetTrigger(false);
    floor->SetColliderTag(L"Floor");
   

    HEIN::AABBColliderComponent* wall1 = stageActor->AddComponent<HEIN::AABBColliderComponent>();
    wall1->Initialize(DirectX::SimpleMath::Vector3(0.08f, 1.0f, 10.0f));
    wall1->SetOffset(DirectX::SimpleMath::Vector3(10.0f, 1.0f, 0.0f));
    wall1->SetRotationOffset(DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f));
    wall1->SetTrigger(false);
    wall1->SetColliderTag(L"Wall1");
   
    HEIN::AABBColliderComponent* wall2 = stageActor->AddComponent<HEIN::AABBColliderComponent>();
    wall2->Initialize(DirectX::SimpleMath::Vector3(10.0f, 1.0f, 0.08f));
    wall2->SetOffset(DirectX::SimpleMath::Vector3(0.0f, 1.0f, 10.0f));
    wall2->SetRotationOffset(DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f));
    wall2->SetTrigger(false);
    wall2->SetColliderTag(L"Wall2");
   
    HEIN::AABBColliderComponent* wall3 = stageActor->AddComponent<HEIN::AABBColliderComponent>();
    wall3->Initialize(DirectX::SimpleMath::Vector3(0.08f, 1.0f, 10.4f));
    wall3->SetOffset(DirectX::SimpleMath::Vector3(-10.0f, 1.0f, 0.0f));
    wall3->SetRotationOffset(DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f));
    wall3->SetTrigger(false);
    wall3->SetColliderTag(L"Wall3");
  
    HEIN::RigidBodyComponent* stageBody = stageActor->AddComponent<HEIN::RigidBodyComponent>();
    stageBody->Initialize(0.0f, false, true);

    stageActor->Start();
    return stageActor;
}

HEIN::EnemySpawnData HEIN::ActorFactory::CreateEnemy(GameContext& gameContext)
{
    HEIN::EnemySpawnData spawnData;

    spawnData.enemyActor = std::make_unique<HEIN::Actor>(L"Enemy");

    HEIN::TransformComponent* ptransform = spawnData.enemyActor->AddComponent<HEIN::TransformComponent>();
    ptransform->SetPosition(DirectX::SimpleMath::Vector3(25.0f, 4.0f, 0.0f));
    ptransform->SetScale(DirectX::SimpleMath::Vector3(0.10f));

    // ThirdPersonCamera model
    spawnData.tpsModel = spawnData.enemyActor->AddComponent<HEIN::SkinnedModelComponent>();
    spawnData.tpsModel->Initialize(gameContext,
        L"Resources/Models/knight/knight.sdkmesh", // normal model
        L"Resources/Models/knight");
    spawnData.tpsModel->LoadAnimation("Idle", L"Resources/Models/knight/idle.sdkmesh_anim");
    spawnData.tpsModel->LoadAnimation("Walk", L"Resources/Models/knight/running.sdkmesh_anim");
    spawnData.tpsModel->LoadAnimation("OneHand", L"Resources/Models/knight/swing.sdkmesh_anim");

    
    // Head Collider
    HEIN::CapsuleColliderComponent* HeadCapsule = spawnData.enemyActor->AddComponent<HEIN::CapsuleColliderComponent>();
    HeadCapsule->Initialize(1.5f, 1.0f);
    HeadCapsule->SetCollisionLayer(CollisionLayer::Layer_Enemy);
    HEIN::BoneLinkComponent* HeadLink = spawnData.enemyActor->AddComponent<HEIN::BoneLinkComponent>();
    HeadLink->Initialize(spawnData.tpsModel, L"mixamorig:Head");
    HeadLink->LinkTo(HeadCapsule);

    // Body Collider
    HEIN::CapsuleColliderComponent* BodyCapsule = spawnData.enemyActor->AddComponent<HEIN::CapsuleColliderComponent>();
    BodyCapsule->Initialize(2.0f, 0.0f);
    BodyCapsule->SetCollisionLayer(CollisionLayer::Layer_Enemy);
    HEIN::TwoBoneLinkComponent* BodyLink = spawnData.enemyActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    BodyLink->Initialize(spawnData.tpsModel, L"mixamorig:Spine2", L"mixamorig:Hips");
    BodyLink->LinkTo(BodyCapsule);

    // Right Arm Collider
    HEIN::CapsuleColliderComponent* RightarmCapsule = spawnData.enemyActor->AddComponent<HEIN::CapsuleColliderComponent>();
    RightarmCapsule->Initialize(1.0f, 1.0f);
    RightarmCapsule->SetCollisionLayer(CollisionLayer::Layer_Enemy);
    HEIN::TwoBoneLinkComponent* RightarmLink = spawnData.enemyActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    RightarmLink->Initialize(spawnData.tpsModel, L"mixamorig:RightArm", L"mixamorig:RightForeArm");
    RightarmLink->LinkTo(RightarmCapsule);
    HEIN::CapsuleColliderComponent* RightforearmCapsule = spawnData.enemyActor->AddComponent<HEIN::CapsuleColliderComponent>();
    RightforearmCapsule->Initialize(0.6f, 1.0f);
    RightforearmCapsule->SetCollisionLayer(CollisionLayer::Layer_Enemy);
    HEIN::TwoBoneLinkComponent* RightforearmLink = spawnData.enemyActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    RightforearmLink->Initialize(spawnData.tpsModel, L"mixamorig:RightForeArm", L"mixamorig:RightHand");
    RightforearmLink->LinkTo(RightforearmCapsule);


    // Left Arm Collider
    HEIN::CapsuleColliderComponent* LeftarmCapsule = spawnData.enemyActor->AddComponent<HEIN::CapsuleColliderComponent>();
    LeftarmCapsule->Initialize(1.0f, 1.0f);
    LeftarmCapsule->SetCollisionLayer(CollisionLayer::Layer_Enemy);
    HEIN::TwoBoneLinkComponent* LeftarmLink = spawnData.enemyActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    LeftarmLink->Initialize(spawnData.tpsModel, L"mixamorig:LeftArm", L"mixamorig:LeftForeArm");
    LeftarmLink->LinkTo(LeftarmCapsule);
    HEIN::CapsuleColliderComponent* LeftforearmCapsule = spawnData.enemyActor->AddComponent<HEIN::CapsuleColliderComponent>();
    LeftforearmCapsule->Initialize(0.6f, 1.0f);
    LeftforearmCapsule->SetCollisionLayer(CollisionLayer::Layer_Enemy);
    HEIN::TwoBoneLinkComponent* LeftforearmLink = spawnData.enemyActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    LeftforearmLink->Initialize(spawnData.tpsModel, L"mixamorig:LeftForeArm", L"mixamorig:LeftHand");
    LeftforearmLink->LinkTo(LeftforearmCapsule);

    // Right Leg Collider
    HEIN::CapsuleColliderComponent* RightupLegCapsule = spawnData.enemyActor->AddComponent<HEIN::CapsuleColliderComponent>();
    RightupLegCapsule->Initialize(1.0f, 0.0f);
    RightupLegCapsule->SetCollisionLayer(CollisionLayer::Layer_Enemy);
    HEIN::TwoBoneLinkComponent* RightupLegLink = spawnData.enemyActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    RightupLegLink->Initialize(spawnData.tpsModel, L"mixamorig:RightUpLeg", L"RightLeg");
    RightupLegLink->LinkTo(RightupLegCapsule);
    HEIN::CapsuleColliderComponent* RightLegCapsule = spawnData.enemyActor->AddComponent<HEIN::CapsuleColliderComponent>();
    RightLegCapsule->Initialize(0.7f, 0.0f);
    RightLegCapsule->SetCollisionLayer(CollisionLayer::Layer_Enemy);
    HEIN::TwoBoneLinkComponent* RightLegLink = spawnData.enemyActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    RightLegLink->Initialize(spawnData.tpsModel, L"mixamorig:RightLeg", L"mixamorig:RightFoot");
    RightLegLink->LinkTo(RightLegCapsule);
   /* HEIN::OBBColliderComponent* RightFoot = spawnData.enemyActor->AddComponent<HEIN::OBBColliderComponent>();
    RightFoot->Initialize(DirectX::SimpleMath::Vector3(6.0f, 12.0f, 3.0f));
    RightFoot->SetOffset(DirectX::SimpleMath::Vector3(0.0f, -6.0f, -3.0f));
    RightFoot->SetColliderTag(L"eRightFoot");
    HEIN::BoneLinkComponent* RightFootLink = spawnData.enemyActor->AddComponent<HEIN::BoneLinkComponent>();
    RightFootLink->Initialize(spawnData.tpsModel, L"mixamorig:RightToeBase");
    RightFootLink->LinkTo(RightFoot);*/

    // Left Leg Collider
    HEIN::CapsuleColliderComponent* LeftupLegCapsule = spawnData.enemyActor->AddComponent<HEIN::CapsuleColliderComponent>();
    LeftupLegCapsule->Initialize(1.0f, 0.0f);
    LeftupLegCapsule->SetCollisionLayer(CollisionLayer::Layer_Enemy);
    HEIN::TwoBoneLinkComponent* LeftupLegLink = spawnData.enemyActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    LeftupLegLink->Initialize(spawnData.tpsModel, L"mixamorig:LeftUpLeg", L"mixamorig:LeftLeg");
    LeftupLegLink->LinkTo(LeftupLegCapsule);
    HEIN::CapsuleColliderComponent* LeftLegCapsule = spawnData.enemyActor->AddComponent<HEIN::CapsuleColliderComponent>();
    LeftLegCapsule->Initialize(0.7f, 0.0f);
    LeftLegCapsule->SetCollisionLayer(CollisionLayer::Layer_Enemy);
    HEIN::TwoBoneLinkComponent* LeftLegLink = spawnData.enemyActor->AddComponent < HEIN::TwoBoneLinkComponent>();
    LeftLegLink->Initialize(spawnData.tpsModel, L"mixamorig:LeftLeg", L"mixamorig:LeftFoot");
    LeftLegLink->LinkTo(LeftLegCapsule);
    /*HEIN::OBBColliderComponent* LeftFoot = spawnData.enemyActor->AddComponent<HEIN::OBBColliderComponent>();
    LeftFoot->Initialize(DirectX::SimpleMath::Vector3(6.0f, 12.0f, 3.0f));
    LeftFoot->SetOffset(DirectX::SimpleMath::Vector3(0.0f, -6.0f, -3.0f));
    LeftFoot->SetColliderTag(L"eLeftFoot");
    HEIN::BoneLinkComponent* LeftFootLink = spawnData.enemyActor->AddComponent<HEIN::BoneLinkComponent>();
    LeftFootLink->Initialize(spawnData.tpsModel, L"mixamorig:LeftToeBase");
    LeftFootLink->LinkTo(LeftFoot);*/



    // Socket
    HEIN::SocketComponent* socketComp = spawnData.enemyActor->AddComponent<HEIN::SocketComponent>();
    HEIN::Socket weaponSocket(
        L"WeaponSocket",
        L"mixamorig:RightHandThumb4",
        DirectX::SimpleMath::Vector3(-0.770f, -1.280f, -0.550f),
        DirectX::SimpleMath::Vector3(2.172f, 0.670f, 1.280f)
    );
    socketComp->AddSocket(weaponSocket);

    HEIN::RigidBodyComponent* rigidBody = spawnData.enemyActor->AddComponent<HEIN::RigidBodyComponent>();
    rigidBody->Initialize(80.0f, true, false);
    HEIN::CapsuleColliderComponent* rootPushbox = spawnData.enemyActor->AddComponent<HEIN::CapsuleColliderComponent>();
    rootPushbox->Initialize(3.0f, 12.0f); // Adjust height to match your Knight
    rootPushbox->SetOffset(DirectX::SimpleMath::Vector3(0.0f, 90.0f, 0.0f));
    rootPushbox->SetTrigger(false);      // This one physically hits the floor
    rootPushbox->SetColliderTag(L"EnemyRoot");
    

    // SET BONES TO TRIGGERS (So they don't push the floor)
    HeadCapsule->SetTrigger(true);
    BodyCapsule->SetTrigger(true);
    RightarmCapsule->SetTrigger(true);
    RightforearmCapsule->SetTrigger(true);
    LeftarmCapsule->SetTrigger(true);
    LeftforearmCapsule->SetTrigger(true);
    RightupLegCapsule->SetTrigger(true);
    RightLegCapsule->SetTrigger(true);
    //RightFoot->SetTrigger(true);
    LeftupLegCapsule->SetTrigger(true);
    LeftLegCapsule->SetTrigger(true);
    //LeftFoot->SetTrigger(true);



    spawnData.enemyActor->Start();
    return spawnData;
}
