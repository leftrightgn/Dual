//--------------------------------------------------------------------------------------
// File: GameScene.cpp
// Author: HEIN SOE KHANT
//--------------------------------------------------------------------------------------
#include "pch.h"
#include "GameScene.h"
#include "../../../External/Engine/Common/InputManager.h"
#include "../../../External/Engine/Camera/CameraController.h"
#include "../../../External/Engine/Camera/SpringCameraMode.h"
#include "../../../External/Engine/Components/TransformComponent.h"
#include "../../../External/Engine/Components/SkinnedModelComponent.h"
#include "../../../External/Engine/Components/LightComponent.h"
#include "../../../External/Engine/FrameWork/GameContext.h"
#include <Components/PlayerInputComponent.h>
#include <Components/CombatStateMachineComponent.h>
#include <Components/CharacterMovementComponent.h>
#include <Components/BehaviourTreeComponent.h>
#include <Components/PossessableComponent.h>
#include <BlackBoard/CombatBlackBoard.h>
#include "../../../External/Engine/Components/DamageDealerComponent.h"
#include "../../../External/Engine/Components/SocketAttachmentComponent.h"
#include "../../../External/Engine/Components/RigidBodyComponent.h"
#include "../../../External/Engine/Components/ColliderComponent/CapsuleColliderComponent.h"
#include "../../../External/Engine/Components/ColliderComponent/AABBColliderComponent.h"
#include "../../../External/Engine/Components/ColliderComponent/TerrainColliderComponent.h"
#include "../../../External/Engine/Components/TerrainComponent.h"
#include "../../../External/Engine/Factory/ComponentFactory.h"
#include <Factory/ActorFactory.h>
#include "../../../External/Engine/Components/HealthComponent.h"
#include "../../../External/Engine/ImGui/imgui.h"
#include "../../../External/Engine/ImGui/ImGuizmo.h"
#include "../../../External/Engine/Common/Event.h"
#include "../../../External/Engine/Common/json.hpp"
#include <commdlg.h>
#include <fstream>
#include <filesystem>
#include <Windows.h>
#include <utility>
#include <cmath>

namespace
{
    // Primary scene configuration path (DOD scene layout, transforms, actor instances)
    std::string GetSceneDataWritePath()
    {
        namespace fs = std::filesystem;
        std::error_code ec;
        std::string target = "Resources/Data/GameScene.json";
        if (!fs::exists("Resources", ec) && fs::exists("Dual/Resources", ec))
        {
            target = "Dual/Resources/Data/GameScene.json";
        }
        fs::create_directories(fs::path(target).parent_path(), ec);
        return target;
    }
}

using namespace DirectX;

// --------------------------------------------------------------------------------------
// シーン切り替え時に呼び出される関数 (OnEnter)
// --------------------------------------------------------------------------------------
void GameScene::OnEnter(GameContext& gameContext)
{
    // Clean all previous actors to guarantee scene isolation from TitleScene or earlier runs
    m_actorManager.ClearAllActors();
    gameContext.actorManager = &m_actorManager;
    gameContext.shadowSystem = &m_shadowSystem;

    // Initialize Core Systems & Viewport Pipeline
    m_physicsSystem = std::make_unique<HEIN::PhysicsSystem>();
    m_damageSystem = std::make_unique<HEIN::DamageSystem>();

    m_skybox = std::make_unique<HEIN::Skybox>();
    m_skybox->Initialize(gameContext, L"Resources/Textures/skybox.dds");

    m_shadowSystem.Initialize(gameContext.deviceResources.GetD3DDevice(), 2048, 2048);

    D3D11_VIEWPORT viewport = gameContext.deviceResources.GetScreenViewport();
    float aspectRatio = static_cast<float>(viewport.Width) / static_cast<float>(viewport.Height);
    m_proj = SimpleMath::Matrix::CreatePerspectiveFieldOfView(DirectX::XM_PI / 4.0f, aspectRatio, 0.01f, 5000.0f);

    // Register all game-level components into ComponentFactory reflection registry
    HEIN::ActorFactory::RegisterGameComponents();

    // Load the Scene Data from JSON (Prioritizes GameScene.json, falls back to AutoSave.json)
    LoadSceneData(gameContext);

    // Re-establish "Glue" Connections, AI, weapons, Start(), and camera modes
    PostSceneLoadInit(gameContext);

    // Debug Display Controller & Trigger Event Listeners
    m_debugDisplay = std::make_unique<HEIN::DebugDisplayController>();
    m_debugDisplay->Initialize();
    UpdateDebugTargets();

    gameContext.eventManager->AddTriggerListener([this](const HEIN::TriggerEventPayLoad& payLoad) {
        m_damageSystem->HandlTriggerHit(payLoad, m_actorManager);
    });
}

// --------------------------------------------------------------------------------------
// シーン終了時に呼び出される関数 (OnExit)
// --------------------------------------------------------------------------------------
void GameScene::OnExit(GameContext& gameContext)
{
    // Auto-save on exit to primary GameScene path
    nlohmann::json j = m_actorManager.Serialize();
    std::ofstream sceneFile(GetSceneDataWritePath());
    if (sceneFile.is_open())
    {
        sceneFile << j.dump(4);
    }

    gameContext.actorManager = nullptr;
    gameContext.shadowSystem = nullptr;
    gameContext.mainCamera = nullptr;
    m_actorManager.ClearAllActors();
}

// --------------------------------------------------------------------------------------
// 更新 (Update) - Orchestrates ECS Pipeline Phases
// --------------------------------------------------------------------------------------
void GameScene::Update(GameContext& gameContext)
{
    gameContext.actorManager = &m_actorManager;
    gameContext.shadowSystem = &m_shadowSystem;
    gameContext.isEditorMode = (!m_isPlaying || (m_debugDisplay && m_debugDisplay->isVisible()));
    gameContext.showUI = (m_debugDisplay != nullptr) ? m_debugDisplay->GetDebugUI().IsShowUIEnabled() : true;

    float deltaTime = static_cast<float>(gameContext.timer.GetElapsedSeconds());

    // Phase 1: Editor & Tooling Phase
    m_debugDisplay->Update(gameContext, m_actorManager);
    HandleEditorActions(gameContext);

    if (!m_isPlaying)
    {
        deltaTime = 0.0f;
    }

    // Phase 2: Input Phase
    ProcessInputPhase(gameContext);

    // Phase 3: Simulation Phase (Physics, Hierarchies, LateUpdate, Collisions, Tracking)
    ProcessSimulationPhase(gameContext, deltaTime);

    // Phase 4: Lifecycle Phase (Garbage Collection / Death System)
    ProcessLifecyclePhase(gameContext);
}

// --------------------------------------------------------------------------------------
// 描画 (Render) - Orchestrates Rendering Passes
// --------------------------------------------------------------------------------------
void GameScene::Render(GameContext& gameContext)
{
    ID3D11DeviceContext* context = gameContext.deviceResources.GetD3DDeviceContext();
    DirectX::SimpleMath::Matrix view = DirectX::SimpleMath::Matrix::Identity;

    HEIN::CameraController* activeCamera = GetActiveCameraController(gameContext);
    if (activeCamera != nullptr)
    {
        view = activeCamera->GetView();
    }

    gameContext.actorManager = &m_actorManager;
    gameContext.shadowSystem = &m_shadowSystem;
    gameContext.isEditorMode = (!m_isPlaying || (m_debugDisplay && m_debugDisplay->isVisible()));
    gameContext.showUI = (m_debugDisplay != nullptr) ? m_debugDisplay->GetDebugUI().IsShowUIEnabled() : true;

    // Pass 1: Shadow Map Depth Generation Pass
    RenderShadowPhase(gameContext, context);

    // Pass 2: Main Color & Scene Drawing Pass
    RenderMainPassPhase(gameContext, context, view);

    // Pass 3: ImGui Editor & Gizmo Overlay Pass
    m_debugDisplay->Render(gameContext, m_actorManager, m_skybox.get(), view, m_proj);

    // Pass 4: Ghost Possession Prompt Overlay
    if (gameContext.showUI && m_isGhost && m_targetVesselID != HEIN::INVALID_ACTOR_ID)
    {
        ImGuiIO& io = ImGui::GetIO();
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.78f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs;
        if (ImGui::Begin("PossessOverlayPrompt", nullptr, flags))
        {
            ImGui::SetWindowFontScale(1.8f);
            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.8f, 1.0f), "[Q] POSSESS VESSEL");
            ImGui::End();
        }
    }
}

// --------------------------------------------------------------------------------------
// ECS Pipeline Phase Implementations
// --------------------------------------------------------------------------------------

void GameScene::ProcessInputPhase(GameContext& gameContext)
{
    HEIN::CameraController* camera = GetActiveCameraController(gameContext);
    HEIN::Actor* player = GetPlayerActor();

    bool isUICapturingMouse = ImGui::GetIO().WantCaptureMouse || ImGuizmo::IsUsing() || ImGuizmo::IsOver();
    bool isUICapturingKeyboard = ImGui::GetIO().WantCaptureKeyboard;

    // Camera Input Processing
    if (!m_debugDisplay->isMagnified() && camera != nullptr)
    {
        HEIN::CameraInputState cameraInput;
        const DirectX::Mouse::State& mouseState = gameContext.mouseState;

        if (mouseState.positionMode == DirectX::Mouse::MODE_RELATIVE)
        {
            cameraInput.mouseX = isUICapturingMouse ? 0.0f : static_cast<float>(mouseState.x);
            cameraInput.mouseY = isUICapturingMouse ? 0.0f : static_cast<float>(mouseState.y);
        }
        else
        {
            // In absolute mode (cursor free / UI mode), do NOT feed raw screen pixel coordinates into rotation delta.
            // Only feed delta if right-click dragging and UI is not capturing mouse.
            if (!isUICapturingMouse && mouseState.rightButton && gameContext.inputManager != nullptr)
            {
                auto mouseDelta = gameContext.inputManager->GetMouseDelta();
                cameraInput.mouseX = static_cast<float>(mouseDelta.first);
                cameraInput.mouseY = static_cast<float>(mouseDelta.second);
            }
            else
            {
                cameraInput.mouseX = 0.0f;
                cameraInput.mouseY = 0.0f;
            }
        }

        cameraInput.isLeftMouseDown = isUICapturingMouse ? false : mouseState.leftButton;
        cameraInput.scrollWheelDelta = static_cast<float>(mouseState.scrollWheelValue);
        cameraInput.ignoreScroll = isUICapturingMouse;
        cameraInput.ignoreMovement = isUICapturingKeyboard;

        camera->ProcessInput(cameraInput);

        HEIN::CameraType targetCameraType;
        if (gameContext.inputManager->WasCameraSwitchPressed(gameContext, targetCameraType))
        {
            if (targetCameraType == HEIN::CameraType::Spring)
            {
                camera->RequestSwitch(targetCameraType);
            }
        }
    }

    // Player Input Processing
    if (m_isPlaying && player != nullptr && !m_debugDisplay->isMagnified() && !isUICapturingMouse && !isUICapturingKeyboard)
    {
        gameContext.inputManager->BroadCastPlayerInput(gameContext, player->GetID());

        if (auto* inputComp = player->GetComponent<HEIN::PlayerInputComponent>())
        {
            inputComp->ProcessInput(gameContext);
        }
    }

    // Ghost Possession System (Press Q when near an unoccupied vessel)
    m_targetVesselID = HEIN::INVALID_ACTOR_ID;
    if (m_isGhost && m_isPlaying && player != nullptr)
    {
        auto* ghostTrans = player->GetComponent<HEIN::TransformComponent>();
        if (ghostTrans != nullptr)
        {
            DirectX::SimpleMath::Vector3 ghostPos = ghostTrans->GetPosition();
            float closestDistSq = FLT_MAX;
            HEIN::Actor* closestVessel = nullptr;

            for (const auto& pair : m_actorManager.GetAllActors())
            {
                HEIN::Actor* actor = pair.second.get();
                if (!actor || actor == player) continue;

                auto* possessable = actor->GetComponent<HEIN::PossessableComponent>();
                if (possessable != nullptr && !possessable->IsOccupied())
                {
                    auto* trans = actor->GetComponent<HEIN::TransformComponent>();
                    if (trans != nullptr)
                    {
                        float distSq = DirectX::SimpleMath::Vector3::DistanceSquared(ghostPos, trans->GetPosition());
                        float radius = possessable->GetPossessRadius();
                        if (distSq <= radius * radius && distSq < closestDistSq)
                        {
                            closestDistSq = distSq;
                            closestVessel = actor;
                        }
                    }
                }
            }

            if (closestVessel != nullptr)
            {
                m_targetVesselID = closestVessel->GetID();

                if (gameContext.keyboardTracker.pressed.Q)
                {
                    // 1. Deactivate and hide Ghost Actor (preserve in memory so it can return if the possessed vessel dies)
                    player->SetActive(false);
                    player->SetVisible(false);
                    if (auto* rb = player->GetComponent<HEIN::RigidBodyComponent>())
                    {
                        rb->SetVelocity(DirectX::SimpleMath::Vector3::Zero);
                        rb->SetKinematic(true);
                    }
                    if (auto* bb = player->GetComponent<HEIN::CombatBlackBoard>())
                    {
                        bb->moveIntent = DirectX::SimpleMath::Vector3::Zero;
                        bb->currentVelocity = DirectX::SimpleMath::Vector3::Zero;
                    }
                    if (auto* model = player->GetComponent<HEIN::SkinnedModelComponent>())
                    {
                        model->SetVisible(false);
                    }

                    // 2. Possess the Vessel
                    HEIN::ActorFactory::PossessVessel(closestVessel, m_actorManager);
                    m_playerID = closestVessel->GetID();
                    m_isGhost = false;
                    m_targetVesselID = HEIN::INVALID_ACTOR_ID;

                    // 3. Re-hook Camera to newly possessed vessel
                    if (camera != nullptr)
                    {
                        SetupCameraModes(gameContext, camera, closestVessel->GetComponent<HEIN::SkinnedModelComponent>());
                    }

                    // 4. Re-hook Boss AI targeting
                    if (HEIN::Actor* enemy = GetEnemyActor())
                    {
                        HEIN::ActorFactory::SetupEnemyAI(enemy, &m_actorManager, m_playerID);
                    }
                }
            }
        }
    }
}

void GameScene::ProcessSimulationPhase(GameContext& gameContext, float deltaTime)
{
    if (m_isPlaying)
    {
        m_actorManager.UpdateAll(deltaTime);
        m_physicsSystem->UpdateMovement(gameContext, m_actorManager, deltaTime);
        m_physicsSystem->UpdateCollisions(gameContext, m_actorManager, deltaTime);
        m_actorManager.UpdateAllHierarchies(); // Math Cascades Downwards
        m_actorManager.LateUpdateAll(deltaTime);
    }
    else
    {
        // When paused/stopped, update hierarchies and dynamic sockets/bone links so Editor changes reflect live
        m_actorManager.UpdateAllHierarchies();
        m_actorManager.LateUpdateAll(0.0f);
    }

    // Dynamic Camera Target Tracking
    HEIN::Actor* player = GetPlayerActor();
    if (player != nullptr)
    {
        auto* pTransform = player->GetComponent<HEIN::TransformComponent>();
        if (pTransform != nullptr)
        {
            DirectX::SimpleMath::Vector3 playerPos = pTransform->GetPosition();
            DirectX::SimpleMath::Vector3 targetPos = playerPos + DirectX::SimpleMath::Vector3(0.0f, 15.0f, 0.0f);

            auto* pModel = player->GetComponent<HEIN::SkinnedModelComponent>();
            if (pModel != nullptr)
            {
                DirectX::SimpleMath::Vector3 headPos = pModel->GetBoneWorldPosition(L"mixamorig:HeadTop_End", pTransform->GetWorldMatrix());
                if (headPos != DirectX::SimpleMath::Vector3::Zero && (headPos - playerPos).LengthSquared() <= 10000.0f)
                {
                    targetPos = headPos;
                }
            }

            auto* pSM = player->GetComponent<HEIN::CombatStateMachineComponent>();
            float heightAboveRoot = targetPos.y - playerPos.y;

            static float s_standingHeight = 15.0f;
            bool isDodging = (pSM && pSM->GetCurrentStateName() == "Dodge") || (heightAboveRoot < 6.0f);

            if (!isDodging && heightAboveRoot > 8.0f)
            {
                s_standingHeight = heightAboveRoot;
            }

            if (isDodging)
            {
                // When dodging, maintain camera target at stable standing height above root position
                targetPos.y = pTransform->GetPosition().y + s_standingHeight;
            }

            m_targetPos = targetPos;
        }
    }

    // Update Projection Matrix from Active Camera FOV
    HEIN::CameraController* activeCamera = gameContext.mainCamera;
    if (activeCamera != nullptr)
    {
        D3D11_VIEWPORT viewport = gameContext.deviceResources.GetScreenViewport();
        float aspectRatio = static_cast<float>(viewport.Width) / static_cast<float>(viewport.Height);
        m_proj = DirectX::SimpleMath::Matrix::CreatePerspectiveFieldOfView(activeCamera->GetFov(), aspectRatio, 0.1f, 5000.0f);
    }
}

void GameScene::ProcessLifecyclePhase(GameContext& gameContext)
{
    // Health & Death Evaluation
    HEIN::Actor* playerDiedActor = nullptr;

    for (const auto& pair : m_actorManager.GetAllActors())
    {
        HEIN::Actor* currentActor = pair.second.get();
        if (!currentActor) continue;

        auto* health = currentActor->GetComponent<HEIN::HealthComponent>();
        if (health != nullptr && health->isDead())
        {
            if (currentActor->GetID() == m_playerID && currentActor->GetActorType() == HEIN::ActorType::Player)
            {
                playerDiedActor = currentActor;
            }
            else
            {
                m_actorManager.DestroyID(currentActor->GetID());
            }
        }
    }

    // Handle Player Death -> Spawn Ghost & Destroy Current Body
    if (playerDiedActor != nullptr && !m_isGhost)
    {
        DirectX::SimpleMath::Vector3 deathPos(0.0f, 0.0f, 0.0f);
        if (auto* transform = playerDiedActor->GetComponent<HEIN::TransformComponent>())
        {
            deathPos = transform->GetPosition();
        }

        // Clamp death position to ground surface elevation (terrain or static environment colliders like Stage floor)
        float groundHeight = -FLT_MAX;
        bool foundGround = false;
        DirectX::SimpleMath::Vector3 terrainNormal;
        for (const auto& pair : m_actorManager.GetAllActors())
        {
            if (auto* terrainCol = pair.second->GetComponent<HEIN::TerrainColliderComponent>())
            {
                float th = 0.0f;
                if (terrainCol->GetHeightAtPosition(deathPos.x, deathPos.z, th, terrainNormal))
                {
                    groundHeight = th;
                    foundGround = true;
                }
                break;
            }
        }
        for (const auto& pair : m_actorManager.GetAllActors())
        {
            if (!pair.second || !pair.second->IsActive()) continue;
            for (auto* aabbComp : pair.second->GetComponents<HEIN::AABBColliderComponent>())
            {
                if (!aabbComp || aabbComp->IsTrigger()) continue;
                if (aabbComp->IsWall()) continue;
                if (!aabbComp->IsGround() && (aabbComp->GetCollisionLayer() & (HEIN::CollisionLayer::Layer_Environment | HEIN::CollisionLayer::Layer_Default)) == 0) continue;

                const auto& aabb = aabbComp->GetWorldAABB();
                float minX = aabb.Center.x - aabb.Extents.x;
                float maxX = aabb.Center.x + aabb.Extents.x;
                float minZ = aabb.Center.z - aabb.Extents.z;
                float maxZ = aabb.Center.z + aabb.Extents.z;
                float topY = aabb.Center.y + aabb.Extents.y;
                float botY = aabb.Center.y - aabb.Extents.y;

                if (deathPos.x >= minX && deathPos.x <= maxX && deathPos.z >= minZ && deathPos.z <= maxZ && deathPos.y >= (botY - 5.0f))
                {
                    if (!foundGround || topY > groundHeight)
                    {
                        groundHeight = topY;
                        foundGround = true;
                    }
                }
            }
        }
        if (foundGround && deathPos.y < groundHeight)
        {
            deathPos.y = groundHeight;
        }

        // 1. Keep dead body laying on the ground as a corpse (boss drops aggro)
        playerDiedActor->SetActorType(HEIN::ActorType::Default);
        playerDiedActor->SetTag(L"Corpse");

        if (auto* possessable = playerDiedActor->GetComponent<HEIN::PossessableComponent>())
        {
            possessable->SetOccupied(true);
        }

        if (auto* gauge = playerDiedActor->GetComponent<HEIN::GaugeComponent>())
        {
            gauge->SetVisible(false);
        }

        if (auto* health = playerDiedActor->GetComponent<HEIN::HealthComponent>())
        {
            health->SetGameplayInvincible(true);
        }

        if (auto* input = playerDiedActor->GetComponent<HEIN::PlayerInputComponent>())
        {
            playerDiedActor->RemoveComponent(input);
        }

        if (auto* movement = playerDiedActor->GetComponent<HEIN::CharacterMovementComponent>())
        {
            playerDiedActor->RemoveComponent(movement);
        }

        if (auto* bb = playerDiedActor->GetComponent<HEIN::CombatBlackBoard>())
        {
            bb->moveIntent = DirectX::SimpleMath::Vector3::Zero;
            bb->currentSpeed = 0.0f;
        }

        if (auto* transform = playerDiedActor->GetComponent<HEIN::TransformComponent>())
        {
            DirectX::SimpleMath::Vector3 rot = transform->GetRotationEuler();
            rot.x = -DirectX::XM_PIDIV2; // Lay flat on ground
            transform->SetRotationEuler(rot);
            DirectX::SimpleMath::Vector3 pos = transform->GetPosition();
            if (foundGround)
            {
                pos.y = groundHeight + 0.2f; // Rest dead corpse cleanly on ground or stage floor
            }
            transform->SetPosition(pos);
        }

        // Disable weapon hitbox on corpse
        for (HEIN::ActorID childID : playerDiedActor->GetChildren())
        {
            if (auto* child = m_actorManager.GetActor(childID))
            {
                if (auto* dealer = child->GetComponent<HEIN::DamageDealerComponent>())
                {
                    dealer->SetActive(false);
                }
            }
        }

        // 2. Spawn or activate ghost at death position
        HEIN::Actor* ghost = nullptr;
        for (const auto& pair : m_actorManager.GetAllActors())
        {
            if (pair.second && pair.second->GetActorType() == HEIN::ActorType::Ghost && pair.second.get() != playerDiedActor)
            {
                ghost = pair.second.get();
                break;
            }
        }
        if (ghost == nullptr)
        {
            ghost = m_actorManager.GetActorByName(L"Ghost");
        }
       
        if (ghost != nullptr && ghost != playerDiedActor)
        {
            ghost->SetActive(true);
            ghost->SetVisible(true);
            ghost->SetActorType(HEIN::ActorType::Ghost);
            if (auto* model = ghost->GetComponent<HEIN::SkinnedModelComponent>())
            {
                model->SetVisible(true);
                model->SetGhostMode(true);
                if (model->HasAnimation("Anim0"))
                {
                    model->ChangeAnimation("Anim0");
                }
                else if (model->HasAnimation("Idle"))
                {
                    model->ChangeAnimation("Idle");
                }
            }
            DirectX::SimpleMath::Vector3 ghostSpawnPos = deathPos + DirectX::SimpleMath::Vector3(0.0f, 3.5f, 0.0f);
            if (auto* transform = ghost->GetComponent<HEIN::TransformComponent>())
            {
                transform->SetPosition(ghostSpawnPos);
            }
            auto* bb = ghost->GetComponent<HEIN::CombatBlackBoard>();
            if (!bb)
            {
                bb = ghost->AddComponent<HEIN::CombatBlackBoard>();
            }
            if (bb)
            {
                bb->currentSpeed = 35.0f;
                bb->currentTurnSpeed = 12.0f;
                bb->spawnPosition = ghostSpawnPos;
                bb->hasSetSpawnPosition = true;
                bb->moveIntent = DirectX::SimpleMath::Vector3::Zero;
                bb->currentVelocity = DirectX::SimpleMath::Vector3::Zero;
            }
            if (!ghost->GetComponent<HEIN::CharacterMovementComponent>())
            {
                ghost->AddComponent<HEIN::CharacterMovementComponent>();
            }
            if (!ghost->GetComponent<HEIN::PlayerInputComponent>())
            {
                ghost->AddComponent<HEIN::PlayerInputComponent>(&m_actorManager);
            }
            auto* rb = ghost->GetComponent<HEIN::RigidBodyComponent>();
            if (!rb)
            {
                rb = ghost->AddComponent<HEIN::RigidBodyComponent>();
                rb->Initialize(80.0f, true, false);
            }
            if (rb)
            {
                rb->SetKinematic(false);
                rb->SetVelocity(DirectX::SimpleMath::Vector3::Zero);
                rb->SetNeedsInitialSnap(false);
            }
            if (!ghost->GetComponent<HEIN::CapsuleColliderComponent>())
            {
                auto* col = ghost->AddComponent<HEIN::CapsuleColliderComponent>();
                col->Initialize(3.0f, 12.0f);
                col->SetOffset(DirectX::SimpleMath::Vector3(0.0f, 90.0f, 0.0f));
                col->SetTrigger(false);
                col->SetCollisionLayer(HEIN::CollisionLayer::Layer_Player);
                col->SetCollisionMask(HEIN::CollisionLayer::Layer_Environment | HEIN::CollisionLayer::Layer_Ground | HEIN::CollisionLayer::Layer_Wall);
            }
            ghost->Start();
        }
        if (ghost != nullptr)
        {
            m_playerID = ghost->GetID();
            m_isGhost = true;

            // 3. Re-hook camera to track ghost
            if (auto* camera = GetActiveCameraController(gameContext))
            {
                SetupCameraModes(gameContext, camera, ghost->GetComponent<HEIN::SkinnedModelComponent>());
            }

            // 4. Enemy AI drops target (boss goes back to idle or spawn tether)
            if (HEIN::Actor* enemy = GetEnemyActor())
            {
                if (auto* bt = enemy->GetComponent<HEIN::BehaviourTreeComponent>())
                {
                    bt->SetTargetID(HEIN::INVALID_ACTOR_ID);
                }
            }
        }
    }

    m_actorManager.CleanUpDestroyedActors();
}

// --------------------------------------------------------------------------------------
// Render Phase Implementations
// --------------------------------------------------------------------------------------

void GameScene::RenderShadowPhase(GameContext& gameContext, ID3D11DeviceContext* context)
{
    HEIN::LightComponent* activeLight = nullptr;
    for (auto& pair : m_actorManager.GetAllActors())
    {
        activeLight = pair.second->GetComponent<HEIN::LightComponent>();
        if (activeLight && activeLight->CastsShadows()) break;
    }

    DirectX::SimpleMath::Vector3 lightDir(0.5f, -1.0f, 0.5f);
    DirectX::SimpleMath::Vector3 targetCenter = DirectX::SimpleMath::Vector3::Zero;
    HEIN::Actor* player = GetPlayerActor();
    if (player)
    {
        if (auto* trans = player->GetComponent<HEIN::TransformComponent>())
        {
            targetCenter = trans->GetPosition();
        }
    }
    DirectX::SimpleMath::Vector3 lightPos = targetCenter - lightDir * 1000.0f;
    float projSize = 500.0f;

    if (activeLight)
    {
        if (auto* lightOwner = activeLight->GetOwner())
        {
            if (auto* lightTransform = lightOwner->GetComponent<HEIN::TransformComponent>())
            {
                lightDir = lightTransform->GetForward();
                if (activeLight->GetLightType() == HEIN::LightType::Directional)
                {
                    lightPos = targetCenter - lightDir * 1000.0f;
                    projSize = (activeLight->GetRange() > 0.1f) ? activeLight->GetRange() : 500.0f;
                }
                else
                {
                    lightPos = lightTransform->GetPosition();
                    projSize = activeLight->GetRange();
                }
            }
        }
    }
    lightDir.Normalize();

    DirectX::SimpleMath::Vector3 up = DirectX::SimpleMath::Vector3::Up;
    if (std::abs(lightDir.Dot(up)) > 0.999f)
    {
        up = DirectX::SimpleMath::Vector3::Right;
    }

    DirectX::SimpleMath::Vector3 finalTargetCenter = targetCenter;

    if (activeLight == nullptr || activeLight->GetLightType() == HEIN::LightType::Directional)
    {
        // Option 2: World-locked texel snapping for Directional Light shadow map
        // 1. Establish orientation matrix centered at world origin
        DirectX::SimpleMath::Matrix lightViewOrigin = DirectX::SimpleMath::Matrix::CreateLookAt(
            DirectX::SimpleMath::Vector3::Zero,
            lightDir * 1000.0f,
            up
        );

        // 2. Project targetCenter (player) into orientation space
        DirectX::SimpleMath::Vector3 targetInLight = DirectX::SimpleMath::Vector3::Transform(targetCenter, lightViewOrigin);

        // 3. Snap to discrete texel multiples in world units
        const float shadowMapResolution = 2048.0f;
        float worldUnitsPerTexel = projSize / shadowMapResolution;
        targetInLight.x = std::floor(targetInLight.x / worldUnitsPerTexel) * worldUnitsPerTexel;
        targetInLight.y = std::floor(targetInLight.y / worldUnitsPerTexel) * worldUnitsPerTexel;

        // 4. Transform snapped position back to world space
        DirectX::SimpleMath::Matrix invLightView = lightViewOrigin.Invert();
        finalTargetCenter = DirectX::SimpleMath::Vector3::Transform(targetInLight, invLightView);
    }

    DirectX::SimpleMath::Vector3 finalLightPos = finalTargetCenter - lightDir * 1000.0f;
    DirectX::SimpleMath::Vector3 finalTargetPos = finalTargetCenter;

    DirectX::SimpleMath::Matrix viewMat = DirectX::SimpleMath::Matrix::CreateLookAt(finalLightPos, finalTargetPos, up);
    DirectX::SimpleMath::Matrix projMat = DirectX::SimpleMath::Matrix::CreateOrthographic(projSize, projSize, 0.1f, 3000.0f);

    DirectX::SimpleMath::Matrix lightViewProj = viewMat * projMat;
    m_shadowSystem.SetLightViewProj(lightViewProj);

    m_shadowSystem.BindShadowMap(context);
    m_actorManager.DrawAllShadows(gameContext, lightViewProj);
    m_shadowSystem.UnbindShadowMap(context);
}

void GameScene::RenderMainPassPhase(GameContext& gameContext, ID3D11DeviceContext* context, const DirectX::SimpleMath::Matrix& view)
{
    // Restore Main Render Target
    ID3D11RenderTargetView* rtv = gameContext.deviceResources.GetRenderTargetView();
    ID3D11DepthStencilView* dsv = gameContext.deviceResources.GetDepthStencilView();
    context->OMSetRenderTargets(1, &rtv, dsv);
    D3D11_VIEWPORT viewport = gameContext.deviceResources.GetScreenViewport();
    context->RSSetViewports(1, &viewport);

    if (m_skybox)
    {
        m_skybox->Draw(gameContext, view, m_proj);
    }

    // Bind Shadow Map SRV for shaders (slot 4)
    ID3D11ShaderResourceView* shadowSRV = m_shadowSystem.GetShadowMapSRV();
    context->PSSetShaderResources(4, 1, &shadowSRV);

    // Bind Comparison Sampler for shaders (slot 1)
    ID3D11SamplerState* shadowSampler = m_shadowSystem.GetShadowSampler();
    context->PSSetSamplers(1, 1, &shadowSampler);

    // Draw all active entities
    m_actorManager.DrawAll(gameContext, view, m_proj);

    // Unbind Shadow Map SRV and Sampler
    ID3D11ShaderResourceView* nullSRV = nullptr;
    context->PSSetShaderResources(4, 1, &nullSRV);

    ID3D11SamplerState* nullSampler = nullptr;
    context->PSSetSamplers(1, 1, &nullSampler);

    // Transparent Pipeline Setup & Reset to Opaque Defaults
    ID3D11SamplerState* wrapSampler = gameContext.commonStates.LinearWrap();
    context->RSSetState(gameContext.commonStates.CullNone());
    context->PSSetSamplers(0, 1, &wrapSampler);
    context->OMSetBlendState(gameContext.commonStates.AlphaBlend(), nullptr, 0xFFFFFFFF);
    context->OMSetDepthStencilState(gameContext.commonStates.DepthRead(), 0);

    context->RSSetState(gameContext.commonStates.CullCounterClockwise());
    context->OMSetBlendState(gameContext.commonStates.Opaque(), nullptr, 0xFFFFFFFF);
    context->OMSetDepthStencilState(gameContext.commonStates.DepthDefault(), 0);
}

// --------------------------------------------------------------------------------------
// Editor & Scene Management
// --------------------------------------------------------------------------------------

void GameScene::HandleEditorActions(GameContext& gameContext)
{
    HEIN::EditorAction uiAction = m_debugDisplay->GetUIAction();

    if (uiAction == HEIN::EditorAction::PlayPressed || !m_debugDisplay->isVisible())
    {
        m_isPlaying = true;
    }
    else if (uiAction == HEIN::EditorAction::StopPressed)
    {
        m_isPlaying = false;
    }
    else if (uiAction == HEIN::EditorAction::LoadPressed)
    {
        WCHAR szFile[260] = { 0 };
        OPENFILENAMEW ofn = { 0 };
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = gameContext.deviceResources.GetWindow();
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile) / sizeof(WCHAR);
        ofn.lpstrFilter = L"JSON Files\0*.json\0Scene Files\0*.Scene\0All Files\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

        if (GetOpenFileNameW(&ofn) == TRUE)
        {
            std::ifstream file(szFile);
            if (file.is_open())
            {
                try
                {
                    file.seekg(0, std::ios::end);
                    std::streampos fileSize = file.tellg();
                    file.seekg(0, std::ios::beg);

                    if (fileSize > 0)
                    {
                        nlohmann::json j;
                        file >> j;

                        gameContext.mainCamera = nullptr;
                        m_actorManager.Deserialize(j);
                        m_actorManager.InitializeAfterDeserialize(gameContext);

                        PostSceneLoadInit(gameContext);

                        UpdateDebugTargets();
                        m_isPlaying = true;
                    }
                }
                catch (const std::exception& e)
                {
                    OutputDebugStringA("Failed to parse scene JSON: ");
                    OutputDebugStringA(e.what());
                    OutputDebugStringA("\n");
                }
            }
        }
    }
    else if (uiAction == HEIN::EditorAction::NewScenePressed)
    {
        gameContext.mainCamera = nullptr;
        m_actorManager.ClearAllActors();
        m_playerID = HEIN::INVALID_ACTOR_ID;
        m_enemyID = HEIN::INVALID_ACTOR_ID;
        m_cameraID = HEIN::INVALID_ACTOR_ID;
        UpdateDebugTargets();
    }
    else if (uiAction == HEIN::EditorAction::SavePressed)
    {
        WCHAR szFile[260] = { 0 };
        OPENFILENAMEW ofn = { 0 };
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = gameContext.deviceResources.GetWindow();
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile) / sizeof(WCHAR);
        ofn.lpstrFilter = L"JSON Files\0*.json\0Scene Files\0*.Scene\0All Files\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
        ofn.lpstrDefExt = L"json";

        if (GetSaveFileNameW(&ofn) == TRUE)
        {
            nlohmann::json j = m_actorManager.Serialize();
            std::ofstream file(szFile);
            if (file.is_open())
            {
                file << j.dump(4);
            }
            // Save to primary GameScene.json
            std::string sceneDataPath = GetSceneDataWritePath();
            std::ofstream sceneFile(sceneDataPath);
            if (sceneFile.is_open())
            {
                sceneFile << j.dump(4);
            }
        }
    }
    else if (uiAction == HEIN::EditorAction::AutoSavePressed)
    {
        nlohmann::json j = m_actorManager.Serialize();

        // Save to primary GameScene.json
        std::string sceneDataPath = GetSceneDataWritePath();
        std::ofstream sceneFile(sceneDataPath);
        if (sceneFile.is_open())
        {
            sceneFile << j.dump(4);
        }
    }
    else if (uiAction == HEIN::EditorAction::CreateStagePressed)
    {
        HEIN::ActorID stageID = HEIN::ActorFactory::CreateStage(m_actorManager, gameContext);
        if (m_debugDisplay != nullptr && stageID != HEIN::INVALID_ACTOR_ID)
        {
            m_debugDisplay->GetDebugUI().SetSelectedActor(m_actorManager.GetActor(stageID));
        }
        UpdateDebugTargets();
    }
}

void GameScene::LoadSceneData(GameContext& gameContext)
{
    // Search candidates for primary GameScene.json
    std::vector<std::string> candidates = {
        "Resources/Data/GameScene.json",
        "Dual/Resources/Data/GameScene.json",
        "../Dual/Resources/Data/GameScene.json"
    };

    std::ifstream sceneFile;
    std::string loadedPath;
    for (const auto& path : candidates)
    {
        sceneFile.open(path);
        if (sceneFile.is_open())
        {
            loadedPath = path;
            break;
        }
    }
    if (sceneFile.is_open())
    {
        try
        {
            sceneFile.seekg(0, std::ios::end);
            std::streampos fileSize = sceneFile.tellg();
            sceneFile.seekg(0, std::ios::beg);

            if (fileSize > 0)
            {
                nlohmann::json j;
                sceneFile >> j;
                if (!j.empty())
                {
                    m_actorManager.ClearAllActors();
                    m_actorManager.Deserialize(j);
                    m_actorManager.InitializeAfterDeserialize(gameContext);
                }
            }
        }
        catch (const std::exception& e)
        {
            OutputDebugStringA(("Failed to parse scene JSON (" + loadedPath + "): ").c_str());
            OutputDebugStringA(e.what());
            OutputDebugStringA("\n");
        }
    }
}

void GameScene::UpdateDebugTargets()
{
    if (!m_debugDisplay) return;

    auto GetIDByName = [this](const std::wstring& name) -> HEIN::ActorID {
        auto* a = m_actorManager.GetActorByName(name);
        return a ? a->GetID() : HEIN::INVALID_ACTOR_ID;
    };

    m_debugDisplay->SetDebugTargets(
        GetIDByName(L"Player"),
        GetIDByName(L"Sword"),
        GetIDByName(L"Axe"),
        GetIDByName(L"StageRoot"),
        GetIDByName(L"Enemy")
    );
}

// --------------------------------------------------------------------------------------
// Entity & Component Accessors
// --------------------------------------------------------------------------------------

HEIN::Actor* GameScene::GetPlayerActor()
{
    HEIN::Actor* player = m_actorManager.GetActor(m_playerID);
    if (player && (player->GetActorType() == HEIN::ActorType::Player || m_isGhost))
    {
        return player;
    }

    // 1. Look for an actor explicitly marked as ActorType::Player
    for (const auto& pair : m_actorManager.GetAllActors())
    {
        if (pair.second && pair.second->GetActorType() == HEIN::ActorType::Player)
        {
            m_playerID = pair.first;
            return pair.second.get();
        }
    }

    // 2. Look for an actor named/tagged "Player"
    player = m_actorManager.GetActorByName(L"Player");
    if (player)
    {
        m_playerID = player->GetID();
        return player;
    }

    // 3. Fallback: If no player actor exists yet, select the first possessable vessel as starting body
    for (const auto& pair : m_actorManager.GetAllActors())
    {
        if (pair.second && pair.second->GetActorType() != HEIN::ActorType::Enemy && pair.second->GetActorType() != HEIN::ActorType::Ghost)
        {
            if (pair.second->GetComponent<HEIN::PossessableComponent>())
            {
                m_playerID = pair.first;
                return pair.second.get();
            }
        }
    }

    return nullptr;
}

HEIN::Actor* GameScene::GetEnemyActor()
{
    HEIN::Actor* enemy = m_actorManager.GetActor(m_enemyID);
    if (!enemy)
    {
        enemy = m_actorManager.GetActorByName(L"Enemy");
        if (enemy) m_enemyID = enemy->GetID();
    }
    return enemy;
}

HEIN::CameraController* GameScene::GetActiveCameraController(GameContext& gameContext)
{
    HEIN::Actor* cameraActor = m_actorManager.GetActor(m_cameraID);
    if (cameraActor != nullptr)
    {
        auto* cam = cameraActor->GetComponent<HEIN::CameraController>();
        if (cam != nullptr)
        {
            gameContext.mainCamera = cam;
            return cam;
        }
    }

    // Dynamic search for any entity equipped with a CameraController
    for (const auto& pair : m_actorManager.GetAllActors())
    {
        if (auto* cam = pair.second->GetComponent<HEIN::CameraController>())
        {
            m_cameraID = pair.first;
            gameContext.mainCamera = cam;
            return cam;
        }
    }

    gameContext.mainCamera = nullptr;
    return nullptr;
}

// --------------------------------------------------------------------------------------
// Camera Modes Registration
// --------------------------------------------------------------------------------------

void GameScene::SetupCameraModes(GameContext& gameContext, HEIN::CameraController* cameraComp, HEIN::SkinnedModelComponent* modelPointer)
{
    if (!cameraComp) return;

    gameContext.mainCamera = cameraComp;

    HEIN::Actor* player = GetPlayerActor();
    if (player != nullptr)
    {
        auto* playerTransform = player->GetComponent<HEIN::TransformComponent>();

        // Safe defensive null checks against uninitialized models/transforms
        if (playerTransform != nullptr)
        {
            DirectX::SimpleMath::Vector3 playerPos = playerTransform->GetPosition();
            m_targetPos = playerPos + DirectX::SimpleMath::Vector3(0.0f, 15.0f, 0.0f);

            if (modelPointer != nullptr)
            {
                modelPointer->Update(0.0f);
                DirectX::SimpleMath::Vector3 headPos = modelPointer->GetBoneWorldPosition(L"mixamorig:HeadTop_End", playerTransform->GetWorldMatrix());
                if (headPos != DirectX::SimpleMath::Vector3::Zero && (headPos - playerPos).LengthSquared() <= 10000.0f)
                {
                    m_targetPos = headPos;
                }
            }
        }

        cameraComp->ClearRegisteredCameras();

        cameraComp->RegisterCamera(
            HEIN::CameraType::Spring,
            [this]()
            {
                return std::make_unique<HEIN::SpringCameraMode>(
                    &m_actorManager, m_playerID, &m_targetPos);
            }
        );
    }

    cameraComp->SetFirstCamera(HEIN::CameraType::Spring);
}

void GameScene::PostSceneLoadInit(GameContext& gameContext)
{
    // Re-establish "Glue" Connections
    HEIN::Actor* player = GetPlayerActor();
    HEIN::Actor* enemy = GetEnemyActor();
    HEIN::CameraController* camera = GetActiveCameraController(gameContext);

    if (player != nullptr)
    {
        player->SetActorType(HEIN::ActorType::Player);
        m_playerID = player->GetID();

        m_isGhost = false;
        m_targetVesselID = HEIN::INVALID_ACTOR_ID;

        // If this actor is a possessable vessel, mark it as occupied
        if (auto* possessable = player->GetComponent<HEIN::PossessableComponent>())
        {
            possessable->SetOccupied(true);
        }

        // Ensure player's gauge is visible
        if (auto* gauge = player->GetComponent<HEIN::GaugeComponent>())
        {
            gauge->SetVisible(true);
        }

        // Any pre-placed Ghost actors in the level are kept dormant/invisible until player dies
        for (const auto& pair : m_actorManager.GetAllActors())
        {
            if (pair.second && pair.second->GetActorType() == HEIN::ActorType::Ghost)
            {
                pair.second->SetActive(false);
                pair.second->SetVisible(false);
                if (auto* rb = pair.second->GetComponent<HEIN::RigidBodyComponent>())
                {
                    rb->SetKinematic(true);
                    rb->SetVelocity(DirectX::SimpleMath::Vector3::Zero);
                }
            }
        }

        // Any unpossessed vessel in the level is put into dormant sleep mode (render only, shut other functions until possessed)
        for (const auto& pair : m_actorManager.GetAllActors())
        {
            HEIN::Actor* actor = pair.second.get();
            if (!actor || actor == player) continue;

            if (auto* possessable = actor->GetComponent<HEIN::PossessableComponent>())
            {
                if (!possessable->IsOccupied())
                {
                    // 1. Keep visible for rendering, but deactivate UI gauges
                    if (auto* gauge = actor->GetComponent<HEIN::GaugeComponent>())
                    {
                        gauge->SetVisible(false);
                    }

                    // 2. Disable expensive procedural foot IK
                    if (auto* procAnim = actor->GetComponent<HEIN::ProceduralAnimationComponent>())
                    {
                        procAnim->SetEnabled(false);
                    }

                    // 3. Put rigidbody into kinematic sleep so physics collisions don't tick
                    if (auto* rb = actor->GetComponent<HEIN::RigidBodyComponent>())
                    {
                        rb->SetKinematic(true);
                        rb->SetVelocity(DirectX::SimpleMath::Vector3::Zero);
                    }

                    // 4. Ensure vessel is invincible while dormant
                    if (auto* health = actor->GetComponent<HEIN::HealthComponent>())
                    {
                        health->SetGameplayInvincible(true);
                    }

                    // 5. Ensure dormant vessel is playing Idle animation
                    if (auto* model = actor->GetComponent<HEIN::SkinnedModelComponent>())
                    {
                        model->ChangeAnimation("Idle");
                    }
                }
            }
        }


        // Ensure runtime components exist for earlier saves
        if (!player->GetComponent<HEIN::CombatBlackBoard>())
            player->AddComponent<HEIN::CombatBlackBoard>();
        if (!player->GetComponent<HEIN::PlayerInputComponent>())
            player->AddComponent<HEIN::PlayerInputComponent>(&m_actorManager);
        if (!player->GetComponent<HEIN::CharacterMovementComponent>())
            player->AddComponent<HEIN::CharacterMovementComponent>();
    }

    if (enemy != nullptr)
    {
        enemy->SetActorType(HEIN::ActorType::Enemy);
        m_enemyID = enemy->GetID();

        if (!enemy->GetComponent<HEIN::CombatBlackBoard>())
        {
            auto* bb = enemy->AddComponent<HEIN::CombatBlackBoard>();
            if (auto* trans = enemy->GetComponent<HEIN::TransformComponent>())
            {
                bb->spawnPosition = trans->GetPosition();
                bb->hasSetSpawnPosition = true;
            }
        }
        if (!enemy->GetComponent<HEIN::CharacterMovementComponent>())
            enemy->AddComponent<HEIN::CharacterMovementComponent>();

        if (auto* model = enemy->GetComponent<HEIN::SkinnedModelComponent>())
        {
            model->SetEnableRootMotion(true);
        }

        if (player != nullptr)
        {
            HEIN::ActorFactory::SetupEnemyAI(enemy, &m_actorManager, player->GetID());
        }
    }

    // Re-link all socket attachment owners and child weapons dynamically across the scene
    for (const auto& pair : m_actorManager.GetAllActors())
    {
        HEIN::Actor* actor = pair.second.get();
        if (!actor) continue;

        if (auto* socketComp = actor->GetComponent<HEIN::SocketAttachmentComponent>())
        {
            if (!socketComp->GetOwnerTag().empty())
            {
                if (HEIN::Actor* targetOwner = m_actorManager.GetActorByName(socketComp->GetOwnerTag()))
                {
                    actor->SetOwnerID(targetOwner->GetID());
                    socketComp->Initialize(targetOwner->GetID(), socketComp->GetSocketName());
                }
            }
        }
        else if (actor->GetParentID() != HEIN::INVALID_ACTOR_ID)
        {
            actor->SetOwnerID(actor->GetParentID());
        }
    }

    if (HEIN::Actor* sword = m_actorManager.GetActorByName(L"Sword"))
    {
        if (player != nullptr) sword->SetOwnerID(player->GetID());
    }
    if (HEIN::Actor* axe = m_actorManager.GetActorByName(L"Axe"))
    {
        if (enemy != nullptr) axe->SetOwnerID(enemy->GetID());
    }

    // Ensure any actor with TerrainComponent also has a TerrainColliderComponent for physical collisions
    for (const auto& pair : m_actorManager.GetAllActors())
    {
        if (pair.second && pair.second->GetComponent<HEIN::TerrainComponent>())
        {
            if (!pair.second->GetComponent<HEIN::TerrainColliderComponent>())
            {
                auto* tc = pair.second->AddComponent<HEIN::TerrainColliderComponent>();
                tc->SetCollisionLayer(HEIN::CollisionLayer::Layer_Environment | HEIN::CollisionLayer::Layer_Ground);
                tc->SetCollisionMask(HEIN::CollisionLayer::Layer_All);
            }
        }
    }

    // Ensure Ghost actor has RigidBodyComponent and CapsuleColliderComponent to collide with TerrainComponent
    if (HEIN::Actor* ghostActor = m_actorManager.GetActorByName(L"Ghost"))
    {
        if (!ghostActor->GetComponent<HEIN::RigidBodyComponent>())
        {
            auto* rb = ghostActor->AddComponent<HEIN::RigidBodyComponent>();
            rb->Initialize(80.0f, true, false);
            rb->SetNeedsInitialSnap(false);
        }
        if (!ghostActor->GetComponent<HEIN::CapsuleColliderComponent>())
        {
            auto* col = ghostActor->AddComponent<HEIN::CapsuleColliderComponent>();
            col->Initialize(3.0f, 12.0f);
            col->SetOffset(DirectX::SimpleMath::Vector3(0.0f, 90.0f, 0.0f));
            col->SetTrigger(false);
            col->SetCollisionLayer(HEIN::CollisionLayer::Layer_Player);
            col->SetCollisionMask(HEIN::CollisionLayer::Layer_Environment | HEIN::CollisionLayer::Layer_Ground | HEIN::CollisionLayer::Layer_Wall);
        }
    }

    // Call Start() on all loaded actors so components initialize and subscribe to Messenger
    for (const auto& pair : m_actorManager.GetAllActors())
    {
        if (pair.second)
        {
            pair.second->Start();
        }
    }

    // Re-hook the camera modes to the loaded player model
    if (player != nullptr && camera != nullptr)
    {
        HEIN::SkinnedModelComponent* pModel = player->GetComponent<HEIN::SkinnedModelComponent>();
        SetupCameraModes(gameContext, camera, pModel);
    }
}
