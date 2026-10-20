#include "pch.h"
#include <BlackBoard/CombatBlackBoard.h>
#include <Components/PlayerInputComponent.h>
#include <Components/CharacterMovementComponent.h>
#include <Components/CombatStateMachineComponent.h>
#include <Components/ShockwaveEmitterComponent.h>
#include <Components/PossessableComponent.h>
#include "BehaviourTree/BTChaseNode.h"
#include "BehaviourTree/BTCheckDistance.h"
#include "BehaviourTree/BTDodgeNode.h"
#include <Components/BehaviourTreeComponent.h>
#include "BehaviourTree/BTIdleNode.h"
#include "BehaviourTree/BTAttackNode.h"
#include "BehaviourTree/BTCheckTargetStance.h"
#include "BehaviourTree/BTCooldownDecorator.h"
#include "BehaviourTree/BTCheckHealth.h"
#include <utility>
#include "../../External/Engine/Components/HealthComponent.h"
#include "../../External/Engine/Components/GaugeComponent.h"
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
#include "../../External/Engine/Components/WeaponTrailComponent.h"
#include "../../External/Engine/Components/ColliderComponent/MeshColliderComponent.h"
#include "../../External/Engine/Components/ColliderComponent/AABBColliderComponent.h"
#include "../../External/Engine/BehaviourTree/BTSelector.h"
#include "../../External/Engine/BehaviourTree/BTSequence.h"
#include "../../External/Engine/Camera/CameraController.h"
#include "../../External/Engine/Factory/ComponentFactory.h"
#include "ActorFactory.h"
#include "../../External/Engine/Common/SkeletonProfile.h"
#include "../../External/Engine/Components/TransformComponent.h"
#include "../../External/Engine/Components/ProceduralAnimationComponent.h"
#include <BehaviourTree/BTReturnToSpawnNode.h>
#include <BehaviourTree/BTCheckTetherNode.h>
#include "../../External/Engine/Common/DamageSystem.h"
#include <BehaviourTree/BTStrafeNode.h>
#include "../../External/Engine/Message/Messenger.h"
#include "../../External/Engine/Message/Message.h"

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

    // Single Hybrid Skinned Model (Blend of Root Motion and In-Place animations)
    spawnData.tpsModel = playerActor->AddComponent<HEIN::SkinnedModelComponent>();
    spawnData.tpsModel->Initialize(gameContext,
        L"Resources/Models/knight/knight.sdkmesh",
        L"Resources/Models/knight");

    // Load knight animations
    spawnData.tpsModel->LoadAnimation("Idle", L"Resources/Models/knight/idle.sdkmesh_anim");
    spawnData.tpsModel->LoadAnimation("Walk", L"Resources/Models/knight/RunRoot.sdkmesh_anim");
    spawnData.tpsModel->LoadAnimation("Dodge", L"Resources/Models/knight/Dodge.sdkmesh_anim");
    spawnData.tpsModel->LoadAnimation("StrafeL", L"Resources/Models/knight/strafeL.sdkmesh_anim");
    spawnData.tpsModel->LoadAnimation("StrafeR", L"Resources/Models/knight/strafeR.sdkmesh_anim");
    spawnData.tpsModel->LoadAnimation("Block", L"Resources/Models/knight/block.sdkmesh_anim");
    spawnData.tpsModel->LoadAnimation("Jump", L"Resources/Models/knight/Jump.sdkmesh_anim");
    spawnData.tpsModel->LoadAnimation("Slash1", L"Resources/Models/knight/slash1.sdkmesh_anim");
    spawnData.tpsModel->LoadAnimation("Slash2", L"Resources/Models/knight/slash2.sdkmesh_anim");
    spawnData.tpsModel->LoadAnimation("Slash3", L"Resources/Models/knight/slash3.sdkmesh_anim");
    spawnData.tpsModel->LoadAnimation("Stun", L"Resources/Models/knight/stun.sdkmesh_anim");
    spawnData.tpsModel->SetEnableRootMotion(true); // Root Motion enabled for hybrid walk & attacks
    spawnData.tpsModel->SetVisible(true);

    // Socket
    HEIN::SocketComponent* socketComp = playerActor->AddComponent<HEIN::SocketComponent>();
    HEIN::Socket weaponSocket(
        L"WeaponSocket",
        L"mixamorig:RightHand",
        DirectX::SimpleMath::Vector3(-1.73f, 0.38f, -0.19f),
        DirectX::SimpleMath::Vector3(3.0f, 1.60f, 2.0f)
    );
    socketComp->AddSocket(weaponSocket);

    // Data-driven modular combat state machine (combos, windows, and timings loaded via Knight_Combat.json)
    auto* playerCombat = playerActor->AddComponent<HEIN::CombatStateMachineComponent>();
    playerCombat->LoadFromConfigFile("Resources/Data/Combat/Knight_Combat.json");

    playerActor->AddComponent<HEIN::PlayerBlackBoard>();
    playerActor->AddComponent<HEIN::PlayerInputComponent>(&actorManager);
    playerActor->AddComponent<HEIN::TargetTrackingComponent>(&actorManager, HEIN::ActorType::Enemy);
    playerActor->AddComponent<HEIN::CharacterMovementComponent>();
    playerActor->AddComponent<HEIN::ProceduralAnimationComponent>(&actorManager);
    playerActor->AddComponent<HEIN::ShockwaveEmitterComponent>(&actorManager);

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
    swordDamage->Initialize(damage, DamageType::Physical, false, GameConstants::Weapons::DefaultSwordKnockback);

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

    HEIN::WeaponTrailComponent* swordTrail = sword->AddComponent<HEIN::WeaponTrailComponent>(&actorManager);
    swordTrail->ApplyTarnishedGoldPreset();
    swordTrail->AutoFitOffsetsFromWeapon();

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
    axeDamage->Initialize(damage, DamageType::Physical, true, 30.0f);

    HEIN::TransformComponent* axeTransform = axe->AddComponent<HEIN::TransformComponent>();
    axeTransform->SetScale(DirectX::SimpleMath::Vector3(10.0f));

    HEIN::StaticModelComponent* axeModel = axe->AddComponent<HEIN::StaticModelComponent>();
    axeModel->Initialize(
        gameContext,
        L"Resources/Models/boss/axe.sdkmesh",
        L"Resources/Models/boss"
    );
    HEIN::OBBColliderComponent* axeHitBox = axe->AddComponent<HEIN::OBBColliderComponent>();

    axeHitBox->Initialize(DirectX::SimpleMath::Vector3(0.2f, 0.3f, 0.25f));
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
    axeCapsule->Initialize(1.5f, 8.0f);
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

    HEIN::WeaponTrailComponent* axeTrail = axe->AddComponent<HEIN::WeaponTrailComponent>(&actorManager);
    axeTrail->ApplyBossBloodflamePreset();
    axeTrail->AutoFitOffsetsFromWeapon();

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
    floorModel->m_castShadows = false;
    HEIN::MeshColliderComponent* floorPhysics = floorActor->AddComponent<HEIN::MeshColliderComponent>();
    floorPhysics->LoadFromObj(L"Resources/Models/stage/floor1.obj");
    floorPhysics->SetCollisionLayer(CollisionLayer::Layer_Environment | CollisionLayer::Layer_Ground);
    floorPhysics->SetColliderTag(L"Ground");
    // Link Floor to Root
    floorActor->SetParent(stageRoot->GetID());
    stageRoot->AddChild(floorActor->GetID());

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
    spawnData.tpsModel->LoadAnimation("OneHand", L"Resources/Models/Boss/Standing Melee Attack Horizontal.sdkmesh_anim");
    spawnData.tpsModel->LoadAnimation("Combo1", L"Resources/Models/Boss/Standing Melee Attack Horizontal.sdkmesh_anim");
    spawnData.tpsModel->LoadAnimation("Combo2", L"Resources/Models/Boss/Standing Melee Attack Downward.sdkmesh_anim");
    spawnData.tpsModel->LoadAnimation("Combo3", L"Resources/Models/Boss/Standing Melee Attack Backhand.sdkmesh_anim");
    spawnData.tpsModel->LoadAnimation("HeavyAttack", L"Resources/Models/Boss/Standing Melee Run Jump Attack.sdkmesh_anim");
    spawnData.tpsModel->LoadAnimation("Dodge", L"Resources/Models/Boss/dodge.sdkmesh_anim");
    spawnData.tpsModel->SetEnableRootMotion(true); // Root Motion enabled for jump & attack animations
    spawnData.tpsModel->SetVisible(true);

    // Head Collider
    HEIN::CapsuleColliderComponent* HeadCapsule = enemyActor->AddComponent<HEIN::CapsuleColliderComponent>();
    HeadCapsule->Initialize(3.5f, 1.0f);
    HeadCapsule->SetColliderTag(L"HeadCollider");
    HeadCapsule->SetCollisionLayer(CollisionLayer::Layer_Enemy);
    HEIN::BoneLinkComponent* HeadLink = enemyActor->AddComponent<HEIN::BoneLinkComponent>();
    HeadLink->Initialize(spawnData.tpsModel, L"mixamorig:Head");
    HeadLink->LinkTo(HeadCapsule);

    // Body Collider
    HEIN::CapsuleColliderComponent* BodyCapsule = enemyActor->AddComponent<HEIN::CapsuleColliderComponent>();
    BodyCapsule->Initialize(6.5f, 0.0f);
    BodyCapsule->SetColliderTag(L"BodyCollider");
    BodyCapsule->SetCollisionLayer(CollisionLayer::Layer_Enemy);
    HEIN::TwoBoneLinkComponent* BodyLink = enemyActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    BodyLink->Initialize(spawnData.tpsModel, L"mixamorig:Spine2", L"mixamorig:Hips");
    BodyLink->LinkTo(BodyCapsule);

    // Right Arm Collider
    HEIN::CapsuleColliderComponent* RightarmCapsule = enemyActor->AddComponent<HEIN::CapsuleColliderComponent>();
    RightarmCapsule->Initialize(3.0f, 0.0f);
    RightarmCapsule->SetColliderTag(L"RArmCollider");
    RightarmCapsule->SetCollisionLayer(CollisionLayer::Layer_Enemy);
    HEIN::TwoBoneLinkComponent* RightarmLink = enemyActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    RightarmLink->Initialize(spawnData.tpsModel, L"mixamorig:RightArm", L"mixamorig:RightForeArm");
    RightarmLink->LinkTo(RightarmCapsule);
    HEIN::CapsuleColliderComponent* RightforearmCapsule = enemyActor->AddComponent<HEIN::CapsuleColliderComponent>();
    RightforearmCapsule->Initialize(2.5f, 0.0f);
    RightforearmCapsule->SetColliderTag(L"RForearmCollider");
    RightforearmCapsule->SetCollisionLayer(CollisionLayer::Layer_Enemy);
    HEIN::TwoBoneLinkComponent* RightforearmLink = enemyActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    RightforearmLink->Initialize(spawnData.tpsModel, L"mixamorig:RightForeArm", L"mixamorig:RightHand");
    RightforearmLink->LinkTo(RightforearmCapsule);

    // Left Arm Collider
    HEIN::CapsuleColliderComponent* LeftarmCapsule = enemyActor->AddComponent<HEIN::CapsuleColliderComponent>();
    LeftarmCapsule->Initialize(3.0f, 0.0f);
    LeftarmCapsule->SetColliderTag(L"LArmCollider");
    LeftarmCapsule->SetCollisionLayer(CollisionLayer::Layer_Enemy);
    HEIN::TwoBoneLinkComponent* LeftarmLink = enemyActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    LeftarmLink->Initialize(spawnData.tpsModel, L"mixamorig:LeftArm", L"mixamorig:LeftForeArm");
    LeftarmLink->LinkTo(LeftarmCapsule);
    HEIN::CapsuleColliderComponent* LeftforearmCapsule = enemyActor->AddComponent<HEIN::CapsuleColliderComponent>();
    LeftforearmCapsule->Initialize(2.5f, 0.0f);
    LeftforearmCapsule->SetColliderTag(L"LForearmCollider");
    LeftforearmCapsule->SetCollisionLayer(CollisionLayer::Layer_Enemy);
    HEIN::TwoBoneLinkComponent* LeftforearmLink = enemyActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    LeftforearmLink->Initialize(spawnData.tpsModel, L"mixamorig:LeftForeArm", L"mixamorig:LeftHand");
    LeftforearmLink->LinkTo(LeftforearmCapsule);

    // Right Leg Collider
    HEIN::CapsuleColliderComponent* RightupLegCapsule = enemyActor->AddComponent<HEIN::CapsuleColliderComponent>();
    RightupLegCapsule->Initialize(2.8f, 0.0f);
    RightupLegCapsule->SetColliderTag(L"RUpLegCollider");
    RightupLegCapsule->SetCollisionLayer(CollisionLayer::Layer_Enemy);
    HEIN::TwoBoneLinkComponent* RightupLegLink = enemyActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    RightupLegLink->Initialize(spawnData.tpsModel, L"mixamorig:RightUpLeg", L"mixamorig:RightLeg");
    RightupLegLink->LinkTo(RightupLegCapsule);
    HEIN::CapsuleColliderComponent* RightLegCapsule = enemyActor->AddComponent<HEIN::CapsuleColliderComponent>();
    RightLegCapsule->Initialize(1.8f, 0.0f);
    RightLegCapsule->SetColliderTag(L"RLegCollider");
    RightLegCapsule->SetCollisionLayer(CollisionLayer::Layer_Enemy);
    HEIN::TwoBoneLinkComponent* RightLegLink = enemyActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    RightLegLink->Initialize(spawnData.tpsModel, L"mixamorig:RightLeg", L"mixamorig:RightFoot");
    RightLegLink->LinkTo(RightLegCapsule);

    // Left Leg Collider
    HEIN::CapsuleColliderComponent* LeftupLegCapsule = enemyActor->AddComponent<HEIN::CapsuleColliderComponent>();
    LeftupLegCapsule->Initialize(2.3f, 0.0f);
    LeftupLegCapsule->SetColliderTag(L"LUpLegCollider");
    LeftupLegCapsule->SetCollisionLayer(CollisionLayer::Layer_Enemy);
    HEIN::TwoBoneLinkComponent* LeftupLegLink = enemyActor->AddComponent<HEIN::TwoBoneLinkComponent>();
    LeftupLegLink->Initialize(spawnData.tpsModel, L"mixamorig:LeftUpLeg", L"mixamorig:LeftLeg");
    LeftupLegLink->LinkTo(LeftupLegCapsule);
    HEIN::CapsuleColliderComponent* LeftLegCapsule = enemyActor->AddComponent<HEIN::CapsuleColliderComponent>();
    LeftLegCapsule->Initialize(1.8f, 0.0f);
    LeftLegCapsule->SetColliderTag(L"LLegCollider");
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
    rootPushbox->SetCollisionMask(CollisionLayer::Layer_Environment | CollisionLayer::Layer_Ground | CollisionLayer::Layer_Wall | CollisionLayer::Layer_Player);

    // SET BONES TO TRIGGERS (So they don't push the floor)
    HeadCapsule->SetTrigger(true);
    BodyCapsule->SetTrigger(true);
    RightarmCapsule->SetTrigger(true);
    RightforearmCapsule->SetTrigger(true);
    LeftarmCapsule->SetTrigger(true);
    LeftforearmCapsule->SetTrigger(true);
    RightupLegCapsule->SetTrigger(true);
    RightLegCapsule->SetTrigger(true);
    LeftupLegCapsule->SetTrigger(true);
    LeftLegCapsule->SetTrigger(true);

    HEIN::BossBlackBoard* bb = enemyActor->AddComponent<HEIN::BossBlackBoard>();
    bb->spawnPosition = ptransform->GetPosition();
    bb->hasSetSpawnPosition = true;

    enemyActor->AddComponent<HEIN::CharacterMovementComponent>();
    enemyActor->AddComponent<HEIN::ShockwaveEmitterComponent>(&actorManager);

    SetupEnemyAI(enemyActor, &actorManager, targetID);

    // Data-driven modular combat state machine (combos, windows, and timings loaded via Boss_Combat.json)
    auto* enemyCombat = enemyActor->GetComponent<HEIN::CombatStateMachineComponent>();
    if (!enemyCombat)
    {
        enemyCombat = enemyActor->AddComponent<HEIN::CombatStateMachineComponent>();
    }
    enemyCombat->LoadFromConfigFile("Resources/Data/Combat/Boss_Combat.json");

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

void HEIN::ActorFactory::RegisterGameComponents()
{
    ComponentFactory::Initialize();
    ComponentFactory::RegisterComponent<CombatStateMachineComponent>("CombatStateMachineComponent");
    ComponentFactory::RegisterComponent<BehaviourTreeComponent>("BehaviourTreeComponent");
    ComponentFactory::RegisterComponent<CharacterMovementComponent>("CharacterMovementComponent");
    ComponentFactory::RegisterComponent<PlayerInputComponent>("PlayerInputComponent");
    ComponentFactory::RegisterComponent<CombatBlackBoard>("CombatBlackBoard");
    ComponentFactory::RegisterComponent<PlayerBlackBoard>("PlayerBlackBoard");
    ComponentFactory::RegisterComponent<BossBlackBoard>("BossBlackBoard");
    ComponentFactory::RegisterComponent<PossessableComponent>("PossessableComponent");
    ComponentFactory::RegisterComponent<ShockwaveEmitterComponent>("ShockwaveEmitterComponent");

    // UI pulls Stamina (and legacy Block / Dodge) from PlayerBlackBoard or CombatBlackBoard
    GaugeComponent::SetExternalSyncHandler([](Actor* target, GaugeType type, float& outCurrent, float& outMax) -> bool {
        if (!target) return false;
        auto* playerBB = target->GetComponent<PlayerBlackBoard>();
        auto* bb = target->GetComponent<CombatBlackBoard>();

        if (type == GaugeType::Stamina || type == GaugeType::Block || type == GaugeType::Dodge)
        {
            if (playerBB)
            {
                outCurrent = std::max(0.0f, playerBB->currentBlockStamina);
                outMax = playerBB->maxBlockStamina;
                return true;
            }
            else if (bb)
            {
                outCurrent = std::max(0.0f, bb->currentStamina);
                outMax = bb->maxStamina;
                return true;
            }
            return false;
        }
        return false;
    });

    // Intercept damage for blocking states, hit stun, and dynamic physical knockback
    DamageSystem::SetDamageInterceptor([](Actor* attacker, Actor* victim, DamageDealerComponent* dealer, float& damage) -> bool {
        if (!victim) return true;
        auto* sm = victim->GetComponent<CombatStateMachineComponent>();
        auto* bb = victim->GetComponent<CombatBlackBoard>();
        auto* playerBB = victim->GetComponent<PlayerBlackBoard>();

        bool isBlocked = false;
        bool isParried = false;
        bool applyDamage = true;
        bool skipHitStun = false;

        // Block cooldown active: this hit belongs to a swing already blocked.
        // Ignore it completely (no damage, no stamina cost, no knockback).
        if (playerBB && playerBB->blockCooldownTimer > 0.0f)
        {
            return false;
        }

        // Data-Driven Attacker State Evaluation (No string matching!)
        CombatStateMachineComponent* attackerSM = attacker ? attacker->GetComponent<CombatStateMachineComponent>() : nullptr;
        BossBlackBoard* attackerBossBB = attacker ? attacker->GetComponent<BossBlackBoard>() : nullptr;
        PlayerBlackBoard* attackerPlayerBB = attacker ? attacker->GetComponent<PlayerBlackBoard>() : nullptr;

        const StateConfig* attackerConfig = attackerSM ? attackerSM->GetCurrentStateConfig() : nullptr;
        CombatStateType attackerStateType = attackerConfig ? attackerConfig->stateTypeEnum : CombatStateType::Normal;

        bool isHeavyAttack = (attackerStateType == CombatStateType::Attack && attackerConfig && attackerConfig->staminaDamage >= GameConstants::Combat::HeavyAttackStaminaThreshold) ||
                             (attackerBossBB && attackerBossBB->activeNodeName.find("Heavy Attack") != std::string::npos);
        bool isComboFinisher = (attackerStateType == CombatStateType::ComboFinisher) ||
                               (attackerConfig && attackerConfig->comboPermutationStep >= 3);
        bool isGuardCounterAttack = (attackerSM && attackerSM->IsGuardCounter()) || (attackerStateType == CombatStateType::GuardCounter);

        float baseStaminaDamage = attackerConfig ? attackerConfig->staminaDamage : GameConstants::Combat::DefaultStaminaDamage;

        // Process Block / Parry (Elden Ring 4 Defensive Tiers)
        float baseForce = dealer ? dealer->GetKnockbackForce() : GameConstants::Combat::DefaultKnockbackForce;
        if (baseForce <= 0.0f) baseForce = GameConstants::Combat::DefaultKnockbackForce;
        float impulseMagnitude = baseForce;

        if (sm && sm->IsBlocking() && playerBB && !playerBB->isBlockBroken && playerBB->currentBlockStamina > 0.0f)
        {
            isBlocked = true;
            playerBB->blockCooldownTimer = playerBB->blockCooldownDuration;

            bool isTierA = (playerBB->blockActiveTimer <= playerBB->parryActiveWindow && playerBB->parryLockoutTimer <= 0.0f);
            bool isTierB = !isTierA && (
                (playerBB->blockActiveTimer > playerBB->parryActiveWindow && playerBB->blockActiveTimer <= playerBB->partialParryWindow) ||
                (playerBB->blockActiveTimer <= playerBB->partialParryWindow && playerBB->parryLockoutTimer > 0.0f)
            );

            if (isTierA)
            {
                // Tier A - Spontaneous Deflect / Perfect Parry
                applyDamage = false;
                isParried = true;
                impulseMagnitude = 0.0f; // Zero pushback

                // Restore stamina from data config
                playerBB->currentBlockStamina = std::fmin(playerBB->maxBlockStamina, playerBB->currentBlockStamina + playerBB->parryStaminaRestore);

                // Increment deflectStacks (up to maxDeflectStacks) and open guardCounterWindowTimer
                playerBB->deflectStacks = std::min(playerBB->maxDeflectStacks, playerBB->deflectStacks + 1);
                playerBB->guardCounterWindowTimer = playerBB->guardCounterWindowDuration;

                if (dealer)
                {
                    dealer->SetHitstopFrames(playerBB->parryHitstopFrames);
                }

                // Multi-Parry Boss Logic (Data-driven HP threshold and required parries)
                if (attackerBossBB)
                {
                    attackerBossBB->currentParryCount++;
                    attackerBossBB->parryDecayTimer = attackerBossBB->parryDecayDelay;

                    float maxHp = attackerBossBB->maxHealth;
                    float curHp = attackerBossBB->currentHealth;
                    if (auto* attackerHealth = attacker->GetComponent<HealthComponent>())
                    {
                        maxHp = attackerHealth->GetMaxHealth();
                        curHp = attackerHealth->GetCurrentHealth();
                    }
                    float hpPct = (maxHp > 0.0f) ? (curHp / maxHp) : 1.0f;
                    int requiredParries = (hpPct <= attackerBossBB->phase2HealthThresholdPct)
                        ? attackerBossBB->phase2ParriesRequired
                        : attackerBossBB->parriesRequiredForStagger;
                    if (attackerBossBB->currentParryCount >= requiredParries)
                    {
                        attackerBossBB->currentParryCount = 0;
                        Messenger::GetInstance()->Notify(attacker->GetID(), Message::HIT_STUN);
                    }
                    else
                    {
                        // If currentParryCount < requiredParries, Boss continues combo uninterrupted
                        if (auto* attBB = attacker->GetComponent<CombatBlackBoard>())
                        {
                            attBB->lastAttackHit = true;
                        }
                        Messenger::GetInstance()->Notify(attacker->GetID(), Message::ATTACK_HIT_CONFIRMED);
                    }
                }
                else if (attacker)
                {
                    Messenger::GetInstance()->Notify(attacker->GetID(), Message::HIT_STUN);
                }
            }
            else if (isTierB)
            {
                // Tier B - Partial Parry Fallback (Configurable ratios)
                float partialStaminaCost = baseStaminaDamage * playerBB->partialParryStaminaMultiplier;
                playerBB->currentBlockStamina -= partialStaminaCost;

                if (playerBB->currentBlockStamina <= 0.0f)
                {
                    // Tier D - Guard Break Fallback
                    playerBB->currentBlockStamina = 0.0f;
                    playerBB->isBlockBroken = true;
                    playerBB->guardCounterWindowTimer = 0.0f;
                    playerBB->deflectStacks = 0;
                    playerBB->staminaRegenDelayTimer = playerBB->guardBreakDelayTimer;

                    damage = damage * playerBB->guardBreakChipDamageRatio;
                    applyDamage = true;
                    skipHitStun = false;
                    isBlocked = false;

                    if (sm)
                    {
                        sm->ChangeState("Stun", GameConstants::Combat::StunStateTransitionDuration);
                    }
                    Messenger::GetInstance()->Notify(victim->GetID(), Message::HIT_STUN);
                    impulseMagnitude = baseForce * playerBB->guardBreakPushbackMultiplier;
                }
                else
                {
                    // Mitigate damage with configurable chip damage ratio
                    auto* health = victim->GetComponent<HealthComponent>();
                    if (health)
                    {
                        health->ApplyDamage(damage * playerBB->partialParryChipDamageRatio);
                    }
                    applyDamage = false;
                    skipHitStun = true;
                    impulseMagnitude = baseForce * playerBB->partialParryPushbackMultiplier;
                    playerBB->guardCounterWindowTimer = 0.0f;
                    playerBB->staminaRegenDelayTimer = playerBB->partialParryRegenDelay;

                    if (attacker)
                    {
                        if (auto* attBB = attacker->GetComponent<CombatBlackBoard>())
                        {
                            attBB->lastAttackHit = true;
                        }
                        Messenger::GetInstance()->Notify(attacker->GetID(), Message::ATTACK_HIT_CONFIRMED);
                    }
                }
            }
            else
            {
                // Tier C - Standard Guard
                float staminaLost = baseStaminaDamage * (1.0f - (playerBB->guardBoost / GameConstants::Combat::GuardPercentDivisor));
                playerBB->currentBlockStamina -= staminaLost;

                if (attacker)
                {
                    if (auto* attBB = attacker->GetComponent<CombatBlackBoard>())
                    {
                        attBB->lastAttackHit = true;
                    }
                    Messenger::GetInstance()->Notify(attacker->GetID(), Message::ATTACK_HIT_CONFIRMED);
                }

                if (playerBB->currentBlockStamina <= 0.0f)
                {
                    // Tier D - Guard Break
                    playerBB->currentBlockStamina = 0.0f;
                    playerBB->isBlockBroken = true;
                    playerBB->guardCounterWindowTimer = 0.0f;
                    playerBB->deflectStacks = 0;
                    playerBB->staminaRegenDelayTimer = playerBB->guardBreakDelayTimer;

                    damage = damage * playerBB->guardBreakChipDamageRatio;
                    applyDamage = true;
                    skipHitStun = false;
                    isBlocked = false;

                    if (sm)
                    {
                        sm->ChangeState("Stun", GameConstants::Combat::StunStateTransitionDuration);
                    }
                    Messenger::GetInstance()->Notify(victim->GetID(), Message::HIT_STUN);
                    impulseMagnitude = baseForce * playerBB->guardBreakPushbackMultiplier;
                }
                else
                {
                    applyDamage = false;
                    skipHitStun = true;
                    playerBB->staminaRegenDelayTimer = playerBB->blockRegenDelay;

                    if (isHeavyAttack)
                    {
                        // Heavy slam: heavy pushback, disables Guard Counter
                        impulseMagnitude = baseForce * playerBB->heavyBlockPushbackMultiplier;
                        playerBB->guardCounterWindowTimer = 0.0f;
                    }
                    else
                    {
                        // Normal combo swings: reduced block pushback, open Guard Counter window
                        impulseMagnitude = baseForce * playerBB->blockPushbackMultiplier;
                        playerBB->guardCounterWindowTimer = playerBB->guardCounterWindowDuration;
                    }
                }
            }

            if (bb) bb->consecutiveHitsTaken = 0;
        }

        // Calculate direction from attacker to victim along horizontal ground plane
        DirectX::SimpleMath::Vector3 dir = DirectX::SimpleMath::Vector3::Zero;
        auto* victimTrans = victim->GetComponent<TransformComponent>();
        auto* attackerTrans = attacker ? attacker->GetComponent<TransformComponent>() : nullptr;

        if (victimTrans && attackerTrans)
        {
            dir = victimTrans->GetPosition() - attackerTrans->GetPosition();
            dir.y = 0.0f;
        }

        if (dir.LengthSquared() > 0.0001f)
        {
            dir.Normalize();
        }
        else if (attackerTrans)
        {
            dir = attackerTrans->GetForward();
            dir.y = 0.0f;
            if (dir.LengthSquared() > 0.0001f)
                dir.Normalize();
            else
                dir = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 1.0f);
        }
        else
        {
            dir = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 1.0f);
        }

        // Anti-combo enemy burst: if enemy takes 3 consecutive unblocked hits, reset counter, push attacker away with -60.0f radial impulse, and skip hit-stun
        if (!isBlocked && bb)
        {
            bb->consecutiveHitsTaken++;
            if (bb->consecutiveHitsTaken >= 3)
            {
                bb->consecutiveHitsTaken = 0;
                skipHitStun = true;

                if (attacker)
                {
                    DirectX::SimpleMath::Vector3 burstImpulse = -dir * 60.0f;
                    if (auto* attMoveComp = attacker->GetComponent<CharacterMovementComponent>())
                    {
                        attMoveComp->ApplyImpulse(burstImpulse);
                    }
                    else if (auto* attRb = attacker->GetComponent<RigidBodyComponent>())
                    {
                        attRb->SetHorizontalVelocity(attRb->GetVelocity() + burstImpulse);
                    }
                }
            }
        }

        // Guard Counter Damage & Stance Bonus (When Player Hits Boss / Enemy)
        if (isGuardCounterAttack)
        {
            if (attackerPlayerBB)
            {
                impulseMagnitude *= (1.5f + 0.20f * attackerPlayerBB->deflectStacks);
                attackerPlayerBB->deflectStacks = 0;
            }
            if (dealer)
            {
                int gcHitstop = attackerPlayerBB ? attackerPlayerBB->guardCounterHitstopFrames : 9;
                dealer->SetHitstopFrames(gcHitstop);
            }
            Messenger::GetInstance()->Notify(victim->GetID(), Message::HIT_STUN);
            skipHitStun = false;
        }

        // Sync per-state Hitstop frames on dealer
        if (attackerSM && dealer && !isGuardCounterAttack)
        {
            int stateHitstop = attackerSM->GetCurrentHitstopFrames();
            if (isComboFinisher)
            {
                int finFreeze = attackerPlayerBB ? attackerPlayerBB->comboFinisherHitstopFrames : 8;
                stateHitstop = std::max(stateHitstop, finFreeze); // 3rd-hit combo finisher freeze across all 6 permutations
            }
            if (isParried)
            {
                int parryFreeze = playerBB ? playerBB->parryHitstopFrames : 8;
                stateHitstop = std::max(stateHitstop, parryFreeze); // Crisp parry freeze
            }
            dealer->SetHitstopFrames(stateHitstop);
        }

        if (!isBlocked && !skipHitStun)
        {
            if (isHeavyAttack || (dealer && dealer->CausesStun()))
            {
                Messenger::GetInstance()->Notify(victim->GetID(), Message::HIT_STUN);
            }
        }

        if (!isBlocked && !isParried && !isGuardCounterAttack)
        {
            if (isHeavyAttack)
            {
                impulseMagnitude *= 1.4f; // Heavy attack impact bonus
            }
        }

        DirectX::SimpleMath::Vector3 impulse = dir * impulseMagnitude;

        // Grounded physical knockback (purely horizontal along ground plane)

        if (isParried)
        {
            impulse = DirectX::SimpleMath::Vector3::Zero;
        }

        if (auto* moveComp = victim->GetComponent<CharacterMovementComponent>())
        {
            moveComp->ApplyImpulse(impulse);
        }
        else if (auto* rb = victim->GetComponent<RigidBodyComponent>())
        {
            rb->SetHorizontalVelocity(rb->GetVelocity() + impulse);
        }

        return applyDamage;
    });
}

void HEIN::ActorFactory::SetupEnemyAI(Actor* enemyActor, ActorManager* actorManager, HEIN::ActorID targetID)
{
    if (!enemyActor || !actorManager) return;

    if (!enemyActor->GetComponent<HEIN::TargetTrackingComponent>())
    {
        enemyActor->AddComponent<HEIN::TargetTrackingComponent>(actorManager, HEIN::ActorType::Player);
    }
    if (!enemyActor->GetComponent<HEIN::ProceduralAnimationComponent>())
    {
        enemyActor->AddComponent<HEIN::ProceduralAnimationComponent>(actorManager);
    }

    std::unique_ptr<HEIN::BTSelector> aiBrain = std::make_unique<HEIN::BTSelector>();

    // ---------------------------------------------------------
    // LEASHING SEQUENCE (Out of Bound Safety)
    // ---------------------------------------------------------
    std::unique_ptr<HEIN::BTSequence> leashSequence = std::make_unique<HEIN::BTSequence>();
    leashSequence->AddChild(std::make_unique<HEIN::BTCheckTetherNode>(60.0f));
    leashSequence->AddChild(std::make_unique<HEIN::BTReturnToSpawnNode>(25.0f));
    aiBrain->AddChild(std::move(leashSequence));

    // ---------------------------------------------------------
    // REACTIVE COUNTER-PLAY
    // ---------------------------------------------------------
    // If the player starts swinging in close quarters, the boss dodge (halved cooldown)
    std::unique_ptr<HEIN::BTSequence> reactiveDodge = std::make_unique<HEIN::BTSequence>();
    reactiveDodge->AddChild(std::make_unique<HEIN::BTCheckDistance>(0.0f, 20.0f));
    reactiveDodge->AddChild(std::make_unique<HEIN::BTCheckTargetStance>(true));  // is targetAttacking
    reactiveDodge->AddChild(std::make_unique<HEIN::BTCooldownDecorator>(GameConstants::BossAI::ReactiveDodgeCooldown, std::make_unique<HEIN::BTDodgeNode>(GameConstants::BossAI::DodgeActionDuration)));
    aiBrain->AddChild(std::move(reactiveDodge));

    // ---------------------------------------------------------
    // Enraged Berserker (Health <= 50%)
    // ---------------------------------------------------------
    std::unique_ptr<HEIN::BTSequence> phase2Branch = std::make_unique<HEIN::BTSequence>();
    phase2Branch->AddChild(std::make_unique<HEIN::BTCheckHealth>(0.0f, GameConstants::BossAI::Phase2HealthThreshold));

    std::unique_ptr<HEIN::BTSelector> phase2Combat = std::make_unique<HEIN::BTSelector>();

    // Shield-Crush Heavy Attack (If player is blocking in close range)
    std::unique_ptr<HEIN::BTSequence> p2GuardBreak = std::make_unique<HEIN::BTSequence>();
    p2GuardBreak->AddChild(std::make_unique<HEIN::BTCheckDistance>(0.0f, GameConstants::BossAI::CloseGuardBreakDistance));
    p2GuardBreak->AddChild(std::make_unique<HEIN::BTCheckTargetStance>(CombatStance::Blocking));
    p2GuardBreak->AddChild(std::make_unique<HEIN::BTCooldownDecorator>(GameConstants::BossAI::P2GuardBreakCooldown, std::make_unique<HEIN::BTAttackNode>(GameConstants::BossAI::HeavyAttackDuration, GameConstants::BossAI::CloseGuardBreakDistance, true)));
    phase2Combat->AddChild(std::move(p2GuardBreak));

    // Heavy Attack Ground Slam Mix-Up (Elden Ring Shockwave Stomp every ~2.0s)
    std::unique_ptr<HEIN::BTSequence> p2Heavy = std::make_unique<HEIN::BTSequence>();
    p2Heavy->AddChild(std::make_unique<HEIN::BTCheckDistance>(0.0f, GameConstants::BossAI::HeavyAttackDistance));
    p2Heavy->AddChild(std::make_unique<HEIN::BTCooldownDecorator>(GameConstants::BossAI::P2HeavyCooldown, std::make_unique<HEIN::BTAttackNode>(GameConstants::BossAI::HeavyAttackDuration, GameConstants::BossAI::HeavyAttackDistance, true)));
    phase2Combat->AddChild(std::move(p2Heavy));

    // Relentless 3-Hit Combo Chain (6 Permutations: 123, 132, 213, 231, 312, 321)
    std::unique_ptr<HEIN::BTSequence> p2Combo = std::make_unique<HEIN::BTSequence>();
    p2Combo->AddChild(std::make_unique<HEIN::BTCheckDistance>(0.0f, GameConstants::BossAI::ComboAttackDistance));
    p2Combo->AddChild(std::make_unique<HEIN::BTCooldownDecorator>(GameConstants::BossAI::P2ComboCooldown, std::make_unique<HEIN::BTAttackNode>(GameConstants::BossAI::ComboAttackDuration, GameConstants::BossAI::ComboAttackDistance, false)));
    phase2Combat->AddChild(std::move(p2Combo));

    // Fast gap closer (chase speed increased to 130.0f)
    phase2Combat->AddChild(std::make_unique<HEIN::BTChaseNode>(GameConstants::BossAI::ChaseArrivalDistance, GameConstants::BossAI::Phase2ChaseSpeed));

    phase2Branch->AddChild(std::move(phase2Combat));
    aiBrain->AddChild(std::move(phase2Branch));

    // ---------------------------------------------------------
    // Phase 1 : Calculated duelist (Health > 50%)
    // ---------------------------------------------------------
    std::unique_ptr<HEIN::BTSequence> phase1Branch = std::make_unique<HEIN::BTSequence>();
    phase1Branch->AddChild(std::make_unique<HEIN::BTCheckHealth>(GameConstants::BossAI::Phase2HealthThreshold, 1.0f));

    std::unique_ptr<HEIN::BTSelector> phase1Combat = std::make_unique<HEIN::BTSelector>();

    // Shield-Crush Heavy Attack (Punish player turtling behind guard)
    std::unique_ptr<HEIN::BTSequence> p1GuardBreak = std::make_unique<HEIN::BTSequence>();
    p1GuardBreak->AddChild(std::make_unique<HEIN::BTCheckDistance>(0.0f, GameConstants::BossAI::CloseGuardBreakDistance));
    p1GuardBreak->AddChild(std::make_unique<HEIN::BTCheckTargetStance>(CombatStance::Blocking));
    p1GuardBreak->AddChild(std::make_unique<HEIN::BTCooldownDecorator>(GameConstants::BossAI::P1GuardBreakCooldown, std::make_unique<HEIN::BTAttackNode>(GameConstants::BossAI::HeavyAttackDuration, GameConstants::BossAI::CloseGuardBreakDistance, true)));
    phase1Combat->AddChild(std::move(p1GuardBreak));

    // Heavy Attack Ground Slam Mix-Up (Elden Ring Shockwave Stomp every ~3.0s)
    std::unique_ptr<HEIN::BTSequence> p1Heavy = std::make_unique<HEIN::BTSequence>();
    p1Heavy->AddChild(std::make_unique<HEIN::BTCheckDistance>(0.0f, 32.0f));
    p1Heavy->AddChild(std::make_unique<HEIN::BTCooldownDecorator>(3.0f, std::make_unique<HEIN::BTAttackNode>(GameConstants::BossAI::HeavyAttackDuration, GameConstants::BossAI::HeavyAttackDistance, true)));
    phase1Combat->AddChild(std::move(p1Heavy));

    // 3-Hit Combo Strike (6 Permutations: 123, 132, 213, 231, 312, 321)
    std::unique_ptr<HEIN::BTSequence> p1Combo = std::make_unique<HEIN::BTSequence>();
    p1Combo->AddChild(std::make_unique<HEIN::BTCheckDistance>(0.0f, GameConstants::BossAI::ComboAttackDistance));
    p1Combo->AddChild(std::make_unique<HEIN::BTCooldownDecorator>(GameConstants::BossAI::P2ComboCooldown, std::make_unique<HEIN::BTAttackNode>(GameConstants::BossAI::ComboAttackDuration, GameConstants::BossAI::ComboAttackDistance, false)));
    phase1Combat->AddChild(std::move(p1Combo));

    // Deliberate pursuit (chase speed increased to 85.0f)
    phase1Combat->AddChild(std::make_unique<HEIN::BTChaseNode>(GameConstants::BossAI::ChaseArrivalDistance, GameConstants::BossAI::Phase1ChaseSpeed));

    phase1Branch->AddChild(std::move(phase1Combat));
    aiBrain->AddChild(std::move(phase1Branch));

    // =========================================================================
    // FALLBACK IDLE
    // =========================================================================
    aiBrain->AddChild(std::make_unique<HEIN::BTIdleNode>());

    HEIN::BehaviourTreeComponent* btComp = enemyActor->GetComponent<HEIN::BehaviourTreeComponent>();
    if (!btComp)
    {
        btComp = enemyActor->AddComponent<HEIN::BehaviourTreeComponent>();
    }
    btComp->Initialize(std::move(aiBrain), actorManager, targetID);
}

void HEIN::ActorFactory::PossessVessel(
    Actor* vessel,
    ActorManager& actorManager,
    GameContext* gameContext
)
{
    if (!vessel) return;

    vessel->SetActorType(HEIN::ActorType::Player);
    vessel->SetTag(L"Player");

    // Stand upright
    if (auto* transform = vessel->GetComponent<HEIN::TransformComponent>())
    {
        transform->SetRotationEuler(DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f));
    }

    // Make mortal: preserve vessel's own current health, only restore if dead
    if (auto* health = vessel->GetComponent<HEIN::HealthComponent>())
    {
        if (health->GetCurrentHealth() <= 0.0f)
        {
            health->SetCurrentHealth(health->GetMaxHealth());
        }
        health->SetGameplayInvincible(false);
    }

    // Ensure GaugeComponent is visible and active for player HUD
    if (auto* gauge = vessel->GetComponent<HEIN::GaugeComponent>())
    {
        gauge->SetVisible(true);
    }

    // Enable procedural animation
    if (auto* procAnim = vessel->GetComponent<HEIN::ProceduralAnimationComponent>())
    {
        procAnim->SetEnabled(true);
    }

    // Ensure PlayerInputComponent exists and is active
    if (!vessel->GetComponent<HEIN::PlayerInputComponent>())
    {
        vessel->AddComponent<HEIN::PlayerInputComponent>(&actorManager);
    }

    // Ensure PlayerBlackBoard and CharacterMovementComponent exist
    if (!vessel->GetComponent<HEIN::PlayerBlackBoard>())
    {
        if (auto* baseBB = vessel->GetComponent<HEIN::CombatBlackBoard>())
        {
            vessel->RemoveComponent(baseBB);
        }
        vessel->AddComponent<HEIN::PlayerBlackBoard>();
    }
    if (!vessel->GetComponent<HEIN::CharacterMovementComponent>())
    {
        vessel->AddComponent<HEIN::CharacterMovementComponent>();
    }

    if (auto* rb = vessel->GetComponent<HEIN::RigidBodyComponent>())
    {
        rb->SetKinematic(false);
        rb->SetVelocity(DirectX::SimpleMath::Vector3::Zero);
    }

    // Switch to Idle animation
    if (auto* model = vessel->GetComponent<HEIN::SkinnedModelComponent>())
    {
        model->ChangeAnimation("Idle");
    }

    // Mark as occupied
    if (auto* possessable = vessel->GetComponent<HEIN::PossessableComponent>())
    {
        possessable->SetOccupied(true);
    }

    // Ensure weapon children (e.g. vesselsword) are owned by the possessed body and ready for combat
    bool hasWeapon = false;
    for (ActorID childID : vessel->GetChildren())
    {
        if (Actor* child = actorManager.GetActor(childID))
        {
            child->SetOwnerID(vessel->GetID());
            if (auto* socketAttachment = child->GetComponent<HEIN::SocketAttachmentComponent>())
            {
                socketAttachment->SetOwnerTag(vessel->GetTag());
                socketAttachment->Initialize(vessel->GetID(), socketAttachment->GetSocketName());
            }
            if (auto* damageDealer = child->GetComponent<HEIN::DamageDealerComponent>())
            {
                damageDealer->Start();
                hasWeapon = true;
            }
            if (auto* weaponTrail = child->GetComponent<HEIN::WeaponTrailComponent>())
            {
                weaponTrail->Start();
            }
        }
    }

    // Auto-create and equip sword if vessel has no weapon child (e.g. if vesselSword was deleted)
    if (!hasWeapon && gameContext != nullptr)
    {
        CreateSword(actorManager, *gameContext, vessel->GetID(), GameConstants::Weapons::DefaultSwordDamage);
    }

    vessel->Start();
}