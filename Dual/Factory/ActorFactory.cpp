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
#include "../../External/Engine/Components/ColliderComponent/MeshColliderComponent.h"
#include "../../External/Engine/Components/ColliderComponent/AABBColliderComponent.h"
#include "../../External/Engine/BehaviourTree/BTSelector.h"
#include "../../External/Engine/BehaviourTree/BTSequence.h"
#include "../../External/Engine/Camera/CameraController.h"
#include "../../External/Engine/Factory/ComponentFactory.h"
#include "ActorFactory.h"
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

    playerActor->AddComponent<HEIN::CombatBlackBoard>();
    playerActor->AddComponent<HEIN::PlayerInputComponent>(&actorManager);
    playerActor->AddComponent<HEIN::TargetTrackingComponent>(&actorManager, HEIN::ActorType::Enemy);
    playerActor->AddComponent<HEIN::CharacterMovementComponent>();
    playerActor->AddComponent<HEIN::ProceduralAnimationComponent>(&actorManager);

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
    axeDamage->Initialize(damage, DamageType::Physical, true);

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
    floorPhysics->SetCollisionLayer(CollisionLayer::Layer_Environment);
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
    LeftupLegCapsule->SetTrigger(true);
    LeftLegCapsule->SetTrigger(true);

    HEIN::CombatBlackBoard* bb = enemyActor->AddComponent<HEIN::CombatBlackBoard>();
    bb->spawnPosition = ptransform->GetPosition();
    bb->hasSetSpawnPosition = true;

    enemyActor->AddComponent<HEIN::CharacterMovementComponent>();

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

    // UI pulls Block and Dodge directly from the actor's CombatBlackBoard
    GaugeComponent::SetExternalSyncHandler([](Actor* target, GaugeType type, float& outCurrent, float& outMax) -> bool {
        if (!target) return false;
        auto* bb = target->GetComponent<CombatBlackBoard>();
        if (!bb) return false;

        if (type == GaugeType::Block)
        {
            outCurrent = bb->currentBlockStamina;
            outMax = bb->maxBlockStamina;
            return true;
        }
        else if (type == GaugeType::Dodge)
        {
            outCurrent = (bb->dodgeCooldownTimer > 0.0f)
                ? (bb->maxDodgeCooldown - bb->dodgeCooldownTimer)
                : bb->maxDodgeCooldown;
            outMax = bb->maxDodgeCooldown;
            return true;
        }
        return false;
    });

    // Intercept damage for blocking states
    DamageSystem::SetDamageInterceptor([](Actor* attacker, Actor* victim, float damage) -> bool {
        if (!victim) return true;
        auto* sm = victim->GetComponent<CombatStateMachineComponent>();
        auto* bb = victim->GetComponent<CombatBlackBoard>();
        if (sm && sm->IsBlocking() && bb && !bb->isBlockBroken)
        {
            // Block absorbs the hit! Deduct stamina
            bb->currentBlockStamina -= 1.0f; // 1 block point per hit
            if (bb->currentBlockStamina <= 0.0f)
            {
                bb->currentBlockStamina = 0.0f;
                bb->isBlockBroken = true;
                // Guard broke! Stun the victim immediately
                Messenger::GetInstance()->Notify(victim->GetID(), Message::HIT_STUN);
                return true; // Guard broke! Damage breaks through to health
            }
            return false; // Attack blocked! No health damage
        }

        // If attacker is executing a heavy attack, ensure victim is stunned
        if (attacker)
        {
            auto* attackerSM = attacker->GetComponent<CombatStateMachineComponent>();
            if (attackerSM)
            {
                const std::string& attackerState = attackerSM->GetCurrentStateName();
                if (attackerState == "HeavyAttack" || attackerState.find("Heavy") != std::string::npos)
                {
                    Messenger::GetInstance()->Notify(victim->GetID(), Message::HIT_STUN);
                }
            }
        }

        return true; // Normal hit, apply full damage
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
    // If the player starts swinging in close quarters, the boss dodge
    std::unique_ptr<HEIN::BTSequence> reactiveDodge = std::make_unique<HEIN::BTSequence>();
    reactiveDodge->AddChild(std::make_unique<HEIN::BTCheckDistance>(0.0f, 20.0f));
    reactiveDodge->AddChild(std::make_unique<HEIN::BTCheckTargetStance>(true));  // is targetAttacking
    reactiveDodge->AddChild(std::make_unique<HEIN::BTCooldownDecorator>(2.5f, std::make_unique<HEIN::BTDodgeNode>(1.2f)));
    aiBrain->AddChild(std::move(reactiveDodge));

    // ---------------------------------------------------------
    // 3. Enraged Berserker (Health <= 50%)
    // ---------------------------------------------------------
    std::unique_ptr<HEIN::BTSequence> phase2Branch = std::make_unique<HEIN::BTSequence>();
    phase2Branch->AddChild(std::make_unique<HEIN::BTCheckHealth>(0.0f, 0.5f));

    std::unique_ptr<HEIN::BTSelector> phase2Combat = std::make_unique<HEIN::BTSelector>();

    // 3A. Shield-Crush Heavy Attack (If player is blocking in close range)
    std::unique_ptr<HEIN::BTSequence> p2GuardBreak = std::make_unique<HEIN::BTSequence>();
    p2GuardBreak->AddChild(std::make_unique<HEIN::BTCheckDistance>(0.0f, 28.0f));
    p2GuardBreak->AddChild(std::make_unique<HEIN::BTCheckTargetStance>(CombatStance::Blocking));
    p2GuardBreak->AddChild(std::make_unique<HEIN::BTCooldownDecorator>(1.2f, std::make_unique<HEIN::BTAttackNode>(3.2f, 28.0f, true)));
    phase2Combat->AddChild(std::move(p2GuardBreak));

    // 3B. Relentless 3-Hit Combo Chain (Aggressive attack with hit-confirmed combo flow)
    std::unique_ptr<HEIN::BTSequence> p2Combo = std::make_unique<HEIN::BTSequence>();
    p2Combo->AddChild(std::make_unique<HEIN::BTCheckDistance>(0.0f, 28.0f));
    p2Combo->AddChild(std::make_unique<HEIN::BTCooldownDecorator>(0.8f, std::make_unique<HEIN::BTAttackNode>(3.6f, 28.0f, false)));
    phase2Combat->AddChild(std::move(p2Combo));

    // 3C. Heavy Attack Mix-Up
    std::unique_ptr<HEIN::BTSequence> p2Heavy = std::make_unique<HEIN::BTSequence>();
    p2Heavy->AddChild(std::make_unique<HEIN::BTCheckDistance>(20.0f, 35.0f));
    p2Heavy->AddChild(std::make_unique<HEIN::BTCooldownDecorator>(2.5f, std::make_unique<HEIN::BTAttackNode>(3.2f, 35.0f, true)));
    phase2Combat->AddChild(std::move(p2Heavy));

    // 3D. Fast gap closer
    phase2Combat->AddChild(std::make_unique<HEIN::BTChaseNode>(25.0f, 110.0f));

    phase2Branch->AddChild(std::move(phase2Combat));
    aiBrain->AddChild(std::move(phase2Branch));

    // ---------------------------------------------------------
    // 4. Phase 1 : Calculated duelist (Health > 50%)
    // ---------------------------------------------------------
    std::unique_ptr<HEIN::BTSequence> phase1Branch = std::make_unique<HEIN::BTSequence>();
    phase1Branch->AddChild(std::make_unique<HEIN::BTCheckHealth>(0.5f, 1.0f));

    std::unique_ptr<HEIN::BTSelector> phase1Combat = std::make_unique<HEIN::BTSelector>();

    // 4A. Shield-Crush Heavy Attack (Punish player turtling behind guard)
    std::unique_ptr<HEIN::BTSequence> p1GuardBreak = std::make_unique<HEIN::BTSequence>();
    p1GuardBreak->AddChild(std::make_unique<HEIN::BTCheckDistance>(0.0f, 28.0f));
    p1GuardBreak->AddChild(std::make_unique<HEIN::BTCheckTargetStance>(CombatStance::Blocking));
    p1GuardBreak->AddChild(std::make_unique<HEIN::BTCooldownDecorator>(2.0f, std::make_unique<HEIN::BTAttackNode>(3.2f, 28.0f, true)));
    phase1Combat->AddChild(std::move(p1GuardBreak));

    // 4B. 3-Hit Combo Strike (Hit-confirmed: chains to 2nd & 3rd attacks on hit)
    std::unique_ptr<HEIN::BTSequence> p1Combo = std::make_unique<HEIN::BTSequence>();
    p1Combo->AddChild(std::make_unique<HEIN::BTCheckDistance>(0.0f, 28.0f));
    p1Combo->AddChild(std::make_unique<HEIN::BTCooldownDecorator>(0.8f, std::make_unique<HEIN::BTAttackNode>(3.6f, 28.0f, false)));
    phase1Combat->AddChild(std::move(p1Combo));

    // 4C. Mid-Range Heavy Attack Lunge Mix-Up
    std::unique_ptr<HEIN::BTSequence> p1Heavy = std::make_unique<HEIN::BTSequence>();
    p1Heavy->AddChild(std::make_unique<HEIN::BTCheckDistance>(20.0f, 35.0f));
    p1Heavy->AddChild(std::make_unique<HEIN::BTCooldownDecorator>(4.5f, std::make_unique<HEIN::BTAttackNode>(3.2f, 35.0f, true)));
    phase1Combat->AddChild(std::move(p1Heavy));

    // 4D. Deliberate pursuit
    phase1Combat->AddChild(std::make_unique<HEIN::BTChaseNode>(25.0f, 55.0f));

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