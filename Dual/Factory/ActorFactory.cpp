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
#include <Entities/ActorManager.h>

HEIN::PlayerSpawnData HEIN::ActorFactory::CreateKnight(
    ActorManager& actorManager,
    GameContext& gameContext, 
    CameraController* cameraController,
    DirectX::SimpleMath::Vector3* targetCameraOut
)
{
    HEIN::PlayerSpawnData spawnData;

    HEIN::Actor* playerActor = actorManager.CreateActor(L"Player");

    spawnData.playerID = playerActor->GetID();

    HEIN::TransformComponent* ptransform = playerActor->AddComponent<HEIN::TransformComponent>();
    ptransform->SetPosition(DirectX::SimpleMath::Vector3(0.0f, 4.0f, 0.0f));
    ptransform->SetScale(DirectX::SimpleMath::Vector3(0.10f));

    // ThirdPersonCamera model
    spawnData.tpsModel = playerActor->AddComponent<HEIN::SkinnedModelComponent>();
    spawnData.tpsModel->Initialize(gameContext,
        L"Resources/Models/knight/knight.sdkmesh", // normal model
        L"Resources/Models/knight");
    spawnData.tpsModel->LoadAnimation("Idle", L"Resources/Models/knight/idle.sdkmesh_anim");
    spawnData.tpsModel->LoadAnimation("Walk", L"Resources/Models/knight/running.sdkmesh_anim");
    spawnData.tpsModel->LoadAnimation("OneHand", L"Resources/Models/knight/swing.sdkmesh_anim");

    // FirstPersonCamera model
    spawnData.fpsModel = playerActor->AddComponent<HEIN::SkinnedModelComponent>();
    spawnData.fpsModel->Initialize(gameContext,
        L"Resources/Models/knight/knight.sdkmesh", // headless/arms model
        L"Resources/Models/knight");
    spawnData.fpsModel->LoadAnimation("Idle", L"Resources/Models/knight/idle.sdkmesh_anim");
    spawnData.fpsModel->LoadAnimation("Walk", L"Resources/Models/knight/running.sdkmesh_anim");
    spawnData.fpsModel->LoadAnimation("OneHand", L"Resources/Models/knight/swing.sdkmesh_anim");


    // Head Collider
    HEIN::CapsuleColliderComponent* HeadCapsule = playerActor->AddComponent<HEIN::CapsuleColliderComponent>();
    HeadCapsule->Initialize(1.5f, 1.0f);
    HeadCapsule->SetCollisionLayer(CollisionLayer::Layer_Player);
    HeadCapsule->SetCollisionMask(CollisionLayer::Layer_Enemy | CollisionLayer::Layer_EnemyWeapon);
    HEIN::BoneLinkComponent* HeadLink = playerActor->AddComponent<HEIN::BoneLinkComponent>();
    HeadLink->Initialize(spawnData.tpsModel, L"mixamorig:Head");
    HeadLink->LinkTo(HeadCapsule);

    // Body Collider
    HEIN::CapsuleColliderComponent* BodyCapsule = playerActor->AddComponent<HEIN::CapsuleColliderComponent>();
    BodyCapsule->Initialize(2.0f, 0.0f);
    BodyCapsule->SetCollisionLayer(CollisionLayer::Layer_Player);
    BodyCapsule->SetCollisionMask(CollisionLayer::Layer_Enemy | CollisionLayer::Layer_EnemyWeapon);
    HEIN::TwoBoneLinkComponent* BodyLink = playerActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    BodyLink->Initialize(spawnData.tpsModel, L"mixamorig:Spine2", L"mixamorig:Hips");
    BodyLink->LinkTo(BodyCapsule);

    // Right Arm Collider
    HEIN::CapsuleColliderComponent* RightarmCapsule = playerActor->AddComponent<HEIN::CapsuleColliderComponent>();
    RightarmCapsule->Initialize(1.0f, 1.0f);
    RightarmCapsule->SetCollisionLayer(CollisionLayer::Layer_Player);
    RightarmCapsule->SetCollisionMask(CollisionLayer::Layer_Enemy | CollisionLayer::Layer_EnemyWeapon);
    HEIN::TwoBoneLinkComponent* RightarmLink = playerActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    RightarmLink->Initialize(spawnData.tpsModel, L"mixamorig:RightArm", L"mixamorig:RightForeArm");
    RightarmLink->LinkTo(RightarmCapsule);
    HEIN::CapsuleColliderComponent* RightforearmCapsule = playerActor->AddComponent<HEIN::CapsuleColliderComponent>();
    RightforearmCapsule->Initialize(0.6f, 1.0f);
    RightforearmCapsule->SetCollisionLayer(CollisionLayer::Layer_Player);
    RightforearmCapsule->SetCollisionMask(CollisionLayer::Layer_Enemy | CollisionLayer::Layer_EnemyWeapon);
    HEIN::TwoBoneLinkComponent* RightforearmLink = playerActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    RightforearmLink->Initialize(spawnData.tpsModel, L"mixamorig:RightForeArm", L"mixamorig:RightHand");
    RightforearmLink->LinkTo(RightforearmCapsule);


    // Left Arm Collider
    HEIN::CapsuleColliderComponent* LeftarmCapsule = playerActor->AddComponent<HEIN::CapsuleColliderComponent>();
    LeftarmCapsule->Initialize(1.0f, 1.0f);
    LeftarmCapsule->SetCollisionLayer(CollisionLayer::Layer_Player);
    LeftarmCapsule->SetCollisionMask(CollisionLayer::Layer_Enemy | CollisionLayer::Layer_EnemyWeapon);
    HEIN::TwoBoneLinkComponent* LeftarmLink = playerActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    LeftarmLink->Initialize(spawnData.tpsModel, L"mixamorig:LeftArm", L"mixamorig:LeftForeArm");
    LeftarmLink->LinkTo(LeftarmCapsule);
    HEIN::CapsuleColliderComponent* LeftforearmCapsule = playerActor->AddComponent<HEIN::CapsuleColliderComponent>();
    LeftforearmCapsule->Initialize(0.6f, 1.0f);
    LeftforearmCapsule->SetCollisionLayer(CollisionLayer::Layer_Player);
    LeftforearmCapsule->SetCollisionMask(CollisionLayer::Layer_Enemy | CollisionLayer::Layer_EnemyWeapon);
    HEIN::TwoBoneLinkComponent* LeftforearmLink = playerActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    LeftforearmLink->Initialize(spawnData.tpsModel, L"mixamorig:LeftForeArm", L"mixamorig:LeftHand");
    LeftforearmLink->LinkTo(LeftforearmCapsule);

    // Right Leg Collider
    HEIN::CapsuleColliderComponent* RightupLegCapsule = playerActor->AddComponent<HEIN::CapsuleColliderComponent>();
    RightupLegCapsule->Initialize(1.0f, 0.0f);
    RightupLegCapsule->SetCollisionLayer(CollisionLayer::Layer_Player);
    RightupLegCapsule->SetCollisionMask(CollisionLayer::Layer_Enemy | CollisionLayer::Layer_EnemyWeapon);
    HEIN::TwoBoneLinkComponent* RightupLegLink = playerActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    RightupLegLink->Initialize(spawnData.tpsModel, L"mixamorig:RightUpLeg", L"RightLeg");
    RightupLegLink->LinkTo(RightupLegCapsule);
    HEIN::CapsuleColliderComponent* RightLegCapsule = playerActor->AddComponent<HEIN::CapsuleColliderComponent>();
    RightLegCapsule->Initialize(0.7f, 0.0f);
    RightLegCapsule->SetCollisionLayer(CollisionLayer::Layer_Player);
    RightLegCapsule->SetCollisionMask(CollisionLayer::Layer_Enemy | CollisionLayer::Layer_EnemyWeapon);
    HEIN::TwoBoneLinkComponent* RightLegLink = playerActor->AddComponent<HEIN::TwoBoneLinkComponent>();
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
    HEIN::CapsuleColliderComponent* LeftupLegCapsule = playerActor->AddComponent<HEIN::CapsuleColliderComponent>();
    LeftupLegCapsule->Initialize(1.0f, 0.0f);
    LeftupLegCapsule->SetCollisionLayer(CollisionLayer::Layer_Player);
    LeftupLegCapsule->SetCollisionMask(CollisionLayer::Layer_Enemy | CollisionLayer::Layer_EnemyWeapon);
    HEIN::TwoBoneLinkComponent* LeftupLegLink = playerActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    LeftupLegLink->Initialize(spawnData.tpsModel, L"mixamorig:LeftUpLeg", L"mixamorig:LeftLeg");
    LeftupLegLink->LinkTo(LeftupLegCapsule);
    HEIN::CapsuleColliderComponent* LeftLegCapsule = playerActor->AddComponent<HEIN::CapsuleColliderComponent>();
    LeftLegCapsule->Initialize(0.7f, 0.0f);
    LeftLegCapsule->SetCollisionLayer(CollisionLayer::Layer_Player);
    LeftLegCapsule->SetCollisionMask(CollisionLayer::Layer_Enemy | CollisionLayer::Layer_EnemyWeapon);
    HEIN::TwoBoneLinkComponent* LeftLegLink = playerActor->AddComponent < HEIN::TwoBoneLinkComponent>();
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
    HEIN::SocketComponent* socketComp = playerActor->AddComponent<HEIN::SocketComponent>();
    HEIN::Socket weaponSocket(
        L"WeaponSocket",
        L"mixamorig:RightHandThumb4",
        DirectX::SimpleMath::Vector3(-0.770f, -1.280f, -0.550f),
        DirectX::SimpleMath::Vector3(2.172f, 0.670f, 1.280f)
    );
    socketComp->AddSocket(weaponSocket);

    HEIN::RigidBodyComponent* rigidBody = playerActor->AddComponent<HEIN::RigidBodyComponent>();
    rigidBody->Initialize(80.0f, true, false);
    HEIN::CapsuleColliderComponent* rootPushbox = playerActor->AddComponent<HEIN::CapsuleColliderComponent>();
    rootPushbox->Initialize(3.0f, 12.0f); // Adjust height to match your Knight
    rootPushbox->SetOffset(DirectX::SimpleMath::Vector3(0.0f, 90.0f, 0.0f));
    rootPushbox->SetTrigger(false);      // This one physically hits the floor
    rootPushbox->SetColliderTag(L"PlayerRoot");
    rootPushbox->SetCollisionLayer(CollisionLayer::Layer_Player);
    rootPushbox->SetCollisionMask(CollisionLayer::Layer_Environment | CollisionLayer::Layer_Enemy);
  

    // SET BONES TO TRIGGERS 
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
    playerActor->AddComponent<HEIN::CombatStateMachineComponent>();
    playerActor->AddComponent<HEIN::CombatBlackBoard>();
    playerActor->AddComponent<HEIN::PlayerInputComponent>(cameraController); // Requires the camera controller
    playerActor->AddComponent<HEIN::CharacterMovementComponent>();
   

    playerActor->Start();
    return spawnData;
}

HEIN::ActorID HEIN::ActorFactory::CreateSword(
    ActorManager& actorManager,
    GameContext& gameContext, 
    HEIN::ActorID wielderID
)
{
    HEIN::Actor* sword = actorManager.CreateActor(L"Sword");
    sword->SetOwnerID(wielderID);

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
        CollisionLayer::Layer_EnemyWeapon
    );
   

    HEIN::SocketAttachmentComponent* socketAttachment = sword->AddComponent<HEIN::SocketAttachmentComponent>(&actorManager);
    socketAttachment->Initialize(wielderID, L"WeaponSocket");

    sword->Start();
    return sword->GetID();
}

HEIN::ActorID HEIN::ActorFactory::CreateStage(ActorManager& actorManager, GameContext& gameContext)
{
    // STAGE ROOT
    HEIN::Actor* stageRoot = actorManager.CreateActor(L"StageRoot");
    HEIN::TransformComponent* rootTran = stageRoot->AddComponent<HEIN::TransformComponent>();
    rootTran->SetPosition(DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f));
    rootTran->SetScale(DirectX::SimpleMath::Vector3(10.0f));

    // FLOOR CHILD 
    HEIN::Actor* floorActor = actorManager.CreateActor(L"StageFloor");
    HEIN::TransformComponent* floorTran = floorActor->AddComponent<HEIN::TransformComponent>();

    HEIN::StaticModelComponent* floorModel = floorActor->AddComponent<HEIN::StaticModelComponent>();
    floorModel->Initialize(gameContext, L"Resources/Models/stage/tile1.sdkmesh", L"Resources/Models/stage");

    HEIN::AABBColliderComponent* floorCol = floorActor->AddComponent<HEIN::AABBColliderComponent>();
    floorCol->InitializeFromModel(floorModel);
    floorCol->SetExtents(DirectX::SimpleMath::Vector3(10.4f, 0.06f, 10.4f));
    floorCol->SetCollisionLayer(CollisionLayer::Layer_Environment);

    // Link Floor to Root
    floorActor->SetParent(stageRoot->GetID());
    stageRoot->AddChild(floorActor->GetID());

    // WALL CHILD 
    HEIN::Actor* wall1Actor = actorManager.CreateActor(L"StageWall1");
    HEIN::TransformComponent* wall1Tran = wall1Actor->AddComponent<HEIN::TransformComponent>();
    wall1Tran->SetPosition(DirectX::SimpleMath::Vector3(10.0f, 1.0f, 0.0f));

    HEIN::AABBColliderComponent* wall1Col = wall1Actor->AddComponent<HEIN::AABBColliderComponent>();
    wall1Col->Initialize(DirectX::SimpleMath::Vector3(0.08f, 1.0f, 10.0f));
    wall1Col->SetCollisionLayer(CollisionLayer::Layer_Environment);

    // Link Wall to Root
    wall1Actor->SetParent(stageRoot->GetID());
    stageRoot->AddChild(wall1Actor->GetID());

    stageRoot->Start();
    floorActor->Start();
    wall1Actor->Start();

    return stageRoot->GetID();
}

HEIN::EnemySpawnData HEIN::ActorFactory::CreateEnemy(ActorManager& actorManager, GameContext& gameContext)
{
    HEIN::EnemySpawnData spawnData;

    Actor* enemyActor = actorManager.CreateActor(L"Enemy");

    spawnData.enemyID = enemyActor->GetID();

    HEIN::TransformComponent* ptransform = enemyActor->AddComponent<HEIN::TransformComponent>();
    ptransform->SetPosition(DirectX::SimpleMath::Vector3(25.0f, 4.0f, 0.0f));
    ptransform->SetScale(DirectX::SimpleMath::Vector3(0.10f));

    // ThirdPersonCamera model
    spawnData.tpsModel = enemyActor->AddComponent<HEIN::SkinnedModelComponent>();
    spawnData.tpsModel->Initialize(gameContext,
        L"Resources/Models/knight/knight.sdkmesh", // normal model
        L"Resources/Models/knight");
    spawnData.tpsModel->LoadAnimation("Idle", L"Resources/Models/knight/eidle.sdkmesh_anim");
    spawnData.tpsModel->LoadAnimation("Walk", L"Resources/Models/knight/running.sdkmesh_anim");
    spawnData.tpsModel->LoadAnimation("OneHand", L"Resources/Models/knight/swing.sdkmesh_anim");

    
    // Head Collider
    HEIN::CapsuleColliderComponent* HeadCapsule = enemyActor->AddComponent<HEIN::CapsuleColliderComponent>();
    HeadCapsule->Initialize(1.5f, 1.0f);
    HeadCapsule->SetCollisionLayer(CollisionLayer::Layer_Enemy);
    HEIN::BoneLinkComponent* HeadLink = enemyActor->AddComponent<HEIN::BoneLinkComponent>();
    HeadLink->Initialize(spawnData.tpsModel, L"mixamorig:Head");
    HeadLink->LinkTo(HeadCapsule);

    // Body Collider
    HEIN::CapsuleColliderComponent* BodyCapsule = enemyActor->AddComponent<HEIN::CapsuleColliderComponent>();
    BodyCapsule->Initialize(2.0f, 0.0f);
    BodyCapsule->SetCollisionLayer(CollisionLayer::Layer_Enemy);
    HEIN::TwoBoneLinkComponent* BodyLink = enemyActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    BodyLink->Initialize(spawnData.tpsModel, L"mixamorig:Spine2", L"mixamorig:Hips");
    BodyLink->LinkTo(BodyCapsule);

    // Right Arm Collider
    HEIN::CapsuleColliderComponent* RightarmCapsule = enemyActor->AddComponent<HEIN::CapsuleColliderComponent>();
    RightarmCapsule->Initialize(1.0f, 1.0f);
    RightarmCapsule->SetCollisionLayer(CollisionLayer::Layer_Enemy);
    HEIN::TwoBoneLinkComponent* RightarmLink = enemyActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    RightarmLink->Initialize(spawnData.tpsModel, L"mixamorig:RightArm", L"mixamorig:RightForeArm");
    RightarmLink->LinkTo(RightarmCapsule);
    HEIN::CapsuleColliderComponent* RightforearmCapsule = enemyActor->AddComponent<HEIN::CapsuleColliderComponent>();
    RightforearmCapsule->Initialize(0.6f, 1.0f);
    RightforearmCapsule->SetCollisionLayer(CollisionLayer::Layer_Enemy);
    HEIN::TwoBoneLinkComponent* RightforearmLink = enemyActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    RightforearmLink->Initialize(spawnData.tpsModel, L"mixamorig:RightForeArm", L"mixamorig:RightHand");
    RightforearmLink->LinkTo(RightforearmCapsule);


    // Left Arm Collider
    HEIN::CapsuleColliderComponent* LeftarmCapsule = enemyActor->AddComponent<HEIN::CapsuleColliderComponent>();
    LeftarmCapsule->Initialize(1.0f, 1.0f);
    LeftarmCapsule->SetCollisionLayer(CollisionLayer::Layer_Enemy);
    HEIN::TwoBoneLinkComponent* LeftarmLink = enemyActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    LeftarmLink->Initialize(spawnData.tpsModel, L"mixamorig:LeftArm", L"mixamorig:LeftForeArm");
    LeftarmLink->LinkTo(LeftarmCapsule);
    HEIN::CapsuleColliderComponent* LeftforearmCapsule = enemyActor->AddComponent<HEIN::CapsuleColliderComponent>();
    LeftforearmCapsule->Initialize(0.6f, 1.0f);
    LeftforearmCapsule->SetCollisionLayer(CollisionLayer::Layer_Enemy);
    HEIN::TwoBoneLinkComponent* LeftforearmLink = enemyActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    LeftforearmLink->Initialize(spawnData.tpsModel, L"mixamorig:LeftForeArm", L"mixamorig:LeftHand");
    LeftforearmLink->LinkTo(LeftforearmCapsule);

    // Right Leg Collider
    HEIN::CapsuleColliderComponent* RightupLegCapsule = enemyActor->AddComponent<HEIN::CapsuleColliderComponent>();
    RightupLegCapsule->Initialize(1.0f, 0.0f);
    RightupLegCapsule->SetCollisionLayer(CollisionLayer::Layer_Enemy);
    HEIN::TwoBoneLinkComponent* RightupLegLink = enemyActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    RightupLegLink->Initialize(spawnData.tpsModel, L"mixamorig:RightUpLeg", L"RightLeg");
    RightupLegLink->LinkTo(RightupLegCapsule);
    HEIN::CapsuleColliderComponent* RightLegCapsule = enemyActor->AddComponent<HEIN::CapsuleColliderComponent>();
    RightLegCapsule->Initialize(0.7f, 0.0f);
    RightLegCapsule->SetCollisionLayer(CollisionLayer::Layer_Enemy);
    HEIN::TwoBoneLinkComponent* RightLegLink = enemyActor->AddComponent<HEIN::TwoBoneLinkComponent>();
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
    HEIN::CapsuleColliderComponent* LeftupLegCapsule = enemyActor->AddComponent<HEIN::CapsuleColliderComponent>();
    LeftupLegCapsule->Initialize(1.0f, 0.0f);
    LeftupLegCapsule->SetCollisionLayer(CollisionLayer::Layer_Enemy);
    HEIN::TwoBoneLinkComponent* LeftupLegLink = enemyActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    LeftupLegLink->Initialize(spawnData.tpsModel, L"mixamorig:LeftUpLeg", L"mixamorig:LeftLeg");
    LeftupLegLink->LinkTo(LeftupLegCapsule);
    HEIN::CapsuleColliderComponent* LeftLegCapsule = enemyActor->AddComponent<HEIN::CapsuleColliderComponent>();
    LeftLegCapsule->Initialize(0.7f, 0.0f);
    LeftLegCapsule->SetCollisionLayer(CollisionLayer::Layer_Enemy);
    HEIN::TwoBoneLinkComponent* LeftLegLink = enemyActor->AddComponent < HEIN::TwoBoneLinkComponent>();
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
    HEIN::SocketComponent* socketComp = enemyActor->AddComponent<HEIN::SocketComponent>();
    HEIN::Socket weaponSocket(
        L"WeaponSocket",
        L"mixamorig:RightHandThumb4",
        DirectX::SimpleMath::Vector3(-0.770f, -1.280f, -0.550f),
        DirectX::SimpleMath::Vector3(2.172f, 0.670f, 1.280f)
    );
    socketComp->AddSocket(weaponSocket);

    HEIN::RigidBodyComponent* rigidBody = enemyActor->AddComponent<HEIN::RigidBodyComponent>();
    rigidBody->Initialize(80.0f, true, false);
    HEIN::CapsuleColliderComponent* rootPushbox = enemyActor->AddComponent<HEIN::CapsuleColliderComponent>();
    rootPushbox->Initialize(3.0f, 12.0f); // Adjust height to match your Knight
    rootPushbox->SetOffset(DirectX::SimpleMath::Vector3(0.0f, 90.0f, 0.0f));
    rootPushbox->SetTrigger(false);      // This one physically hits the floor
    rootPushbox->SetColliderTag(L"EnemyRoot");
    rootPushbox->SetCollisionLayer(CollisionLayer::Layer_Enemy);
    rootPushbox->SetCollisionMask(CollisionLayer::Layer_Environment | CollisionLayer::Layer_Player);
    

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



    enemyActor->Start();
    return spawnData;
}
