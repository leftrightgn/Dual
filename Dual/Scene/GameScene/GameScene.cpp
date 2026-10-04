//--------------------------------------------------------------------------------------
// File: GameScene.cpp
// Author: HEIN SOE KHANT
//--------------------------------------------------------------------------------------
#include "pch.h"
#include "GameScene.h"
#include "../../../External/Engine/Common/InputManager.h"
#include "../../../External/Engine/Camera/DebugCameraMode.h"
#include "../../../External/Engine/Camera/CameraController.h"
#include "../../../External/Engine/Camera/ThirdPersonMode.h"
#include "../../../External/Engine/Camera/FirstPersonMode.h"
#include "../../../External/Engine/Camera/LockOnCameraMode.h"
#include "../../../External/Engine/Camera/SpringCameraMode.h"
#include "../../../External/Engine/Components/TransformComponent.h"
#include "../../../External/Engine/Components/SkinnedModelComponent.h"
#include "../../../External/Engine/Components/LightComponent.h"
#include "../../../External/Engine/FrameWork/GameContext.h"
#include <Components/PlayerInputComponent.h>
#include <Components/CombatStateMachineComponent.h>
#include <Factory/ActorFactory.h>
#include "../../../External/Engine/Components/HealthComponent.h"
#include "../../../External/Engine/ImGui/imgui.h"
#include "../../../External/Engine/ImGui/ImGuizmo.h"
#include "../../../External/Engine/Common/Event.h"
#include "../../../External/Engine/Common/json.hpp"
#include <commdlg.h>
#include <fstream>
#include <Windows.h>
#include <utility>
#include <cmath>

using namespace DirectX;

// --------------------------------------------------------------------------------------
// シーン切り替え時に呼び出される関数 (OnEnter)
// --------------------------------------------------------------------------------------
void GameScene::OnEnter(GameContext& gameContext)
{
    // Initialize Core Systems & Viewport Pipeline
    m_physicsSystem = std::make_unique<HEIN::PhysicsSystem>();
    m_damageSystem = std::make_unique<HEIN::DamageSystem>();

    m_skybox = std::make_unique<HEIN::Skybox>();
    m_skybox->Initialize(gameContext, L"Resources/Textures/skybox.dds");

    m_shadowSystem.Initialize(gameContext.deviceResources.GetD3DDevice(), 2048, 2048);

    D3D11_VIEWPORT viewport = gameContext.deviceResources.GetScreenViewport();
    float aspectRatio = static_cast<float>(viewport.Width) / static_cast<float>(viewport.Height);
    m_proj = SimpleMath::Matrix::CreatePerspectiveFieldOfView(DirectX::XM_PI / 4.0f, aspectRatio, 0.01f, 5000.0f);

    // Entity Spawning via Actor Factory
    m_cameraID = HEIN::ActorFactory::CreateMainCamera(m_actorManager);

    HEIN::PlayerSpawnData playerData = HEIN::ActorFactory::CreateKnight(m_actorManager, gameContext, &m_targetPos);
    m_playerID = playerData.playerID;
    HEIN::ActorFactory::CreateSword(m_actorManager, gameContext, m_playerID, 5.0f);

    HEIN::EnemySpawnData enemyData = HEIN::ActorFactory::CreateEnemy(m_actorManager, gameContext, m_playerID);
    m_enemyID = enemyData.enemyID;
    HEIN::ActorFactory::CreateAxe(m_actorManager, gameContext, m_enemyID, 20.0f);

    HEIN::ActorFactory::CreateStage(m_actorManager, gameContext);

    // Camera Controller & Modes Registration
    SetupCameraModes(gameContext, GetActiveCameraController(gameContext), playerData.tpsModel);

    // Debug Display Controller & Trigger Event Listeners
    m_debugDisplay = std::make_unique<HEIN::DebugDisplayController>();
    m_debugDisplay->Initialize();
    UpdateDebugTargets();

    gameContext.eventManager->AddTriggerListener([this](const HEIN::TriggerEventPayLoad& payLoad) {
        m_damageSystem->HandlTriggerHit(payLoad, m_actorManager);
    });

    // AutoSave Scene State Overlay
    LoadAutoSave(gameContext);
    UpdateDebugTargets();
}

// --------------------------------------------------------------------------------------
// 更新 (Update) - Orchestrates ECS Pipeline Phases
// --------------------------------------------------------------------------------------
void GameScene::Update(GameContext& gameContext)
{
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
    ProcessLifecyclePhase();
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

    // Pass 1: Shadow Map Depth Generation Pass
    RenderShadowPhase(gameContext, context);

    // Pass 2: Main Color & Scene Drawing Pass
    RenderMainPassPhase(gameContext, context, view);

    // Pass 3: ImGui Editor & Gizmo Overlay Pass
    m_debugDisplay->Render(gameContext, m_actorManager, m_skybox.get(), view, m_proj);
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
            cameraInput.mouseX = static_cast<float>(mouseState.x);
            cameraInput.mouseY = static_cast<float>(mouseState.y);
        }

        cameraInput.isLeftMouseDown = isUICapturingMouse ? false : mouseState.leftButton;
        cameraInput.scrollWheelDelta = static_cast<float>(mouseState.scrollWheelValue);
        cameraInput.ignoreScroll = isUICapturingMouse;
        cameraInput.ignoreMovement = isUICapturingKeyboard;

        camera->ProcessInput(cameraInput);

        HEIN::CameraType targetCameraType;
        if (gameContext.inputManager->WasCameraSwitchPressed(gameContext, targetCameraType))
        {
            camera->RequestSwitch(targetCameraType);
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
}

void GameScene::ProcessSimulationPhase(GameContext& gameContext, float deltaTime)
{
    if (m_isPlaying)
    {
        m_actorManager.UpdateAll(deltaTime);
        m_physicsSystem->UpdateMovement(gameContext, m_actorManager, deltaTime);
        m_actorManager.UpdateAllHierarchies(); // Math Cascades Downwards
        m_actorManager.LateUpdateAll(deltaTime);
        m_physicsSystem->UpdateCollisions(gameContext, m_actorManager, deltaTime);
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
        auto* pModel = player->GetComponent<HEIN::SkinnedModelComponent>();
        auto* pSM = player->GetComponent<HEIN::CombatStateMachineComponent>();

        if (pTransform != nullptr && pModel != nullptr)
        {
            DirectX::SimpleMath::Vector3 headPos = pModel->GetBoneWorldPosition(L"mixamorig:HeadTop_End", pTransform->GetWorldMatrix());
            float heightAboveRoot = headPos.y - pTransform->GetPosition().y;

            static float s_standingHeight = 15.0f;
            bool isDodging = (pSM && pSM->GetCurrentStateName() == "Dodge") || (heightAboveRoot < 6.0f);

            if (!isDodging && heightAboveRoot > 8.0f)
            {
                s_standingHeight = heightAboveRoot;
            }

            if (isDodging)
            {
                // When dodging, maintain camera target at stable standing height above root position
                headPos.y = pTransform->GetPosition().y + s_standingHeight;
            }

            m_targetPos = headPos;
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

void GameScene::ProcessLifecyclePhase()
{
    // Health & Death Evaluation
    for (const auto& pair : m_actorManager.GetAllActors())
    {
        HEIN::Actor* currentActor = pair.second.get();
        if (!currentActor) continue;

        auto* health = currentActor->GetComponent<HEIN::HealthComponent>();
        if (health != nullptr && health->isDead())
        {
            m_actorManager.DestroyID(currentActor->GetID());
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
    DirectX::SimpleMath::Vector3 lightPos(-500.0f, 1000.0f, -500.0f);
    float projSize = 150.0f;

    if (activeLight)
    {
        if (auto* lightOwner = activeLight->GetOwner())
        {
            if (auto* lightTransform = lightOwner->GetComponent<HEIN::TransformComponent>())
            {
                lightDir = lightTransform->GetForward();
                if (activeLight->GetLightType() == HEIN::LightType::Directional)
                {
                    HEIN::Actor* player = GetPlayerActor();
                    DirectX::SimpleMath::Vector3 targetCenter = DirectX::SimpleMath::Vector3::Zero;
                    if (player)
                    {
                        if (auto* trans = player->GetComponent<HEIN::TransformComponent>())
                        {
                            targetCenter = trans->GetPosition();
                        }
                    }
                    lightPos = targetCenter - lightDir * 500.0f;
                    projSize = (activeLight->GetRange() > 0.1f) ? activeLight->GetRange() : 150.0f;
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

    DirectX::SimpleMath::Vector3 targetPos = lightPos + lightDir * 500.0f;
    DirectX::SimpleMath::Vector3 up = DirectX::SimpleMath::Vector3::Up;
    if (std::abs(lightDir.Dot(up)) > 0.999f)
    {
        up = DirectX::SimpleMath::Vector3::Right;
    }

    DirectX::SimpleMath::Matrix viewMat = DirectX::SimpleMath::Matrix::CreateLookAt(lightPos, targetPos, up);
    DirectX::SimpleMath::Matrix projMat = DirectX::SimpleMath::Matrix::CreateOrthographic(projSize, projSize, 0.1f, 1000.0f);
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
                nlohmann::json j;
                file >> j;

                gameContext.mainCamera = nullptr;
                m_actorManager.Deserialize(j);
                m_actorManager.InitializeAfterDeserialize(gameContext);

                UpdateDebugTargets();
                m_isPlaying = true;
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
            std::ofstream file(szFile);
            if (file.is_open())
            {
                nlohmann::json j = m_actorManager.Serialize();
                file << j.dump(4);
            }
            std::ofstream autoSave("AutoSave.json");
            if (autoSave.is_open())
            {
                nlohmann::json j = m_actorManager.Serialize();
                autoSave << j.dump(4);
            }
        }
    }
    else if (uiAction == HEIN::EditorAction::AutoSavePressed)
    {
        std::ofstream autoSave("AutoSave.json");
        if (autoSave.is_open())
        {
            nlohmann::json j = m_actorManager.Serialize();
            autoSave << j.dump(4);
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

void GameScene::LoadAutoSave(GameContext& gameContext)
{
    std::ifstream autoSaveFile("AutoSave.json");
    if (autoSaveFile.is_open())
    {
        nlohmann::json j;
        autoSaveFile >> j;
        m_actorManager.Deserialize(j);
        m_actorManager.InitializeAfterDeserialize(gameContext);
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
    if (!player)
    {
        player = m_actorManager.GetActorByName(L"Player");
        if (player) m_playerID = player->GetID();
    }
    return player;
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
        if (modelPointer != nullptr && playerTransform != nullptr)
        {
            modelPointer->Update(0.0f);
            m_targetPos = modelPointer->GetBoneWorldPosition(L"mixamorig:HeadTop_End", playerTransform->GetWorldMatrix());
        }

        cameraComp->RegisterCamera(
            HEIN::CameraType::FirstPerson,
            [this, modelPointer]()
            {
                return std::make_unique<HEIN::FirstPersonMode>(
                    &m_actorManager, m_playerID, &m_targetPos, modelPointer, modelPointer);
            }
        );

        cameraComp->RegisterCamera(
            HEIN::CameraType::ThirdPerson,
            [this, modelPointer]()
            {
                return std::make_unique<HEIN::ThirdPersonMode>(
                    &m_actorManager, m_playerID, &m_targetPos, modelPointer, modelPointer);
            }
        );

        cameraComp->RegisterCamera(
            HEIN::CameraType::Spring,
            [this]()
            {
                return std::make_unique<HEIN::SpringCameraMode>(
                    &m_actorManager, m_playerID, &m_targetPos);
            }
        );

        cameraComp->RegisterCamera(
            HEIN::CameraType::LockOn,
            [this]()
            {
                return std::make_unique<HEIN::LockOnCameraMode>(
                    &m_actorManager, m_playerID, m_enemyID);
            }
        );
    }

    cameraComp->RegisterCamera(
        HEIN::CameraType::Debug,
        []() { return std::make_unique<HEIN::DebugCameraMode>(); }
    );

    cameraComp->SetFirstCamera(HEIN::CameraType::Spring);
}
