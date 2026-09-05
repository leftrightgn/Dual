#include "pch.h"
#include <BlackBoard/CombatBlackBoard.h>
#include <Components/PlayerInputComponent.h>
#include <Components/CharacterMovementComponent.h>
#include <Components/CombatStateMachineComponent.h>
#include "BehaviourTree/BTChaseNode.h"
#include "BehaviourTree/BTCheckDistance.h"


#include "BehaviourTree/BTDodgeNode.h"
#include <Components/BehaviourTreeComponent.h>
#include "BehaviourTree/BTIdleNode.h"
#include <States/CombatStates.h>
#include <BehaviourTree/BTAttackNode.h>
#include <utility>
#include "../../External/Engine/Components/HealthComponent.h"
#include "../../External/Engine/Components/ColliderComponent/CapsuleColliderComponent.h"
#include "../../External/Engine/Components/BoneLinkComponent.h"
#include "../../External/Engine/Components/TwoBoneLinkComponent.h"
#include "../../External/Engine/Components/SocketComponent.h"
#include "../../External/Engine/Components/RigidBodyComponent.h"
#include "../../External/Engine/Components/TargetTrackingComponent.h"
#include "../../External/Engine/Components/DamageDealerComponent.h"
#include "../../External/Engine/Components/ColliderComponent/OBBColliderComponent.h"
#include "../../External/Engine/Components/SocketAttachmentComponent.h"
#include "../../External/Engine/Components/StaticModelComponent.h"
#include "../../External/Engine/Components/ColliderComponent/MeshColliderComponent.h"
#include "../../External/Engine/Components/ColliderComponent/AABBColliderComponent.h"
#include "../../External/Engine/BehaviourTree/BTSelector.h"
#include "../../External/Engine/BehaviourTree/BTSequence.h"
#include "../../External/Engine/Camera/CameraController.h"
#include "ActorFactory.h"
#include "../../External/Engine/Components/TransformComponent.h"
#include <BehaviourTree/BTReturnToSpawnNode.h>
#include <BehaviourTree/BTCheckTetherNode.h>


HEIN::PlayerSpawnData HEIN::ActorFactory::CreateKnight(
    ActorManager& actorManager,
    GameContext& gameContext, 
    DirectX::SimpleMath::Vector3* /*targetCameraOut*/
)
{
    HEIN::PlayerSpawnData spawnData;

    HEIN::Actor* playerActor = actorManager.CreateActor(L"Player");

    spawnData.playerID = playerActor->GetID();
    playerActor->SetActorType(HEIN::ActorType::Player);
    HEIN::HealthComponent* playerHealth = playerActor->AddComponent<HEIN::HealthComponent>();
    playerHealth->Initialize(100);
    HEIN::TransformComponent* ptransform = playerActor->AddComponent<HEIN::TransformComponent>();
    ptransform->SetPosition(DirectX::SimpleMath::Vector3(0.0f, 4.0f, -40.0f));
    ptransform->SetScale(DirectX::SimpleMath::Vector3(0.10f));

    // ThirdPersonCamera model
    spawnData.tpsModel = playerActor->AddComponent<HEIN::SkinnedModelComponent>();
    spawnData.tpsModel->Initialize(gameContext,
        L"Resources/Models/knight/knight.sdkmesh", // normal model
        L"Resources/Models/knight");
    spawnData.tpsModel->LoadAnimation("Idle", L"Resources/Models/knight/idle.sdkmesh_anim");
    spawnData.tpsModel->LoadAnimation("Walk", L"Resources/Models/knight/running.sdkmesh_anim");
    spawnData.tpsModel->LoadAnimation("OneHand", L"Resources/Models/knight/swing.sdkmesh_anim");
    spawnData.tpsModel->LoadAnimation("Dodge", L"Resources/Models/knight/Dodge.sdkmesh_anim");
    spawnData.tpsModel->LoadAnimation("StrafeL", L"Resources/Models/knight/strafeL.sdkmesh_anim");
    spawnData.tpsModel->LoadAnimation("StrafeR", L"Resources/Models/knight/strafeR.sdkmesh_anim");
    spawnData.tpsModel->LoadAnimation("Block", L"Resources/Models/knight/block.sdkmesh_anim");

    // FirstPersonCamera model
    //spawnData.fpsModel = playerActor->AddComponent<HEIN::SkinnedModelComponent>();
    //spawnData.fpsModel->Initialize(gameContext,
    //    L"Resources/Models/knight/knight.sdkmesh", // headless/arms model
    //    L"Resources/Models/knight");
    //spawnData.fpsModel->LoadAnimation("Idle", L"Resources/Models/knight/idle.sdkmesh_anim");
    //spawnData.fpsModel->LoadAnimation("Walk", L"Resources/Models/knight/running.sdkmesh_anim");
    //spawnData.fpsModel->LoadAnimation("OneHand", L"Resources/Models/knight/swing.sdkmesh_anim");


    // Head Collider
    HEIN::CapsuleColliderComponent* HeadCapsule = playerActor->AddComponent<HEIN::CapsuleColliderComponent>();
    HeadCapsule->Initialize(1.5f, 1.0f);
    HeadCapsule->SetCollisionLayer(CollisionLayer::Layer_Player);
    HeadCapsule->SetCollisionMask(CollisionLayer::Layer_Enemy | CollisionLayer::Layer_EnemyWeapon);
    HeadCapsule->SetColliderTag(L"HeadCollider");
    HEIN::BoneLinkComponent* HeadLink = playerActor->AddComponent<HEIN::BoneLinkComponent>();
    HeadLink->Initialize(spawnData.tpsModel, L"mixamorig:Head");
    HeadLink->LinkTo(HeadCapsule);

    // Body Collider
    HEIN::CapsuleColliderComponent* BodyCapsule = playerActor->AddComponent<HEIN::CapsuleColliderComponent>();
    BodyCapsule->Initialize(2.0f, 0.0f);
    BodyCapsule->SetCollisionLayer(CollisionLayer::Layer_Player);
    BodyCapsule->SetCollisionMask(CollisionLayer::Layer_Enemy | CollisionLayer::Layer_EnemyWeapon);
    BodyCapsule->SetColliderTag(L"BodyCollider");
    HEIN::TwoBoneLinkComponent* BodyLink = playerActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    BodyLink->Initialize(spawnData.tpsModel, L"mixamorig:Spine2", L"mixamorig:Hips");
    BodyLink->LinkTo(BodyCapsule);

    // Right Arm Collider
    HEIN::CapsuleColliderComponent* RightarmCapsule = playerActor->AddComponent<HEIN::CapsuleColliderComponent>();
    RightarmCapsule->Initialize(1.0f, 1.0f);
    RightarmCapsule->SetCollisionLayer(CollisionLayer::Layer_Player);
    RightarmCapsule->SetCollisionMask(CollisionLayer::Layer_Enemy | CollisionLayer::Layer_EnemyWeapon);
    RightarmCapsule->SetColliderTag(L"RarmCollider");
    HEIN::TwoBoneLinkComponent* RightarmLink = playerActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    RightarmLink->Initialize(spawnData.tpsModel, L"mixamorig:RightArm", L"mixamorig:RightForeArm");
    RightarmLink->LinkTo(RightarmCapsule);
    HEIN::CapsuleColliderComponent* RightforearmCapsule = playerActor->AddComponent<HEIN::CapsuleColliderComponent>();
    RightforearmCapsule->Initialize(0.6f, 1.0f);
    RightforearmCapsule->SetCollisionLayer(CollisionLayer::Layer_Player);
    RightforearmCapsule->SetCollisionMask(CollisionLayer::Layer_Enemy | CollisionLayer::Layer_EnemyWeapon);
    RightforearmCapsule->SetColliderTag(L"RforearmCollider");
    HEIN::TwoBoneLinkComponent* RightforearmLink = playerActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    RightforearmLink->Initialize(spawnData.tpsModel, L"mixamorig:RightForeArm", L"mixamorig:RightHand");
    RightforearmLink->LinkTo(RightforearmCapsule);


    // Left Arm Collider
    HEIN::CapsuleColliderComponent* LeftarmCapsule = playerActor->AddComponent<HEIN::CapsuleColliderComponent>();
    LeftarmCapsule->Initialize(1.0f, 1.0f);
    LeftarmCapsule->SetCollisionLayer(CollisionLayer::Layer_Player);
    LeftarmCapsule->SetCollisionMask(CollisionLayer::Layer_Enemy | CollisionLayer::Layer_EnemyWeapon);
    LeftarmCapsule->SetColliderTag(L"LarmCollider");
    HEIN::TwoBoneLinkComponent* LeftarmLink = playerActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    LeftarmLink->Initialize(spawnData.tpsModel, L"mixamorig:LeftArm", L"mixamorig:LeftForeArm");
    LeftarmLink->LinkTo(LeftarmCapsule);
    HEIN::CapsuleColliderComponent* LeftforearmCapsule = playerActor->AddComponent<HEIN::CapsuleColliderComponent>();
    LeftforearmCapsule->Initialize(0.6f, 1.0f);
    LeftforearmCapsule->SetCollisionLayer(CollisionLayer::Layer_Player);
    LeftforearmCapsule->SetCollisionMask(CollisionLayer::Layer_Enemy | CollisionLayer::Layer_EnemyWeapon);
    LeftforearmCapsule->SetColliderTag(L"LforearmCollider");
    HEIN::TwoBoneLinkComponent* LeftforearmLink = playerActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    LeftforearmLink->Initialize(spawnData.tpsModel, L"mixamorig:LeftForeArm", L"mixamorig:LeftHand");
    LeftforearmLink->LinkTo(LeftforearmCapsule);

    // Right Leg Collider
    HEIN::CapsuleColliderComponent* RightupLegCapsule = playerActor->AddComponent<HEIN::CapsuleColliderComponent>();
    RightupLegCapsule->Initialize(1.0f, 0.0f);
    RightupLegCapsule->SetCollisionLayer(CollisionLayer::Layer_Player);
    RightupLegCapsule->SetCollisionMask(CollisionLayer::Layer_Enemy | CollisionLayer::Layer_EnemyWeapon);
    RightupLegCapsule->SetColliderTag(L"RupLegCollider");
    HEIN::TwoBoneLinkComponent* RightupLegLink = playerActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    RightupLegLink->Initialize(spawnData.tpsModel, L"mixamorig:RightUpLeg", L"mixamorig:RightLeg");
    RightupLegLink->LinkTo(RightupLegCapsule);
    HEIN::CapsuleColliderComponent* RightLegCapsule = playerActor->AddComponent<HEIN::CapsuleColliderComponent>();
    RightLegCapsule->Initialize(0.7f, 0.0f);
    RightLegCapsule->SetCollisionLayer(CollisionLayer::Layer_Player);
    RightLegCapsule->SetCollisionMask(CollisionLayer::Layer_Enemy | CollisionLayer::Layer_EnemyWeapon);
    RightLegCapsule->SetColliderTag(L"RLegCollider");
    HEIN::TwoBoneLinkComponent* RightLegLink = playerActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    RightLegLink->Initialize(spawnData.tpsModel, L"mixamorig:RightLeg", L"mixamorig:RightFoot");
    RightLegLink->LinkTo(RightLegCapsule);
   
    // Left Leg Collider
    HEIN::CapsuleColliderComponent* LeftupLegCapsule = playerActor->AddComponent<HEIN::CapsuleColliderComponent>();
    LeftupLegCapsule->Initialize(1.0f, 0.0f);
    LeftupLegCapsule->SetCollisionLayer(CollisionLayer::Layer_Player);
    LeftupLegCapsule->SetCollisionMask(CollisionLayer::Layer_Enemy | CollisionLayer::Layer_EnemyWeapon);
    LeftupLegCapsule->SetColliderTag(L"LupLegCollider");
    HEIN::TwoBoneLinkComponent* LeftupLegLink = playerActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    LeftupLegLink->Initialize(spawnData.tpsModel, L"mixamorig:LeftUpLeg", L"mixamorig:LeftLeg");
    LeftupLegLink->LinkTo(LeftupLegCapsule);
    HEIN::CapsuleColliderComponent* LeftLegCapsule = playerActor->AddComponent<HEIN::CapsuleColliderComponent>();
    LeftLegCapsule->Initialize(0.7f, 0.0f);
    LeftLegCapsule->SetCollisionLayer(CollisionLayer::Layer_Player);
    LeftLegCapsule->SetCollisionMask(CollisionLayer::Layer_Enemy | CollisionLayer::Layer_EnemyWeapon);
    LeftLegCapsule->SetColliderTag(L"LLegCollider");
    HEIN::TwoBoneLinkComponent* LeftLegLink = playerActor->AddComponent < HEIN::TwoBoneLinkComponent>();
    LeftLegLink->Initialize(spawnData.tpsModel, L"mixamorig:LeftLeg", L"mixamorig:LeftFoot");
    LeftLegLink->LinkTo(LeftLegCapsule);
    
    // Socket
    HEIN::SocketComponent* socketComp = playerActor->AddComponent<HEIN::SocketComponent>();
    HEIN::Socket weaponSocket(
        L"WeaponSocket",
        L"mixamorig:RightHand",
        DirectX::SimpleMath::Vector3(-1.73f, 0.38f, -0.19f),
        DirectX::SimpleMath::Vector3(3.0f, 1.60f, 2.0f)
    );
    socketComp->AddSocket(weaponSocket);

    HEIN::RigidBodyComponent* rigidBody = playerActor->AddComponent<HEIN::RigidBodyComponent>();
    rigidBody->Initialize(80.0f, true, false);
    HEIN::CapsuleColliderComponent* rootPushbox = playerActor->AddComponent<HEIN::CapsuleColliderComponent>();
    rootPushbox->Initialize(3.0f, 12.0f); // Adjust height to match knight model proportions
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

   
    HEIN::CombatStateMachineComponent* fsm = playerActor->AddComponent<HEIN::CombatStateMachineComponent>();

    // IdleConfig
    HEIN::StateConfig idleConfig;
    idleConfig.animationName = "Idle";
    idleConfig.transitions["OnMove"] = "Walk";
    idleConfig.transitions["OnAttack"] = "OneHand";
    idleConfig.transitions["OnDodge"] = "Dodge";
    idleConfig.transitions["OnStrafe"] = "Strafe";
    idleConfig.transitions["OnBlock"] = "Block";
    fsm->AddState("Idle", std::make_unique<HEIN::IdleState>(idleConfig));

    // WalkConfig
    HEIN::StateConfig walkConfig;
    walkConfig.moveSpeed = 30.0f;
    walkConfig.animationName = "Walk";
    walkConfig.transitions["OnStop"] = "Idle";
    walkConfig.transitions["OnAttack"] = "OneHand";
    walkConfig.transitions["OnDodge"] = "Dodge";
    walkConfig.transitions["OnStrafe"] = "Strafe";
    walkConfig.transitions["OnBlock"] = "Block";
    fsm->AddState("Walk", std::make_unique<HEIN::WalkState>(walkConfig));

    // AttackConfig
    HEIN::StateConfig attackConfig;
    attackConfig.moveSpeed = 5.0f;
    attackConfig.animationName = "OneHand";
    attackConfig.stateDuration = 4.1f;
    attackConfig.comboEndTimes = { HEIN::OneHandAttackState::STAGE_1_END_TIME, HEIN::OneHandAttackState::STAGE_2_END_TIME, HEIN::OneHandAttackState::STAGE_3_END_TIME };
    attackConfig.comboWindowStarts = { HEIN::OneHandAttackState::STAGE_1_WINDOW_START, HEIN::OneHandAttackState::STAGE_2_WINDOW_START, HEIN::OneHandAttackState::STAGE_3_WINDOW_START };
    attackConfig.transitions["OnStop"] = "Idle";
    attackConfig.transitions["OnMove"] = "Walk";
    attackConfig.transitions["OnDodge"] = "Dodge";
    attackConfig.transitions["OnStrafe"] = "Strafe";
    attackConfig.transitions["OnBlock"] = "Block";
    fsm->AddState("OneHand", std::make_unique<HEIN::OneHandAttackState>(attackConfig));

    // DodgeConfig
    HEIN::StateConfig dodgeConfig;
    dodgeConfig.animationName = "Dodge";
    dodgeConfig.moveSpeed = 30.0f;
    dodgeConfig.stateDuration = 1.4f;
    dodgeConfig.transitions["OnStop"] = "Idle";
    dodgeConfig.transitions["OnMove"] = "Walk";
    dodgeConfig.transitions["OnAttack"] = "OneHand";
    dodgeConfig.transitions["OnStrafe"] = "Strafe";
    fsm->AddState("Dodge", std::make_unique<HEIN::DodgeState>(dodgeConfig));

    // StrafeConfig
    HEIN::StateConfig strafeConfig;
    strafeConfig.animationName = "StrafeR";
    strafeConfig.secondaryAnimationName = "StrafeL";
    strafeConfig.moveSpeed = 5.0f;
    strafeConfig.transitions["OnStop"] = "Idle";
    strafeConfig.transitions["OnMove"] = "Walk";
    strafeConfig.transitions["OnDodge"] = "Dodge";
    fsm->AddState("Strafe", std::make_unique<HEIN::StrafeState>(strafeConfig));
    
    // BlockConfig
    HEIN::StateConfig blockConfig;
    blockConfig.animationName = "Block";
    blockConfig.moveSpeed = 4.0f;
    blockConfig.transitions["OnMove"] = "Walk";
    blockConfig.transitions["OnStop"] = "Idle";
    fsm->AddState("Block", std::make_unique<HEIN::BlockState>(blockConfig));

    playerActor->AddComponent<HEIN::CombatBlackBoard>();
    playerActor->AddComponent<HEIN::PlayerInputComponent>(&actorManager); 
    playerActor->AddComponent<HEIN::TargetTrackingComponent>(&actorManager, HEIN::ActorType::Enemy);
    playerActor->AddComponent<HEIN::CharacterMovementComponent>();

    playerActor->Start();
    return spawnData;
}

HEIN::ActorID HEIN::ActorFactory::CreateSword(
    ActorManager& actorManager,
    GameContext& gameContext, 
    HEIN::ActorID wielderID,
    float damage
)
{
    HEIN::Actor* sword = actorManager.CreateActor(L"Sword");
    sword->SetOwnerID(wielderID);

    HEIN::DamageDealerComponent* swordDamage = sword->AddComponent<HEIN::DamageDealerComponent>();
    swordDamage->Initialize(damage, DamageType::Physical);

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

    uint32_t weaponLayer = CollisionLayer::Layer_PlayerWeapon;
    uint32_t weaponMask = CollisionLayer::Layer_Enemy | CollisionLayer::Layer_EnemyWeapon;

    HEIN::Actor* wielder = actorManager.GetActor(wielderID);

    if (wielder != nullptr)
    {
        if (wielder->GetActorType() == HEIN::ActorType::Enemy)
        {
            weaponLayer = CollisionLayer::Layer_EnemyWeapon;
            weaponMask = CollisionLayer::Layer_Player | CollisionLayer::Layer_PlayerWeapon;
        }
    }

    swordHitBox->SetCollisionLayer(weaponLayer);
    swordHitBox->SetCollisionMask(weaponMask);
   

    HEIN::SocketAttachmentComponent* socketAttachment = sword->AddComponent<HEIN::SocketAttachmentComponent>(&actorManager);
    socketAttachment->Initialize(wielderID, L"WeaponSocket");
    actorManager.SetParent(sword->GetID(), wielderID, false);

    sword->Start();
    return sword->GetID();
}

HEIN::ActorID HEIN::ActorFactory::CreateAxe(
    ActorManager& actorManager,
    GameContext& gameContext,
    HEIN::ActorID wielderID, 
    float damage
)
{
    HEIN::Actor* axe = actorManager.CreateActor(L"Axe");
    axe->SetOwnerID(wielderID);

    HEIN::DamageDealerComponent* axeDamage = axe->AddComponent<HEIN::DamageDealerComponent>();
    axeDamage->Initialize(damage, DamageType::Physical);

    HEIN::TransformComponent* axeTransform = axe->AddComponent<HEIN::TransformComponent>();
    axeTransform->SetScale(DirectX::SimpleMath::Vector3(10.0f));

    HEIN::StaticModelComponent* axeModel = axe->AddComponent<HEIN::StaticModelComponent>();
    axeModel->Initialize(
        gameContext,
        L"Resources/Models/boss/axe.sdkmesh",
        L"Resources/Models/boss"
    );
    HEIN::OBBColliderComponent* axeHitBox = axe->AddComponent<HEIN::OBBColliderComponent>();

    axeHitBox->Initialize(DirectX::SimpleMath::Vector3(0.2f, 0.3f, 0.05f));
    axeHitBox->SetOffset(DirectX::SimpleMath::Vector3(-0.4f, -0.9f, -0.05f));
    axeHitBox->SetRotationOffset(
        DirectX::SimpleMath::Vector3(
            DirectX::XMConvertToRadians(1.0f),
            DirectX::XMConvertToRadians(1.0f),
            DirectX::XMConvertToRadians(-12.0f)
        )
    );
    axeHitBox->SetTrigger(true);

    HEIN::CapsuleColliderComponent* axeCapsule = axe->AddComponent<HEIN::CapsuleColliderComponent>();
    axeCapsule->Initialize(0.5f, 8.0f);
    axeCapsule->SetOffset(DirectX::SimpleMath::Vector3(-0.07f, -0.22f, -0.03f));
    axeCapsule->SetRotationOffset(
        DirectX::SimpleMath::Vector3(
            DirectX::XMConvertToRadians(1.0f),
            DirectX::XMConvertToRadians(-2.0f),
            DirectX::XMConvertToRadians(-16.0f)
        )
    );
    axeCapsule->SetTrigger(true);

    uint32_t weaponLayer = CollisionLayer::Layer_PlayerWeapon;
    uint32_t weaponMask = CollisionLayer::Layer_Enemy | CollisionLayer::Layer_EnemyWeapon;

    HEIN::Actor* wielder = actorManager.GetActor(wielderID);

    if (wielder != nullptr)
    {
        if (wielder->GetActorType() == HEIN::ActorType::Enemy)
        {
            weaponLayer = CollisionLayer::Layer_EnemyWeapon;
            weaponMask = CollisionLayer::Layer_Player | CollisionLayer::Layer_PlayerWeapon;
        }
    }

    axeHitBox->SetCollisionLayer(weaponLayer);
    axeCapsule->SetCollisionLayer(weaponLayer);
    axeHitBox->SetCollisionMask(weaponMask);
    axeCapsule->SetCollisionMask(weaponMask);


    HEIN::SocketAttachmentComponent* socketAttachment = axe->AddComponent<HEIN::SocketAttachmentComponent>(&actorManager);
    socketAttachment->Initialize(wielderID, L"WeaponSocket");
    actorManager.SetParent(axe->GetID(), wielderID, false);

    axe->Start();
    return axe->GetID();
}

HEIN::ActorID HEIN::ActorFactory::CreateStage(ActorManager& actorManager, GameContext& gameContext)
{
    // STAGE ROOT
    HEIN::Actor* stageRoot = actorManager.CreateActor(L"StageRoot");
    HEIN::TransformComponent* rootTran = stageRoot->AddComponent<HEIN::TransformComponent>();
    rootTran->SetPosition(DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f));
    rootTran->SetScale(DirectX::SimpleMath::Vector3(10.0f));

    // FLOOR CHILD 
    HEIN::Actor* floorActor = actorManager.CreateActor(L"Floor");
            floorActor->AddComponent<HEIN::TransformComponent>();

    HEIN::StaticModelComponent* floorModel = floorActor->AddComponent<HEIN::StaticModelComponent>();
    floorModel->Initialize(gameContext, L"Resources/Models/stage/floor1.sdkmesh", L"Resources/Models/stage");
    HEIN::MeshColliderComponent* floorPhysics = floorActor->AddComponent<HEIN::MeshColliderComponent>();
    floorPhysics->LoadFromObj(L"Resources/Models/stage/floor1.obj");
    floorPhysics->SetCollisionLayer(CollisionLayer::Layer_Environment);
    // Link Floor to Root
    floorActor->SetParent(stageRoot->GetID());
    stageRoot->AddChild(floorActor->GetID());

    // WALL CHILD 
    DirectX::SimpleMath::Vector3 wallPosition[] =
    {
        DirectX::SimpleMath::Vector3(9.0f, 1.0f, 0.0f),
        DirectX::SimpleMath::Vector3(0.0f, 1.0f, 9.0f),
        DirectX::SimpleMath::Vector3(-9.0f, 1.0f, 0.0f),
        DirectX::SimpleMath::Vector3(0.0f, 1.0f, -9.0f),
    };
    DirectX::SimpleMath::Vector3 wallColPos[] =
    {
        DirectX::SimpleMath::Vector3(0.08f, 1.0f, 10.0f),
        DirectX::SimpleMath::Vector3(10.0f, 1.0f, 0.08f),
        DirectX::SimpleMath::Vector3(0.08f, 1.0f, 10.4f),
        DirectX::SimpleMath::Vector3(10.0f, 1.0f, 0.08f)
    };
    for (int i = 0; i < 4; ++i)
    {
        std::wstring wallName = L"StageWall" + std::to_wstring(i);
        HEIN::Actor* wallActor = actorManager.CreateActor(wallName);
        HEIN::TransformComponent* wallTrans = wallActor->AddComponent<HEIN::TransformComponent>();
        wallTrans->SetPosition(wallPosition[i]);
        HEIN::AABBColliderComponent* wallCol = wallActor->AddComponent<HEIN::AABBColliderComponent>();
        wallCol->Initialize(wallColPos[i]);
        wallCol->SetCollisionLayer(CollisionLayer::Layer_Environment);
        wallActor->SetParent(stageRoot->GetID());
        stageRoot->AddChild(wallActor->GetID());

        wallActor->Start();
    }
    

    // PILLAR
    DirectX::SimpleMath::Vector3 pillarPosition[] =
    {
        DirectX::SimpleMath::Vector3(10.0f, 0.0f, 10.0f),
        DirectX::SimpleMath::Vector3(10.0f, 0.0f, -10.0f),
        DirectX::SimpleMath::Vector3(-10.0f, 0.0f, 10.0f),
        DirectX::SimpleMath::Vector3(-10.0f, 0.0f, -10.0f)
    };
    
    for (int i = 0; i < 4; ++i)
    {
        std::wstring pillarName = L"Pillar" + std::to_wstring(i);
        HEIN::Actor* pillarActor = actorManager.CreateActor(pillarName);
        HEIN::TransformComponent* pillarTrans = pillarActor->AddComponent<HEIN::TransformComponent>();
        pillarTrans->SetPosition(pillarPosition[i]);
        HEIN::StaticModelComponent* pillarModel = pillarActor->AddComponent<HEIN::StaticModelComponent>();
        pillarModel->Initialize(gameContext, L"Resources/Models/stage/pillar.sdkmesh", L"Resources/Models/stage");

        pillarActor->SetParent(stageRoot->GetID());
        stageRoot->AddChild(pillarActor->GetID());

        pillarActor->Start();
    }
    // Bridge
    DirectX::SimpleMath::Vector3 bridgePosition[] =
    {
        DirectX::SimpleMath::Vector3(10.0f, 0.0f, 10.0f),
        DirectX::SimpleMath::Vector3(-10.0f, 0.0f, 10.0f),
        DirectX::SimpleMath::Vector3(10.0f, 0.0f, -10.0f),
        DirectX::SimpleMath::Vector3(-10.0f, 0.0f, -10.0f)
    };
    DirectX::SimpleMath::Vector3 bridgeRotation[] =
    {
        DirectX::SimpleMath::Vector3(0.0f, DirectX::XMConvertToRadians(90.0f), 0.0f),
        DirectX::SimpleMath::Vector3(0.0f, DirectX::XMConvertToRadians(0.0f), 0.0f),
        DirectX::SimpleMath::Vector3(0.0f, DirectX::XMConvertToRadians(90.0f), 0.0f),
        DirectX::SimpleMath::Vector3(0.0f, DirectX::XMConvertToRadians(0.0f), 0.0f)
    };

    for (int i = 0; i < 4; ++i)
    {
        std::wstring bridgeName = L"Bridge" + std::to_wstring(i);
        HEIN::Actor* bridgeActor = actorManager.CreateActor(bridgeName);
        HEIN::TransformComponent* bridgeTrans = bridgeActor->AddComponent<HEIN::TransformComponent>();
        bridgeTrans->SetPosition(bridgePosition[i]);
        bridgeTrans->SetRotationEuler(bridgeRotation[i]);
        HEIN::StaticModelComponent* bridModel = bridgeActor->AddComponent<HEIN::StaticModelComponent>();
        bridModel->Initialize(gameContext, L"Resources/Models/stage/bridge.sdkmesh", L"Resources/Models/stage");
        bridgeActor->SetParent(stageRoot->GetID());
        stageRoot->AddChild(bridgeActor->GetID());

        bridgeActor->Start();
    }
   

    stageRoot->Start();
    floorActor->Start();

    return stageRoot->GetID();
}

HEIN::EnemySpawnData HEIN::ActorFactory::CreateEnemy(
    ActorManager& actorManager, 
    GameContext& gameContext,
    HEIN::ActorID targetID
)
{
    HEIN::EnemySpawnData spawnData;

    Actor* enemyActor = actorManager.CreateActor(L"Enemy");

    spawnData.enemyID = enemyActor->GetID();
    enemyActor->SetActorType(HEIN::ActorType::Enemy);
    HEIN::HealthComponent* enemyHealth = enemyActor->AddComponent<HEIN::HealthComponent>();
    enemyHealth->Initialize(100);

    HEIN::TransformComponent* ptransform = enemyActor->AddComponent<HEIN::TransformComponent>();
    ptransform->SetPosition(DirectX::SimpleMath::Vector3(0.0f, 4.0f, 0.0f));
    ptransform->SetScale(DirectX::SimpleMath::Vector3(0.15f));

    // ThirdPersonCamera model
    spawnData.tpsModel = enemyActor->AddComponent<HEIN::SkinnedModelComponent>();
    spawnData.tpsModel->Initialize(gameContext,
        L"Resources/Models/Boss/Boss.sdkmesh", // normal model
        L"Resources/Models/Boss");
    spawnData.tpsModel->LoadAnimation("Idle", L"Resources/Models/Boss/idle.sdkmesh_anim");
    spawnData.tpsModel->LoadAnimation("Walk", L"Resources/Models/Boss/running.sdkmesh_anim");
    spawnData.tpsModel->LoadAnimation("OneHand", L"Resources/Models/Boss/swing2.sdkmesh_anim");
    spawnData.tpsModel->LoadAnimation("StrafeL", L"Resources/Models/Boss/strafeL.sdkmesh_anim");
    spawnData.tpsModel->LoadAnimation("StrafeR", L"Resources/Models/Boss/strafeR.sdkmesh_anim");
    spawnData.tpsModel->LoadAnimation("Dodge", L"Resources/Models/Boss/Dodge.sdkmesh_anim");

    
    // Head Collider
    HEIN::CapsuleColliderComponent* HeadCapsule = enemyActor->AddComponent<HEIN::CapsuleColliderComponent>();
    HeadCapsule->Initialize(3.5f, 1.0f);
    HeadCapsule->SetCollisionLayer(CollisionLayer::Layer_Enemy);
    HEIN::BoneLinkComponent* HeadLink = enemyActor->AddComponent<HEIN::BoneLinkComponent>();
    HeadLink->Initialize(spawnData.tpsModel, L"mixamorig:Head");
    HeadLink->LinkTo(HeadCapsule);

    // Body Collider
    HEIN::CapsuleColliderComponent* BodyCapsule = enemyActor->AddComponent<HEIN::CapsuleColliderComponent>();
    BodyCapsule->Initialize(6.5f, 0.0f);
    BodyCapsule->SetCollisionLayer(CollisionLayer::Layer_Enemy);
    HEIN::TwoBoneLinkComponent* BodyLink = enemyActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    BodyLink->Initialize(spawnData.tpsModel, L"mixamorig:Spine2", L"mixamorig:Hips");
    BodyLink->LinkTo(BodyCapsule);

    // Right Arm Collider
    HEIN::CapsuleColliderComponent* RightarmCapsule = enemyActor->AddComponent<HEIN::CapsuleColliderComponent>();
    RightarmCapsule->Initialize(3.0f, 0.0f);
    RightarmCapsule->SetCollisionLayer(CollisionLayer::Layer_Enemy);
    HEIN::TwoBoneLinkComponent* RightarmLink = enemyActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    RightarmLink->Initialize(spawnData.tpsModel, L"mixamorig:RightArm", L"mixamorig:RightForeArm");
    RightarmLink->LinkTo(RightarmCapsule);
    HEIN::CapsuleColliderComponent* RightforearmCapsule = enemyActor->AddComponent<HEIN::CapsuleColliderComponent>();
    RightforearmCapsule->Initialize(2.5f, 0.0f);
    RightforearmCapsule->SetCollisionLayer(CollisionLayer::Layer_Enemy);
    HEIN::TwoBoneLinkComponent* RightforearmLink = enemyActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    RightforearmLink->Initialize(spawnData.tpsModel, L"mixamorig:RightForeArm", L"mixamorig:RightHand");
    RightforearmLink->LinkTo(RightforearmCapsule);


    // Left Arm Collider
    HEIN::CapsuleColliderComponent* LeftarmCapsule = enemyActor->AddComponent<HEIN::CapsuleColliderComponent>();
    LeftarmCapsule->Initialize(3.0f, 0.0f);
    LeftarmCapsule->SetCollisionLayer(CollisionLayer::Layer_Enemy);
    HEIN::TwoBoneLinkComponent* LeftarmLink = enemyActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    LeftarmLink->Initialize(spawnData.tpsModel, L"mixamorig:LeftArm", L"mixamorig:LeftForeArm");
    LeftarmLink->LinkTo(LeftarmCapsule);
    HEIN::CapsuleColliderComponent* LeftforearmCapsule = enemyActor->AddComponent<HEIN::CapsuleColliderComponent>();
    LeftforearmCapsule->Initialize(2.5f, 0.0f);
    LeftforearmCapsule->SetCollisionLayer(CollisionLayer::Layer_Enemy);
    HEIN::TwoBoneLinkComponent* LeftforearmLink = enemyActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    LeftforearmLink->Initialize(spawnData.tpsModel, L"mixamorig:LeftForeArm", L"mixamorig:LeftHand");
    LeftforearmLink->LinkTo(LeftforearmCapsule);

    // Right Leg Collider
    HEIN::CapsuleColliderComponent* RightupLegCapsule = enemyActor->AddComponent<HEIN::CapsuleColliderComponent>();
    RightupLegCapsule->Initialize(2.8f, 0.0f);
    RightupLegCapsule->SetCollisionLayer(CollisionLayer::Layer_Enemy);
    HEIN::TwoBoneLinkComponent* RightupLegLink = enemyActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    RightupLegLink->Initialize(spawnData.tpsModel, L"mixamorig:RightUpLeg", L"mixamorig:RightLeg");
    RightupLegLink->LinkTo(RightupLegCapsule);
    HEIN::CapsuleColliderComponent* RightLegCapsule = enemyActor->AddComponent<HEIN::CapsuleColliderComponent>();
    RightLegCapsule->Initialize(1.8f, 0.0f);
    RightLegCapsule->SetCollisionLayer(CollisionLayer::Layer_Enemy);
    HEIN::TwoBoneLinkComponent* RightLegLink = enemyActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    RightLegLink->Initialize(spawnData.tpsModel, L"mixamorig:RightLeg", L"mixamorig:RightFoot");
    RightLegLink->LinkTo(RightLegCapsule);
  
    // Left Leg Collider
    HEIN::CapsuleColliderComponent* LeftupLegCapsule = enemyActor->AddComponent<HEIN::CapsuleColliderComponent>();
    LeftupLegCapsule->Initialize(2.3f, 0.0f);
    LeftupLegCapsule->SetCollisionLayer(CollisionLayer::Layer_Enemy);
    HEIN::TwoBoneLinkComponent* LeftupLegLink = enemyActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    LeftupLegLink->Initialize(spawnData.tpsModel, L"mixamorig:LeftUpLeg", L"mixamorig:LeftLeg");
    LeftupLegLink->LinkTo(LeftupLegCapsule);
    HEIN::CapsuleColliderComponent* LeftLegCapsule = enemyActor->AddComponent<HEIN::CapsuleColliderComponent>();
    LeftLegCapsule->Initialize(1.8f, 0.0f);
    LeftLegCapsule->SetCollisionLayer(CollisionLayer::Layer_Enemy);
    HEIN::TwoBoneLinkComponent* LeftLegLink = enemyActor->AddComponent < HEIN::TwoBoneLinkComponent>();
    LeftLegLink->Initialize(spawnData.tpsModel, L"mixamorig:LeftLeg", L"mixamorig:LeftFoot");
    LeftLegLink->LinkTo(LeftLegCapsule);
   


    // Socket
    HEIN::SocketComponent* socketComp = enemyActor->AddComponent<HEIN::SocketComponent>();
    HEIN::Socket weaponSocket(
        L"WeaponSocket",
        L"mixamorig:RightHand",
        DirectX::SimpleMath::Vector3(1.0f, 3.0f, -1.0f),
        DirectX::SimpleMath::Vector3(3.0f, 0.0f, 1.5f)
    );
    socketComp->AddSocket(weaponSocket);

    HEIN::RigidBodyComponent* rigidBody = enemyActor->AddComponent<HEIN::RigidBodyComponent>();
    rigidBody->Initialize(80.0f, true, false);
    HEIN::CapsuleColliderComponent* rootPushbox = enemyActor->AddComponent<HEIN::CapsuleColliderComponent>();
    rootPushbox->Initialize(5.0f, 17.0f); // Adjust height to match knight model proportions
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

    HEIN::CombatBlackBoard* bb = enemyActor->AddComponent<HEIN::CombatBlackBoard>();
    bb->spawnPosition = ptransform->GetPosition();
    bb->hasSetSpawnPosition = true;
    
    enemyActor->AddComponent<HEIN::CharacterMovementComponent>();
    enemyActor->AddComponent<HEIN::TargetTrackingComponent>(&actorManager, HEIN::ActorType::Player);
  
    std::unique_ptr<HEIN::BTSelector> aiBrain = std::make_unique<HEIN::BTSelector>();

    // ---------------------------------------------------------
    // 1. LEASHING SEQUENCE (Highest Priority - Checked First!)
    // ---------------------------------------------------------
    std::unique_ptr<HEIN::BTSequence> leashSequence = std::make_unique<HEIN::BTSequence>();
    // If enemy wanders > 40m from spawn (OR is currently returning), this succeeds
    leashSequence->AddChild(std::make_unique<HEIN::BTCheckTetherNode>(40.0f));
    // Execute the walk back home
    leashSequence->AddChild(std::make_unique<HEIN::BTReturnToSpawnNode>(20.0f));
    aiBrain->AddChild(std::move(leashSequence));


    // ---------------------------------------------------------
    // 2. COMBAT SEQUENCE (The Aggro Zone)
    // ---------------------------------------------------------
    std::unique_ptr<HEIN::BTSequence> combatSequence = std::make_unique<HEIN::BTSequence>();

    // The player must be within 20 meters of the enemy to start the fight!
    combatSequence->AddChild(std::make_unique<HEIN::BTCheckDistance>(0.0f, 100.0f));

    // If the player is in range, decide how to fight them:
    std::unique_ptr<HEIN::BTSelector> combatSelector = std::make_unique<HEIN::BTSelector>();

    // -- Dodge
    std::unique_ptr<HEIN::BTSequence> dodgeSequence = std::make_unique<HEIN::BTSequence>();
    dodgeSequence->AddChild(std::make_unique<HEIN::BTCheckDistance>(0.0f, 15.0f));
    dodgeSequence->AddChild(std::make_unique<HEIN::BTDodgeNode>(1.4f));
    combatSelector->AddChild(std::move(dodgeSequence));

    // -- Attack
    std::unique_ptr<HEIN::BTSequence> attackSequence = std::make_unique<HEIN::BTSequence>();
    attackSequence->AddChild(std::make_unique<HEIN::BTCheckDistance>(15.0f, 25.0f));
    attackSequence->AddChild(std::make_unique<HEIN::BTAttackNode>(3.4f, 25.0f));
    combatSelector->AddChild(std::move(attackSequence));

    // -- Chase
    combatSelector->AddChild(std::make_unique<HEIN::BTChaseNode>(25.0f, 80.0f));

    combatSequence->AddChild(std::move(combatSelector));
    aiBrain->AddChild(std::move(combatSequence));


    // ---------------------------------------------------------
    // 3. IDLE NODE (Fallback)
    // ---------------------------------------------------------
    // If we are NOT leashing, AND the player is > 20m away... stand perfectly still!
    aiBrain->AddChild(std::make_unique<HEIN::BTIdleNode>());

    HEIN::BehaviourTreeComponent* btComp = enemyActor->AddComponent<HEIN::BehaviourTreeComponent>();
    btComp->Initialize(std::move(aiBrain), &actorManager, targetID);


    HEIN::CombatStateMachineComponent* fsm = enemyActor->AddComponent<HEIN::CombatStateMachineComponent>();

    // IdleConfig
    HEIN::StateConfig idleConfig;
    idleConfig.animationName = "Idle";
    idleConfig.transitions["OnMove"] = "Walk";
    idleConfig.transitions["OnAttack"] = "OneHand";
    idleConfig.transitions["OnStrafe"] = "Strafe";
    idleConfig.transitions["OnDodge"] = "Dodge";
    fsm->AddState("Idle", std::make_unique<HEIN::IdleState>(idleConfig));

    // WalkConfig
    HEIN::StateConfig walkConfig;
    walkConfig.moveSpeed = 30.0f;
    walkConfig.animationName = "Walk";
    walkConfig.transitions["OnStop"] = "Idle";
    walkConfig.transitions["OnAttack"] = "OneHand";
    walkConfig.transitions["OnStrafe"] = "Strafe";
    walkConfig.transitions["OnDodge"] = "Dodge";
    fsm->AddState("Walk", std::make_unique<HEIN::WalkState>(walkConfig));

    // StrafeConfig
    HEIN::StateConfig strafeConfig;
    strafeConfig.animationName = "StrafeR";
    strafeConfig.secondaryAnimationName = "StrafeL";
    strafeConfig.moveSpeed = 5.0f;
    strafeConfig.transitions["OnStop"] = "Idle";
    strafeConfig.transitions["OnMove"] = "Walk";
    strafeConfig.transitions["OnAttack"] = "OneHand";
    strafeConfig.transitions["OnDodge"] = "Dodge";
    fsm->AddState("Strafe", std::make_unique<HEIN::StrafeState>(strafeConfig));

    // AttackConfig
    HEIN::StateConfig attackConfig;
    attackConfig.moveSpeed = 5.0f;
    attackConfig.animationName = "OneHand";
    attackConfig.stateDuration = 3.4f;
    attackConfig.comboEndTimes = { 1.6f, 3.0f, 3.4f };
    attackConfig.comboWindowStarts = { 1.2f, 2.7f, 3.0f };
    attackConfig.transitions["OnStop"] = "Idle";
    attackConfig.transitions["OnMove"] = "Walk";
    attackConfig.transitions["OnDodge"] = "Dodge";
    attackConfig.transitions["OnStrafe"] = "Strafe";
    fsm->AddState("OneHand", std::make_unique<HEIN::OneHandAttackState>(attackConfig));

    // DodgeConfig
    HEIN::StateConfig dodgeConfig;
    dodgeConfig.animationName = "Dodge";
    dodgeConfig.moveSpeed = 30.0f;
    dodgeConfig.stateDuration = 1.4f;
    dodgeConfig.transitions["OnStop"] = "Idle";
    dodgeConfig.transitions["OnMove"] = "Walk";
    dodgeConfig.transitions["OnAttack"] = "OneHand";
    dodgeConfig.transitions["OnStrafe"] = "Strafe";
    fsm->AddState("Dodge", std::make_unique<HEIN::DodgeState>(dodgeConfig));

    enemyActor->Start();
    return spawnData;
}

HEIN::ActorID HEIN::ActorFactory::CreateMainCamera(ActorManager& actorManager)
{
    HEIN::Actor* cameraActor = actorManager.CreateActor(L"MainCamera");

    cameraActor->AddComponent<HEIN::CameraController>();

    cameraActor->Start();
    return cameraActor->GetID();
}
