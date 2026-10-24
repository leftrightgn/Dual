//--------------------------------------------------------------------------------------



// File: GameScene.cpp



// Author: HEIN SOE KHANT



//--------------------------------------------------------------------------------------



#include "pch.h"



#include "GameScene.h"



#include "../../../External/Engine/Common/SkeletonProfile.h"



#include "../../../External/Engine/Common/InputManager.h"



#include "../../../External/Engine/Camera/CameraController.h"



#include "../../../External/Engine/Camera/SpringCameraMode.h"



#include "../../../External/Engine/Camera/LockOnCameraMode.h"



#include "../../../External/Engine/Camera/CinematicMode.h"



#include "../../../External/Engine/Components/TransformComponent.h"



#include "../../../External/Engine/Components/SkinnedModelComponent.h"



#include "../../../External/Engine/Components/LightComponent.h"



#include "../../../External/Engine/FrameWork/GameContext.h"



#include "../../../External/Engine/Scene/SceneManager.h"



#include "../../External/Engine/Components/UIButtonComponent.h"



#include <Components/PlayerInputComponent.h>



#include <Components/CombatStateMachineComponent.h>



#include <Components/ShockwaveEmitterComponent.h>



#include <Components/CharacterMovementComponent.h>



#include <Components/BehaviourTreeComponent.h>



#include <Components/PossessableComponent.h>



#include <BlackBoard/CombatBlackBoard.h>



#include <BlackBoard/PlayerBlackBoard.h>



#include "../../../External/Engine/Components/DamageDealerComponent.h"



#include "../../../External/Engine/Components/SocketAttachmentComponent.h"



#include "../../../External/Engine/Components/WeaponTrailComponent.h"



#include "../../../External/Engine/Message/Messenger.h"



#include "../../../External/Engine/Components/RigidBodyComponent.h"



#include "../../../External/Engine/Components/ColliderComponent/CapsuleColliderComponent.h"



#include "../../../External/Engine/Components/ColliderComponent/AABBColliderComponent.h"



#include "../../../External/Engine/Components/ColliderComponent/TerrainColliderComponent.h"



#include "../../../External/Engine/Components/ColliderComponent/OBBColliderComponent.h"



#include "../../../External/Engine/Components/ColliderComponent/SphereColliderComponent.h" 



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



    // Safeguard level templates from runtime gameplay modifications (e.g. possessed vessel transformed into player)



    void SanitizeActorsForSave(HEIN::ActorManager& actorManager)



    {

        // Retain user-customized UI banners and buttons in scene data and preserve authoring visibility for editor preview





        for (const auto& pair : actorManager.GetAllActors())



        {



            HEIN::Actor* actor = pair.second.get();



            if (!actor) continue;



            // If this actor is a possessable vessel, ensure it is serialized in its clean dormant template state



            if (auto* possessable = actor->GetComponent<HEIN::PossessableComponent>())



            {



                // Restore dormant vessel identity



                actor->SetActorType(HEIN::ActorType::Default);



                actor->SetTag(L"vessel");



                possessable->SetOccupied(false);



                if (auto* health = actor->GetComponent<HEIN::HealthComponent>())



                {



                    health->SetCurrentHealth(health->GetMaxHealth());



                    health->SetGameplayInvincible(false);



                }



                if (auto* gauge = actor->GetComponent<HEIN::GaugeComponent>())



                {



                    gauge->SetVisible(false);



                }



                // If player components were attached during runtime possession, restore base vessel components



                if (auto* input = actor->GetComponent<HEIN::PlayerInputComponent>())



                {



                    actor->RemoveComponent(input);



                }



                if (auto* pbb = actor->GetComponent<HEIN::PlayerBlackBoard>())



                {



                    actor->RemoveComponent(pbb);



                    if (!actor->GetComponent<HEIN::CombatBlackBoard>())



                    {



                        actor->AddComponent<HEIN::CombatBlackBoard>();



                    }



                }



                // Restore weapon child owner tag



                for (HEIN::ActorID childID : actor->GetChildren())



                {



                    if (HEIN::Actor* child = actorManager.GetActor(childID))



                    {



                        if (auto* socket = child->GetComponent<HEIN::SocketAttachmentComponent>())



                        {



                            socket->SetOwnerTag(L"vessel");



                        }



                    }



                }



            }



            else if (actor->GetActorType() == HEIN::ActorType::Enemy || actor->GetActorType() == HEIN::ActorType::Player)



            {



                // Ensure health is restored to full for level template saves



                if (auto* health = actor->GetComponent<HEIN::HealthComponent>())



                {



                    health->SetCurrentHealth(health->GetMaxHealth());



                }



            }



        }



    }



}



using namespace DirectX;



// --------------------------------------------------------------------------------------



// Lifecycle Transition: OnEnter



// --------------------------------------------------------------------------------------



void GameScene::OnEnter(GameContext& gameContext)



{



    // Clear trigger listeners to ensure fresh scene isolation and eliminate dangling lambdas



    if (gameContext.eventManager)



    {



        gameContext.eventManager->ClearTriggerListeners();



    }



    HEIN::Messenger::DestroyInstance();



    // Clean all previous actors to guarantee scene isolation from TitleScene or earlier runs



    m_actorManager.ClearAllActors();



    gameContext.actorManager = &m_actorManager;



    gameContext.shadowSystem = &m_shadowSystem;



    // Reset all gameplay state flags



    m_isPlaying = true;



    m_isGhost = false;



    m_isDying = false;



    m_deathFreezeTimer = 0.0f;



    m_isDeathAnimPlaying = false;



    m_deathAnimTimer = 0.0f;



    m_flowState = GameFlowState::Gameplay;



    m_flowTimer = 0.0f;



    m_hasPlayedIntro = false;



    m_retryButtonID = HEIN::INVALID_ACTOR_ID;



    m_gameOverBannerID = HEIN::INVALID_ACTOR_ID;







    m_gameManagerID = HEIN::INVALID_ACTOR_ID;



    m_isVictory = false;



    m_victoryTimer = 0.0f;



    m_victoryBannerID = HEIN::INVALID_ACTOR_ID;



    m_backToTitleButtonID = HEIN::INVALID_ACTOR_ID;



    m_playerID = HEIN::INVALID_ACTOR_ID;



    m_enemyID = HEIN::INVALID_ACTOR_ID;



    m_cameraID = HEIN::INVALID_ACTOR_ID;



    m_targetVesselID = HEIN::INVALID_ACTOR_ID;



    // Initialize Core Systems & Viewport Pipeline



    m_physicsSystem = std::make_unique<HEIN::PhysicsSystem>();



    m_damageSystem = std::make_unique<HEIN::DamageSystem>();



    m_skybox = std::make_unique<HEIN::Skybox>();



    m_skybox->Initialize(gameContext, L"Resources/Textures/cloudyskybox.dds");



    m_shadowSystem.Initialize(gameContext.deviceResources.GetD3DDevice(), GameConstants::Shadow::DefaultMapWidth, GameConstants::Shadow::DefaultMapHeight);



    D3D11_VIEWPORT viewport = gameContext.deviceResources.GetScreenViewport();



    float aspectRatio = static_cast<float>(viewport.Width) / static_cast<float>(viewport.Height);



    m_proj = SimpleMath::Matrix::CreatePerspectiveFieldOfView(DirectX::XM_PI / 4.0f, aspectRatio, GameConstants::Camera::NearClipPlane, GameConstants::Camera::FarClipPlane);



    // Register all game-level components into ComponentFactory reflection registry



    HEIN::ActorFactory::RegisterGameComponents();



    // Load the Scene Data from JSON (Prioritizes GameScene.json, falls back to AutoSave.json)



    LoadSceneData(gameContext);



    // Re-establish "Glue" Connections, AI, weapons, Start(), and camera modes



    PostSceneLoadInit(gameContext);



    // Editor starts completely unloaded in Game Mode (press F3 to open Editor on demand)



    m_debugDisplay.reset();



    gameContext.isEditorMode = false;



    gameContext.showColliders = false;



    gameContext.showUI = true;

    // In gameplay mode, keep mouse cursor hidden and relative
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    ImGui::SetMouseCursor(ImGuiMouseCursor_None);
    ::SetCursor(nullptr);
    DirectX::Mouse::Get().SetMode(DirectX::Mouse::MODE_RELATIVE);
    while (ShowCursor(FALSE) >= 0);



    // Register safe trigger listener for current scene instance



    gameContext.eventManager->AddTriggerListener([this](const HEIN::TriggerEventPayLoad& payLoad) {



        if (m_damageSystem)



        {



            m_damageSystem->HandlTriggerHit(payLoad, m_actorManager);



        }



    });



    if (HEIN::CameraController* camera = GetActiveCameraController(gameContext))



    {



        camera->UpdateMouseMode();



    }



}



// --------------------------------------------------------------------------------------



// Lifecycle Transition: OnExit



// --------------------------------------------------------------------------------------



void GameScene::OnExit(GameContext& gameContext)



{



    // Auto-save on exit ONLY if user was actively in editor mode, not during live gameplay playtests



    if (m_debugDisplay != nullptr && !m_isPlaying)



    {



        SanitizeActorsForSave(m_actorManager);



        nlohmann::json j = m_actorManager.Serialize();



        std::ofstream sceneFile(GetSceneDataWritePath());



        if (sceneFile.is_open())



        {



            sceneFile << j.dump(4);



        }



    }



    // Unsubscribe and clear event listeners to prevent dangling callbacks across scenes



    if (gameContext.eventManager)



    {



        gameContext.eventManager->ClearTriggerListeners();



    }



    HEIN::Messenger::DestroyInstance();



    UnloadEditor(gameContext);



    m_actorManager.ClearAllActors();



    m_hitboxDebugScope = HitboxDebugScope::Off;



    m_skybox.reset();



    m_physicsSystem.reset();



    m_damageSystem.reset();



    m_shadowSystem.Release();



    gameContext.actorManager = nullptr;



    gameContext.shadowSystem = nullptr;



    gameContext.mainCamera = nullptr;



    ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
    ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
    ::SetCursor(::LoadCursor(nullptr, IDC_ARROW));
    DirectX::Mouse::Get().SetMode(DirectX::Mouse::MODE_ABSOLUTE);
    while (ShowCursor(TRUE) < 0);



    if (ID3D11DeviceContext* context = gameContext.deviceResources.GetD3DDeviceContext())



    {



        context->ClearState();



        context->Flush();



    }



}



// --------------------------------------------------------------------------------------



// Update - Orchestrates ECS Pipeline Phases



// --------------------------------------------------------------------------------------



void GameScene::Update(GameContext& gameContext)



{



    gameContext.actorManager = &m_actorManager;



    gameContext.shadowSystem = &m_shadowSystem;



    float deltaTime = static_cast<float>(gameContext.timer.GetElapsedSeconds());



    // Editor & Tooling Phase (Lazy-loaded on F3, completely unloaded on Play/Close)



    if (m_debugDisplay == nullptr)



    {



        if (HEIN::InputManager::WasActionPressed(gameContext, HEIN::InputAction::EditorToggle))



        {



            OpenEditor();



        }



    }



    if (m_debugDisplay != nullptr)



    {



        gameContext.isEditorMode = true;



        gameContext.showUI = m_debugDisplay->GetDebugUI().IsShowUIEnabled();



        m_debugDisplay->Update(gameContext, m_actorManager);



        HandleEditorActions(gameContext);



    }



    else



    {



        gameContext.isEditorMode = false;



        gameContext.showColliders = false;



        gameContext.showUI = true;



    }



    if (!m_isPlaying)



    {



        deltaTime = 0.0f;



    }



    // Input Phase



    ProcessInputPhase(gameContext);



    // Simulation Phase (Physics, Hierarchies, LateUpdate, Collisions, Tracking)



    ProcessSimulationPhase(gameContext, deltaTime);



    // Lifecycle Phase (Garbage Collection / Death System)



    ProcessLifecyclePhase(gameContext, deltaTime);



}



// --------------------------------------------------------------------------------------



// Render - Orchestrates Rendering Passes



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



    bool isEditorVisible = (m_debugDisplay != nullptr && m_debugDisplay->isVisible());



    gameContext.actorManager = &m_actorManager;



    gameContext.shadowSystem = &m_shadowSystem;



    gameContext.isEditorMode = (!m_isPlaying || isEditorVisible);



    // Shadow Map Depth Generation Pass



    RenderShadowPhase(gameContext, context);



    if (isEditorVisible)



    {



        // In Editor Mode: DebugDisplayController orchestrates SceneViewOnly, GameViewOnly, or SplitSideBySide



        m_debugDisplay->Render(gameContext, m_actorManager, m_skybox.get(), view, m_proj);



    }



    else



    {



        // In Full-Screen Game Mode:



        gameContext.isSceneViewPass = false;



        gameContext.showUI = (m_debugDisplay != nullptr) ? m_debugDisplay->GetDebugUI().IsShowUIEnabled() : true;



        gameContext.showColliders = false;



        RenderMainPassPhase(gameContext, context, view);



    }



    // Ghost Possession Prompt Overlay



    if (gameContext.showUI && !gameContext.isSceneViewPass && m_isGhost && m_targetVesselID != HEIN::INVALID_ACTOR_ID)



    {



        D3D11_VIEWPORT vp = isEditorVisible



            ? m_debugDisplay->GetDebugUI().GetGameViewport(gameContext)



            : gameContext.deviceResources.GetScreenViewport();



        if (!isEditorVisible || m_debugDisplay->GetDebugUI().GetViewportLayout() != EditorViewportLayout::SceneViewOnly)



        {



            float uiScale = std::min(vp.Width / 1280.0f, vp.Height / 720.0f);



            ImGui::SetNextWindowPos(ImVec2(vp.TopLeftX + vp.Width * 0.5f, vp.TopLeftY + vp.Height * 0.78f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));



            ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs;



            if (ImGui::Begin("PossessOverlayPrompt", nullptr, flags))



            {



                ImGui::SetWindowFontScale(std::max(0.5f, 1.8f * uiScale));



                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.8f, 1.0f), "[Q] POSSESS VESSEL");



                ImGui::End();



            }



        }



    }



    // Victory banner and return button are managed directly by scene UI actors (VictoryBanner and BackToTitleButton)



// Hitbox Debugger Badge Overlay



    if (gameContext.showUI && !gameContext.isSceneViewPass && m_hitboxDebugScope != HitboxDebugScope::Off)



    {



        D3D11_VIEWPORT vp = isEditorVisible



            ? m_debugDisplay->GetDebugUI().GetGameViewport(gameContext)



            : gameContext.deviceResources.GetScreenViewport();



        if (!isEditorVisible || m_debugDisplay->GetDebugUI().GetViewportLayout() != EditorViewportLayout::SceneViewOnly)



        {



            float uiScale = std::min(vp.Width / 1280.0f, vp.Height / 720.0f);



            ImGui::SetNextWindowPos(ImVec2(vp.TopLeftX + 24.0f, vp.TopLeftY + 24.0f), ImGuiCond_Always);



            ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs;



            if (ImGui::Begin("HitboxDebugBadge", nullptr, flags))



            {



                ImGui::SetWindowFontScale(std::max(0.5f, 1.3f * uiScale));



                if (m_hitboxDebugScope == HitboxDebugScope::ActiveAttacksOnly)



                {



                    ImGui::TextColored(ImVec4(1.0f, 0.55f, 0.0f, 0.95f), "[F1] Hitbox Scope: ACTIVE ATTACKS ONLY");



                }



                else if (m_hitboxDebugScope == HitboxDebugScope::AllColliders)



                {



                    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 0.95f), "[F1] Hitbox Scope: ALL COLLIDERS");

                }

                // - Display active or nearest encounter trigger status (Hybrid Mode)
                bool displayedEncounter = false;
                if (auto* player = GetPlayerActor())
                {
                    if (auto* pTrans = player->GetComponent<HEIN::TransformComponent>())
                    {
                        float minDistance = 1e9f;
                        HEIN::EncounterTriggerComponent* bestEnc = nullptr;
                        std::wstring actorName = L"";

                        for (const auto& [id, actor] : m_actorManager.GetAllActors())
                        {
                            if (!actor || !actor->IsActive()) continue;
                            if (auto* enc = actor->GetComponent<HEIN::EncounterTriggerComponent>())
                            {
                                if (auto* aTrans = actor->GetComponent<HEIN::TransformComponent>())
                                {
                                    DirectX::SimpleMath::Vector3 diff = pTrans->GetPosition() - aTrans->GetPosition();
                                    diff.y = 0.0f;
                                    float dist = diff.Length();
                                    if (dist < minDistance)
                                    {
                                        minDistance = dist;
                                        bestEnc = enc;
                                        actorName = actor->GetTag();
                                    }
                                }
                            }
                        }

                        if (bestEnc != nullptr)
                        {
                            displayedEncounter = true;
                            float triggerDist = bestEnc->GetTriggerDistance();
                            bool inRange = (minDistance <= triggerDist);
                            ImVec4 color = bestEnc->HasTriggered()
                                ? ImVec4(0.7f, 0.7f, 0.7f, 0.85f)
                                : (inRange ? ImVec4(1.0f, 0.35f, 0.35f, 0.95f) : ImVec4(0.25f, 0.9f, 1.0f, 0.95f));

                            std::string nameStr(actorName.begin(), actorName.end());
                            ImGui::TextColored(color, "Encounter [%s]: Range %.1fm | LockOn %.1fm | Dist: %.1fm [%s]", nameStr.c_str(), triggerDist, bestEnc->GetLockOnDistance(), minDistance, bestEnc->HasTriggered() ? "TRIGGERED" : (inRange ? "IN RANGE" : "OUT OF RANGE"));
                        }
                    }
                }

                if (!displayedEncounter)
                {
                    if (auto* flowComp = GetGameFlowComponent())
                    {
                        if (flowComp->IsIntroCinematicEnabled())
                        {
                            float triggerDist = flowComp->GetBossIntroTriggerDistance();
                            float currentDist = -1.0f;
                            if (auto* player = GetPlayerActor())
                            {
                                if (auto* enemy = GetEnemyActor())
                                {
                                    auto* pTrans = player->GetComponent<HEIN::TransformComponent>();
                                    auto* eTrans = enemy->GetComponent<HEIN::TransformComponent>();
                                    if (pTrans && eTrans)
                                    {
                                        DirectX::SimpleMath::Vector3 diff = pTrans->GetPosition() - eTrans->GetPosition();
                                        diff.y = 0.0f;
                                        currentDist = diff.Length();
                                    }
                                }
                            }

                            if (currentDist >= 0.0f)
                            {
                                if (!m_hasPlayedIntro)
                                {
                                    bool inRange = (currentDist <= triggerDist);
                                    ImVec4 color = inRange ? ImVec4(1.0f, 0.35f, 0.35f, 0.95f) : ImVec4(0.25f, 0.9f, 1.0f, 0.95f);
                                    ImGui::TextColored(color, "Intro Encounter Range: %.1fm | Current Dist: %.1fm [%s]",
                                        triggerDist, currentDist, inRange ? "IN RANGE" : "OUT OF RANGE");
                                }
                                else
                                {
                                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 0.85f), "Intro Encounter Range: %.1fm | Current Dist: %.1fm [COMPLETED]",
                                        triggerDist, currentDist);
                                }
                            }
                        }
                    }
                }



                ImGui::End();



            }



        }



    }



}



// --------------------------------------------------------------------------------------



// ECS Pipeline Phase Implementations



// --------------------------------------------------------------------------------------



void GameScene::ProcessInputPhase(GameContext& gameContext)



{



    if (HEIN::InputManager::WasActionPressed(gameContext, HEIN::InputAction::HitboxDebugToggle))



    {



        int nextMode = (static_cast<int>(m_hitboxDebugScope) + 1) % 3;



        m_hitboxDebugScope = static_cast<HitboxDebugScope>(nextMode);



    }



    // Debug Cheat Key: F10 to immediately defeat enemy and trigger victory flow for testing

    if (ImGui::IsKeyPressed(ImGuiKey_F10))

    {

        if (HEIN::Actor* enemy = GetEnemyActor())

        {

            if (auto* health = enemy->GetComponent<HEIN::HealthComponent>())

            {

                health->SetCurrentHealth(0.0f);

            }

        }

    }







    if (HEIN::InputManager::WasActionPressed(gameContext, HEIN::InputAction::Escape))



    {



        if (gameContext.sceneManager)



        {



            gameContext.sceneManager->LoadScene("TitleScene");



            return;



        }



    }



    HEIN::CameraController* camera = GetActiveCameraController(gameContext);



    HEIN::Actor* player = GetPlayerActor();



    bool isUICapturingMouse = ImGui::GetIO().WantCaptureMouse || ImGuizmo::IsUsing() || ImGuizmo::IsOver();



    bool isUICapturingKeyboard = ImGui::GetIO().WantCaptureKeyboard;



    bool isSceneView = (m_debugDisplay != nullptr && m_debugDisplay->isVisible() && m_debugDisplay->isMagnified());



    // Intro Cutscene Input: allow player to skip intro cutscene with Space, Enter, or Escape

    if (m_flowState == GameFlowState::Intro)

    {

        if (ImGui::IsKeyPressed(ImGuiKey_Space) || ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_Escape) ||

            HEIN::InputManager::WasActionPressed(gameContext, HEIN::InputAction::Jump) ||

            HEIN::InputManager::WasActionPressed(gameContext, HEIN::InputAction::Possess))

        {

            ChangeFlowState(GameFlowState::Gameplay, gameContext);

        }

    }



    // Cursor Visibility & Mouse Mode Management:
    // Cursor should ONLY appear at Victory ("YOU WIN"), Game Over, Title Scene, or Editor mode.
    // In normal gameplay mode, cursor is completely hidden and mouse operates in relative mode.
    bool isEditorActive = (m_debugDisplay != nullptr && m_debugDisplay->isVisible());
    if (m_flowState == GameFlowState::Victory || m_flowState == GameFlowState::GameOver || m_isVictory || isEditorActive)
    {
        ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
        ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
        ::SetCursor(::LoadCursor(nullptr, IDC_ARROW));
        DirectX::Mouse::Get().SetMode(DirectX::Mouse::MODE_ABSOLUTE);
        while (ShowCursor(TRUE) < 0);
    }
    else
    {
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
        ImGui::SetMouseCursor(ImGuiMouseCursor_None);
        ::SetCursor(nullptr);
        while (ShowCursor(FALSE) >= 0);
        if (camera != nullptr)
        {
            camera->UpdateMouseMode();
        }
        else
        {
            DirectX::Mouse::Get().SetMode(DirectX::Mouse::MODE_RELATIVE);
        }
    }



    // Camera Input Processing

    if (!isSceneView && camera != nullptr)

    {

        if (m_flowState == GameFlowState::Intro || m_flowState == GameFlowState::Victory || m_flowState == GameFlowState::GameOver || m_isVictory)

        {

            HEIN::CameraInputState cameraInput;

            cameraInput.mouseX = 0.0f;

            cameraInput.mouseY = 0.0f;

            cameraInput.isLeftMouseDown = false;

            cameraInput.scrollWheelDelta = 0.0f;

            cameraInput.ignoreScroll = true;

            cameraInput.ignoreMovement = true;

            camera->ProcessInput(cameraInput);

        }

        else

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
            bool cameraSwitchRequested = gameContext.inputManager->WasCameraSwitchPressed(gameContext, targetCameraType);

            // Support Tab key or Middle Mouse click for LockOn toggle
            if (!cameraSwitchRequested && (gameContext.keyboardTracker.pressed.Tab ||
                gameContext.mouseButtonTracker.middleButton == DirectX::Mouse::ButtonStateTracker::PRESSED))
            {
                if (!m_isGhost && (!player || player->GetActorType() != HEIN::ActorType::Ghost))
                {
                    targetCameraType = HEIN::CameraType::LockOn;
                    cameraSwitchRequested = true;
                }
            }


            if (cameraSwitchRequested)
            {
                if (targetCameraType == HEIN::CameraType::Spring)
                {
                    camera->RequestSwitch(targetCameraType);
                }
                else if (targetCameraType == HEIN::CameraType::LockOn)
                {
                    if (camera->GetCurrentCameraType() == HEIN::CameraType::LockOn)
                    {
                        camera->RequestSwitch(HEIN::CameraType::Spring);
                    }
                    else if (!m_isGhost && (!player || player->GetActorType() != HEIN::ActorType::Ghost))
                    {
                        camera->RequestSwitch(HEIN::CameraType::LockOn);
                    }
                }
                else
                {
                    camera->RequestSwitch(targetCameraType);
                }
            }
        }
    }



    // Player Input Processing
    if (m_isPlaying && player != nullptr && !m_isDying && m_flowState == GameFlowState::Gameplay && !m_isVictory && !isSceneView && !isUICapturingMouse && !isUICapturingKeyboard)
    {
        gameContext.inputManager->BroadCastPlayerInput(gameContext, player->GetID());
        if (auto* inputComp = player->GetComponent<HEIN::PlayerInputComponent>())
        {
            inputComp->ProcessInput(gameContext);
        }

        // - If player attacks, auto-engage Lock-On camera targeting nearby encounter or enemy actor
        if (!m_isGhost && player->GetActorType() != HEIN::ActorType::Ghost && HEIN::InputManager::WasActionPressed(gameContext, HEIN::InputAction::Attack))
        {
            if (camera != nullptr)
            {
                if (camera->GetCurrentCameraType() != HEIN::CameraType::LockOn)
                {
                    if (auto* pTrans = player->GetComponent<HEIN::TransformComponent>())
                    {
                        for (const auto& [id, actor] : m_actorManager.GetAllActors())
                        {
                            if (!actor || !actor->IsActive() || actor.get() == player) continue;
                            if (auto* hp = actor->GetComponent<HEIN::HealthComponent>())
                            {
                                if (hp->isDead()) continue;
                            }
                            if (auto* eTrans = actor->GetComponent<HEIN::TransformComponent>())
                            {
                                DirectX::SimpleMath::Vector3 diff = pTrans->GetPosition() - eTrans->GetPosition();
                                diff.y = 0.0f;
                                if (diff.LengthSquared() <= 3600.0f) // 60 meters
                                {
                                    camera->RequestSwitch(HEIN::CameraType::LockOn);
                                    camera->ApplyRequest();
                                    if (auto* lockOn = dynamic_cast<HEIN::LockOnCameraMode*>(camera->GetCurrentCameraMode()))
                                    {
                                        lockOn->SetTargetID(id);
                                    }
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    else if (player != nullptr && m_flowState == GameFlowState::Intro)
    {
        // - Firmly hold player stationary in Idle pose during intro cutscene
        if (auto* bb = player->GetComponent<HEIN::CombatBlackBoard>())
        {
            bb->moveIntent = DirectX::SimpleMath::Vector3::Zero;
            bb->localMoveIntent = DirectX::SimpleMath::Vector3::Zero;
            bb->currentVelocity = DirectX::SimpleMath::Vector3::Zero;
        }
        if (auto* rb = player->GetComponent<HEIN::RigidBodyComponent>())
        {
            rb->SetVelocity(DirectX::SimpleMath::Vector3::Zero);
        }
        if (auto* cm = player->GetComponent<HEIN::CharacterMovementComponent>())
        {
            cm->ClearImpulse();
        }
        if (auto* model = player->GetComponent<HEIN::SkinnedModelComponent>())
        {
            if (model->HasAnimation("Idle"))
            {
                model->ChangeAnimation("Idle");
            }
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



                if (HEIN::InputManager::WasActionPressed(gameContext, HEIN::InputAction::Possess))



                {



                    // Deactivate and hide Ghost Actor (preserve in memory so it can return if the possessed vessel dies)



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



                    // Possess the Vessel



                    HEIN::ActorFactory::PossessVessel(closestVessel, m_actorManager, &gameContext);



                    m_playerID = closestVessel->GetID();



                    m_isGhost = false;



                    m_isDying = false;



                    m_deathFreezeTimer = 0.0f;



                    m_targetVesselID = HEIN::INVALID_ACTOR_ID;



                    // Re-hook Camera to newly possessed vessel



                    if (camera != nullptr)



                    {



                        SetupCameraModes(gameContext, camera, closestVessel->GetComponent<HEIN::SkinnedModelComponent>());



                    }



                    // Re-hook Boss AI targeting



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



        // Game Flow State: Intro Sequence updates

        if (m_flowState == GameFlowState::Intro)
        {
            m_flowTimer += deltaTime;
            if (auto* enc = GetActiveEncounterComponent())
            {
                enc->FreezeEncounterActors(gameContext);
            }
            else if (auto* flowComp = GetGameFlowComponent())
            {
                flowComp->FreezeBossAndPlayer(gameContext);
            }

            if (auto* camera = GetActiveCameraController(gameContext))
            {
                if (auto* cinematic = dynamic_cast<HEIN::CinematicMode*>(camera->GetCurrentCameraMode()))
                {
                    float maxDur = 8.5f;
                    if (auto* enc = GetActiveEncounterComponent())
                    {
                        maxDur = enc->GetMaxDuration();
                    }
                    else if (auto* flowComp = GetGameFlowComponent())
                    {
                        maxDur = flowComp->GetIntroMaxDuration();
                    }

                    if (cinematic->IsFinished() || m_flowTimer >= maxDur)
                    {
                        if (auto* enc = GetActiveEncounterComponent())
                        {
                            enc->EndEncounter(gameContext);
                        }
                        ChangeFlowState(GameFlowState::Gameplay, gameContext);
                    }
                }
            }
        }
        else if (m_flowState == GameFlowState::Gameplay)
        {
            // - Update EncounterTriggerComponent states (AI wake-up delay and unfreeze synchronization)
            for (const auto& [id, actor] : m_actorManager.GetAllActors())
            {
                if (!actor || !actor->IsActive()) continue;
                if (auto* enc = actor->GetComponent<HEIN::EncounterTriggerComponent>())
                {
                    enc->UpdateEncounter(deltaTime, gameContext);
                }
            }

            // - Hybrid Mode: evaluate actors with EncounterTriggerComponent (supports Boss1, Boss2, etc.)
            bool triggeredEncounter = false;
            if (auto* player = GetPlayerActor())
            {
                if (!m_isGhost && player->GetActorType() != HEIN::ActorType::Ghost)
                {
                    if (auto* pTrans = player->GetComponent<HEIN::TransformComponent>())
                    {
                        for (const auto& [id, actor] : m_actorManager.GetAllActors())
                        {
                            if (!actor || !actor->IsActive()) continue;
                            if (auto* enc = actor->GetComponent<HEIN::EncounterTriggerComponent>())
                            {
                                if (enc->IsEnabled() && !enc->HasTriggered())
                                {
                                    if (enc->CheckAndTrigger(gameContext, pTrans->GetPosition()))
                                    {
                                        m_activeEncounterActorID = id;
                                        m_flowState = GameFlowState::Intro;
                                        m_flowTimer = 0.0f;
                                        m_hasPlayedIntro = true;
                                        triggeredEncounter = true;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // - Check proximity Lock-On transition for active/engaged encounters
            if (auto* player = GetPlayerActor())
            {
                if (!m_isGhost && player->GetActorType() != HEIN::ActorType::Ghost)
                {
                    if (auto* pTrans = player->GetComponent<HEIN::TransformComponent>())
                    {
                        if (auto* camera = GetActiveCameraController(gameContext))
                        {
                            if (camera->GetCurrentCameraType() != HEIN::CameraType::LockOn)
                            {
                                for (const auto& [id, actor] : m_actorManager.GetAllActors())
                                {
                                    if (!actor || !actor->IsActive() || actor.get() == player) continue;
                                    if (auto* enc = actor->GetComponent<HEIN::EncounterTriggerComponent>())
                                    {
                                        if (enc->IsProximityLockOnEnabled())
                                        {
                                            if (auto* eTrans = actor->GetComponent<HEIN::TransformComponent>())
                                            {
                                                DirectX::SimpleMath::Vector3 diff = pTrans->GetPosition() - eTrans->GetPosition();
                                                diff.y = 0.0f;
                                                float lockDist = enc->GetLockOnDistance();
                                                if (diff.LengthSquared() <= lockDist * lockDist)
                                                {
                                                    camera->RequestSwitch(HEIN::CameraType::LockOn);
                                                    camera->ApplyRequest();
                                                    if (auto* lockOn = dynamic_cast<HEIN::LockOnCameraMode*>(camera->GetCurrentCameraMode()))
                                                    {
                                                        lockOn->SetTargetID(id);
                                                    }
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // - Fallback to central GameFlowStateMachineComponent if no EncounterTriggerComponent triggered
            if (!triggeredEncounter && !m_hasPlayedIntro && !m_isGhost)
            {
                if (auto* player = GetPlayerActor())
                {
                    if (player->GetActorType() != HEIN::ActorType::Ghost)
                    {
                        auto* flowComp = GetGameFlowComponent();
                        const bool introEnabled = (flowComp != nullptr) ? flowComp->IsIntroCinematicEnabled() : true;
                        const float triggerDist = (flowComp != nullptr) ? flowComp->GetBossIntroTriggerDistance() : 30.0f;

                        if (introEnabled)
                        {
                            if (auto* enemy = GetEnemyActor())
                            {
                                auto* pTrans = player->GetComponent<HEIN::TransformComponent>();
                                auto* eTrans = enemy->GetComponent<HEIN::TransformComponent>();
                                if (pTrans && eTrans)
                                {
                                    DirectX::SimpleMath::Vector3 diff = pTrans->GetPosition() - eTrans->GetPosition();
                                    diff.y = 0.0f;
                                    float distSq = diff.LengthSquared();
                                    if (distSq <= triggerDist * triggerDist)
                                    {
                                        ChangeFlowState(GameFlowState::Intro, gameContext);
                                    }
                                    else
                                    {
                                        if (auto* bt = enemy->GetComponent<HEIN::BehaviourTreeComponent>())
                                        {
                                            bt->SetPaused(true);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

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



            DirectX::SimpleMath::Vector3 targetPos = playerPos + DirectX::SimpleMath::Vector3(0.0f, GameConstants::Camera::TargetHeadVerticalOffset, 0.0f);



            auto* pModel = player->GetComponent<HEIN::SkinnedModelComponent>();



            if (pModel != nullptr)



            {



                DirectX::SimpleMath::Vector3 headPos = pModel->GetBoneWorldPosition(HEIN::HumanoidBone::HeadTop, pTransform->GetWorldMatrix());



                if (headPos != DirectX::SimpleMath::Vector3::Zero && (headPos - playerPos).LengthSquared() <= GameConstants::Camera::MaxValidHeadDistanceSq)



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



void GameScene::ProcessLifecyclePhase(GameContext& gameContext, float deltaTime)



{



    // Health & Death Evaluation



    HEIN::Actor* playerDiedActor = nullptr;



    HEIN::Actor* enemyDiedActor = nullptr;





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

            else if (currentActor->GetActorType() == HEIN::ActorType::Enemy || currentActor->GetID() == m_enemyID)

            {

                enemyDiedActor = currentActor;

            }

            else if (currentActor->GetID() != m_victoryBannerID && currentActor->GetID() != m_backToTitleButtonID &&
                     currentActor->GetID() != m_gameOverBannerID && currentActor->GetID() != m_retryButtonID &&
                     currentActor->GetTag() != L"VictoryBanner" && currentActor->GetTag() != L"BackToTitleButton" &&
                     currentActor->GetTag() != L"GameOverBanner" && currentActor->GetTag() != L"RetryButton")

            {

                m_actorManager.DestroyID(currentActor->GetID());

            }

        }



    }



    if (enemyDiedActor != nullptr)
    {
        // - In Hybrid Mode: evaluate whether this actor's defeat triggers match victory
        bool shouldTriggerVictory = true;
        if (auto* enc = enemyDiedActor->GetComponent<HEIN::EncounterTriggerComponent>())
        {
            if (!enc->IsTriggerVictoryOnDeath())
            {
                shouldTriggerVictory = false;
            }
        }

        m_actorManager.DestroyID(enemyDiedActor->GetID());

        // - Check if other living actors with EncounterTriggerComponent marked for victory still remain
        if (shouldTriggerVictory)
        {
            for (const auto& [id, actor] : m_actorManager.GetAllActors())
            {
                if (!actor || !actor->IsActive()) continue;
                if (auto* otherEnc = actor->GetComponent<HEIN::EncounterTriggerComponent>())
                {
                    if (otherEnc->IsTriggerVictoryOnDeath())
                    {
                        if (auto* hp = actor->GetComponent<HEIN::HealthComponent>())
                        {
                            if (!hp->isDead())
                            {
                                shouldTriggerVictory = false;
                                break;
                            }
                        }
                    }
                }
            }
        }

        if (shouldTriggerVictory && m_flowState != GameFlowState::Victory)
        {
            ChangeFlowState(GameFlowState::Victory, gameContext);
        }
    }

    if (m_flowState == GameFlowState::Victory || m_isVictory)
    {
        // Enforce player Idle state throughout victory celebration
        if (HEIN::Actor* player = GetPlayerActor())
        {
            if (auto* bb = player->GetComponent<HEIN::CombatBlackBoard>())
            {
                bb->moveIntent = DirectX::SimpleMath::Vector3::Zero;
                bb->localMoveIntent = DirectX::SimpleMath::Vector3::Zero;
                bb->isAttacking = false;
                bb->isJumping = false;
                bb->currentVelocity = DirectX::SimpleMath::Vector3::Zero;
            }
            if (auto* rb = player->GetComponent<HEIN::RigidBodyComponent>())
            {
                rb->SetVelocity(DirectX::SimpleMath::Vector3::Zero);
            }
        }

        m_victoryTimer += deltaTime;
        bool backToTitle = false;

        // Debounce delay: require at least 2.0 seconds of victory celebration before accepting return inputs
        if (m_victoryTimer >= 2.0f)
        {
            if (auto* btnActor = m_actorManager.GetActor(m_backToTitleButtonID))
            {
                if (auto* btnComp = btnActor->GetComponent<HEIN::UIButtonComponent>())
                {
                    btnComp->SetVisible(true);
                    if (btnComp->IsClicked())
                    {
                        backToTitle = true;
                    }
                }
            }

            if (m_victoryTimer >= 25.0f)
            {
                backToTitle = true;
            }
        }




        if (backToTitle && gameContext.sceneManager && !gameContext.sceneManager->IsTransitioning())

        {

            gameContext.sceneManager->LoadScene("TitleScene");

            return;

        }

    }



    if (m_flowState == GameFlowState::GameOver)

    {

        m_flowTimer += deltaTime;



        // Check Retry Button click

        if (auto* retryActor = m_actorManager.GetActor(m_retryButtonID))

        {

            if (auto* retryComp = retryActor->GetComponent<HEIN::UIButtonComponent>())

            {

                if (retryComp->IsClicked())

                {

                    ResetMatch(gameContext);

                    return;

                }

            }

        }



        // Check Back to Title Button click

        bool backToTitle = false;

        if (auto* btnActor = m_actorManager.GetActor(m_backToTitleButtonID))

        {

            if (auto* btnComp = btnActor->GetComponent<HEIN::UIButtonComponent>())

            {

                if (btnComp->IsClicked())

                {

                    backToTitle = true;

                }

            }

        }



        if (backToTitle && gameContext.sceneManager && !gameContext.sceneManager->IsTransitioning())

        {

            gameContext.sceneManager->LoadScene("TitleScene");

            return;

        }

    }



    // Handle Player Death -> Spawn Ghost & Destroy Current Body



    if (playerDiedActor != nullptr && !m_isGhost)



    {



        if (!m_isDying)



        {



            // Enter death freeze state: hold the dramatic hit frame before transitioning to corpse



            m_isDying = true;



            m_deathFreezeTimer = static_cast<float>(m_deathFreezeFrames) / 60.0f;



            // Trigger hitstop on player model (dramatic time freeze)



            playerDiedActor->TriggerHitstopFrames(m_deathFreezeFrames);



            if (auto* health = playerDiedActor->GetComponent<HEIN::HealthComponent>())



            {



                if (health->IsHitFlashEnabled())



                {



                    if (auto* model = playerDiedActor->GetComponent<HEIN::SkinnedModelComponent>())



                    {



                        model->TriggerHitFlash(health->GetHitFlashColor(), health->GetHitFlashDuration());



                    }



                }



            }



            // Also ensure the enemy/attacker gets hitstop freeze for symmetry



            if (HEIN::Actor* enemy = GetEnemyActor())



            {



                enemy->TriggerHitstopFrames(m_deathFreezeFrames);



            }



            return; // Wait for freeze timer before completing death sequence



        }



        // Count down death freeze timer (initial impact freeze)

        if (m_deathFreezeTimer > 0.0f)

        {

            m_deathFreezeTimer -= deltaTime;

            if (m_deathFreezeTimer > 0.0f)

            {

                return; // Still freezing on lethal impact

            }

        }

        // Freeze finished! Trigger and play the full Death animation state

        if (!m_isDeathAnimPlaying)

        {

            m_isDeathAnimPlaying = true;

            float deathDuration = 3.9f;

            if (auto* sm = playerDiedActor->GetComponent<HEIN::CombatStateMachineComponent>())

            {

                sm->ChangeState("Death", 0.15f);

                sm->ApplyPendingState();

                if (const auto* cfg = sm->GetCurrentStateConfig())

                {

                    if (cfg->stateDuration > 0.1f)

                    {

                        deathDuration = cfg->stateDuration;

                    }

                }

            }

            else if (auto* model = playerDiedActor->GetComponent<HEIN::SkinnedModelComponent>())

            {

                model->CrossfadeAnimation("Death", 0.15f, true);

                float dur = model->GetAnimationDuration("Death");

                if (dur > 0.1f) deathDuration = dur;

            }

            m_deathAnimTimer = deathDuration;

            return; // Wait for death animation to finish playing

        }

        // Count down death animation timer

        if (m_deathAnimTimer > 0.0f)

        {

            m_deathAnimTimer -= deltaTime;

            if (m_deathAnimTimer > 0.0f)

            {

                return; // Still playing death animation

            }

        }

        // Death animation finished! Complete transition to ghost or game over

        m_isDying = false;

        m_isDeathAnimPlaying = false;

        m_deathFreezeTimer = 0.0f;

        m_deathAnimTimer = 0.0f;



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



        // Delete/destroy the actor model and attached child weapons (so no lingering running body in ghost form)



        for (HEIN::ActorID childID : playerDiedActor->GetChildren())



        {



            m_actorManager.DestroyID(childID);



        }



        m_actorManager.DestroyID(playerDiedActor->GetID());



        // Spawn or activate ghost at death position



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



            ghost = m_actorManager.GetFirstActorByType(HEIN::ActorType::Ghost);



        }



       



        if (ghost != nullptr && ghost != playerDiedActor)



        {



            ghost->SetActive(true);



            ghost->SetVisible(true);



            ghost->SetActorType(HEIN::ActorType::Ghost);
            if (auto* ghostMove = ghost->GetComponent<HEIN::CharacterMovementComponent>())
            {
                ghostMove->SetAutoCrossfadeLocomotion(true);
                ghostMove->SetAllowLockOnFacing(false);
            }



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



            DirectX::SimpleMath::Vector3 ghostSpawnPos = deathPos + DirectX::SimpleMath::Vector3(0.0f, GameConstants::Ghost::SpawnVerticalOffset, 0.0f);



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



                rb->Initialize(GameConstants::Ghost::PhysicsMass, true, false);



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



                col->Initialize(GameConstants::Ghost::ColliderRadius, GameConstants::Ghost::ColliderHeight);



                col->SetOffset(DirectX::SimpleMath::Vector3(0.0f, GameConstants::Ghost::ColliderVerticalOffset, 0.0f));



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



            // Re-hook camera to track ghost
            if (auto* camera = GetActiveCameraController(gameContext))
            {
                SetupCameraModes(gameContext, camera, ghost->GetComponent<HEIN::SkinnedModelComponent>());
                camera->RequestSwitch(HEIN::CameraType::Spring);
                camera->ApplyRequest();
            }

            // Ensure ghost blackboard and target tracking do not have lock on enabled
            if (auto* ghostBB = ghost->GetComponent<HEIN::CombatBlackBoard>())
            {
                ghostBB->isLockedOn = false;
            }
            if (auto* ghostTracking = ghost->GetComponent<HEIN::TargetTrackingComponent>())
            {
                ghostTracking->SetLockedOn(false);
                ghostTracking->SetTargetID(HEIN::INVALID_ACTOR_ID);
            }

            // AI across all actors drops target (returns to idle or spawn tether)
            for (const auto& [id, actor] : m_actorManager.GetAllActors())
            {
                if (!actor || !actor->IsActive()) continue;
                if (auto* bt = actor->GetComponent<HEIN::BehaviourTreeComponent>())
                {
                    bt->SetTargetID(HEIN::INVALID_ACTOR_ID);
                }
            }



        }



    }



    else if (playerDiedActor == nullptr && m_isDying)



    {



        m_isDying = false;



        m_deathFreezeTimer = 0.0f;



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



        // Establish orientation matrix centered at world origin



        DirectX::SimpleMath::Matrix lightViewOrigin = DirectX::SimpleMath::Matrix::CreateLookAt(



            DirectX::SimpleMath::Vector3::Zero,



            lightDir * 1000.0f,



            up



        );



        // Project targetCenter (player) into orientation space



        DirectX::SimpleMath::Vector3 targetInLight = DirectX::SimpleMath::Vector3::Transform(targetCenter, lightViewOrigin);



        // Snap to discrete texel multiples in world units



        const float shadowMapResolution = 2048.0f;



        float worldUnitsPerTexel = projSize / shadowMapResolution;



        targetInLight.x = std::floor(targetInLight.x / worldUnitsPerTexel) * worldUnitsPerTexel;



        targetInLight.y = std::floor(targetInLight.y / worldUnitsPerTexel) * worldUnitsPerTexel;



        // Transform snapped position back to world space



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



    bool isEditorVisible = (m_debugDisplay != nullptr && m_debugDisplay->isVisible());



    if (isEditorVisible)



    {



        const float editorClearColor[4] = { 0.14f, 0.14f, 0.14f, 1.0f };



        context->ClearRenderTargetView(rtv, editorClearColor);



    }



    D3D11_VIEWPORT viewport = isEditorVisible



        ? m_debugDisplay->GetDebugUI().GetSceneViewport(gameContext)



        : gameContext.deviceResources.GetScreenViewport();



    context->RSSetViewports(1, &viewport);



    if (viewport.Height > 0.0f)



    {



        float aspectRatio = viewport.Width / viewport.Height;



        float fov = (gameContext.mainCamera != nullptr) ? gameContext.mainCamera->GetFov() : (DirectX::XM_PI / 4.0f);



        m_proj = DirectX::SimpleMath::Matrix::CreatePerspectiveFieldOfView(fov, aspectRatio, 0.1f, 5000.0f);



    }



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



    // If AllColliders mode is active, enable showColliders so DrawAll queues wireframes



    bool prevShowColliders = gameContext.showColliders;



    if (m_hitboxDebugScope == HitboxDebugScope::AllColliders)



    {



        gameContext.showColliders = true;



    }



    // Draw all active entities



    m_actorManager.DrawAll(gameContext, view, m_proj);



    gameContext.showColliders = prevShowColliders;



    // Hitbox Debugger & Dynamic Collision Rendering (Shockwaves, Colliders, Attack Hitboxes)



    if (gameContext.debugCollisionRenderer != nullptr)



    {

        // - Queue boss intro trigger range volume when debug display is active
        if (m_hitboxDebugScope != HitboxDebugScope::Off || gameContext.showColliders || gameContext.isSceneViewPass)
        {
            if (auto* flowComp = GetGameFlowComponent())
            {
                if (flowComp->IsDrawDebugTriggerRange())
                {
                    flowComp->Draw(gameContext, DirectX::SimpleMath::Matrix::Identity, view, m_proj);
                }
            }
        }



        if (m_hitboxDebugScope == HitboxDebugScope::ActiveAttacksOnly)



        {



            // Smart Hitbox Mode: queue only active weapon hitboxes during attack swings



            gameContext.showColliders = true;



            for (const auto& [id, actor] : m_actorManager.GetAllActors())



            {



                if (!actor || !actor->IsActive()) continue;



                if (auto* dealer = actor->GetComponent<HEIN::DamageDealerComponent>())



                {



                    if (dealer->IsActive())



                    {



                        if (auto* obb = actor->GetComponent<HEIN::OBBColliderComponent>())



                            obb->Draw(gameContext, DirectX::SimpleMath::Matrix::Identity, view, m_proj);



                        if (auto* cap = actor->GetComponent<HEIN::CapsuleColliderComponent>())



                            cap->Draw(gameContext, DirectX::SimpleMath::Matrix::Identity, view, m_proj);



                        if (auto* sph = actor->GetComponent<HEIN::SphereColliderComponent>())



                            sph->Draw(gameContext, DirectX::SimpleMath::Matrix::Identity, view, m_proj);



                    }



                }



            }



            gameContext.showColliders = prevShowColliders;



        }



        // Render and flush queued wireframes (queued by colliders, smart hitboxes, or ShockwaveEmitterComponent)



        if (gameContext.debugCollisionRenderer->HasQueuedData())



        {



            gameContext.debugCollisionRenderer->RenderAndFlush(context, gameContext.commonStates, view, m_proj);



        }



        else



        {



            gameContext.debugCollisionRenderer->Clear();



        }



    }



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



void GameScene::OpenEditor()



{



    if (!m_debugDisplay)



    {



        m_debugDisplay = std::make_unique<HEIN::DebugDisplayController>();



        m_debugDisplay->Initialize();



        UpdateDebugTargets();



    }



}



void GameScene::UnloadEditor(GameContext& gameContext)



{



    m_debugDisplay.reset();



    m_actorManager.ReleaseEditorResources();



    if (gameContext.debugCollisionRenderer != nullptr)



    {



        gameContext.debugCollisionRenderer->ReleaseDebugMemory();



    }



    gameContext.isEditorMode = false;



    gameContext.showColliders = false;



    gameContext.showUI = true;



    if (HEIN::CameraController* camera = GetActiveCameraController(gameContext))



    {



        camera->UpdateMouseMode();



    }



}



void GameScene::HandleEditorActions(GameContext& gameContext)



{



    if (!m_debugDisplay) return;



    HEIN::EditorAction uiAction = m_debugDisplay->GetUIAction();



    if (uiAction == HEIN::EditorAction::PlayPressed || !m_debugDisplay->isVisible())



    {



        m_isPlaying = true;



        UnloadEditor(gameContext);



        return;



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



            SanitizeActorsForSave(m_actorManager);



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



        SanitizeActorsForSave(m_actorManager);



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



    // Look for an actor explicitly marked as ActorType::Player



    for (const auto& pair : m_actorManager.GetAllActors())



    {



        if (pair.second && pair.second->GetActorType() == HEIN::ActorType::Player)



        {



            m_playerID = pair.first;



            return pair.second.get();



        }



    }



    // Look for an actor with ActorType::Player



    player = m_actorManager.GetFirstActorByType(HEIN::ActorType::Player);



    if (player)



    {



        m_playerID = player->GetID();



        return player;



    }



    // Fallback: If no player actor exists yet, select the first possessable vessel as starting body



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



        enemy = m_actorManager.GetFirstActorByType(HEIN::ActorType::Enemy);



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



            m_targetPos = playerPos + DirectX::SimpleMath::Vector3(0.0f, GameConstants::Camera::TargetHeadVerticalOffset, 0.0f);



            if (modelPointer != nullptr)



            {



                modelPointer->Update(0.0f);



                DirectX::SimpleMath::Vector3 headPos = modelPointer->GetBoneWorldPosition(HEIN::HumanoidBone::HeadTop, playerTransform->GetWorldMatrix());



                if (headPos != DirectX::SimpleMath::Vector3::Zero && (headPos - playerPos).LengthSquared() <= GameConstants::Camera::MaxValidHeadDistanceSq)



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



        cameraComp->RegisterCamera(



            HEIN::CameraType::LockOn,
            [this]()
            {
                HEIN::ActorID enemyId = m_enemyID;
                if (enemyId == HEIN::INVALID_ACTOR_ID || m_actorManager.GetActor(enemyId) == nullptr)
                {
                    HEIN::Actor* enemy = GetEnemyActor();
                    if (enemy)
                    {
                        enemyId = enemy->GetID();
                    }
                }
                HEIN::ActorID playerId = m_playerID;
                if (playerId == HEIN::INVALID_ACTOR_ID || m_actorManager.GetActor(playerId) == nullptr)
                {
                    HEIN::Actor* player = GetPlayerActor();
                    if (player)
                    {
                        playerId = player->GetID();
                    }
                }
                return std::make_unique<HEIN::LockOnCameraMode>(
                    &m_actorManager, playerId, enemyId);
            }



        );



        cameraComp->RegisterCamera(



            HEIN::CameraType::Cinematic,



            [this, cameraComp]()



            {



                auto cinematic = std::make_unique<HEIN::CinematicMode>(



                    &m_actorManager, m_enemyID, m_targetPos);



                if (cameraComp && cameraComp->HasSavedCinematicTracks())



                {



                    cinematic->DeserializeTracks(cameraComp->GetSavedCinematicTracks());



                }



                else if (auto* enemy = GetEnemyActor())



                {



                    if (auto* player = GetPlayerActor())



                    {



                        auto* eTrans = enemy->GetComponent<HEIN::TransformComponent>();



                        auto* pTrans = player->GetComponent<HEIN::TransformComponent>();



                        if (eTrans && pTrans)



                        {



                            cinematic->SetupBossIntroPreset(eTrans->GetPosition(), pTrans->GetPosition());



                        }



                    }



                }



                return cinematic;



            }



        );



    }



    HEIN::CameraType initialType = cameraComp->GetSavedCameraType().value_or(HEIN::CameraType::Spring);



    cameraComp->SetFirstCamera(initialType);



}



void GameScene::PostSceneLoadInit(GameContext& gameContext)



{



    m_hasPlayedIntro = false;

    if (auto* flowComp = GetGameFlowComponent())

    {

        flowComp->SetHasPlayedIntro(false);

    }



    // Destroy any lingering victory or game over UI actors from previous runs



    // Ensure victory and game over UI elements are hidden at match start but preserved in memory
    auto victoryAct = m_actorManager.GetActorByName(L"VictoryBanner");
    if (victoryAct) { victoryAct->SetVisible(false); victoryAct->SetActive(false); m_victoryBannerID = victoryAct->GetID(); }
    else m_victoryBannerID = HEIN::INVALID_ACTOR_ID;

    auto gameOverAct = m_actorManager.GetActorByName(L"GameOverBanner");
    if (gameOverAct) { gameOverAct->SetVisible(false); gameOverAct->SetActive(false); m_gameOverBannerID = gameOverAct->GetID(); }
    else m_gameOverBannerID = HEIN::INVALID_ACTOR_ID;

    auto backBtnAct = m_actorManager.GetActorByName(L"BackToTitleButton");
    if (backBtnAct) { backBtnAct->SetVisible(false); backBtnAct->SetActive(false); m_backToTitleButtonID = backBtnAct->GetID(); }
    else m_backToTitleButtonID = HEIN::INVALID_ACTOR_ID;

    auto retryBtnAct = m_actorManager.GetActorByName(L"RetryButton");
    if (retryBtnAct) { retryBtnAct->SetVisible(false); retryBtnAct->SetActive(false); m_retryButtonID = retryBtnAct->GetID(); }
    else m_retryButtonID = HEIN::INVALID_ACTOR_ID;



    // Reset health and invincibility on all loaded actors for fresh gameplay state



    for (const auto& pair : m_actorManager.GetAllActors())



    {



        if (HEIN::Actor* actor = pair.second.get())



        {



            if (auto* health = actor->GetComponent<HEIN::HealthComponent>())



            {



                health->SetCurrentHealth(health->GetMaxHealth());



                health->SetGameplayInvincible(false);



            }



        }



    }



    // Re-establish "Glue" Connections



    HEIN::Actor* player = GetPlayerActor();



    HEIN::Actor* enemy = GetEnemyActor();



    // If enemy actor was missing or destroyed in a prior save, reconstruct from factory template



    if (enemy == nullptr)



    {



        HEIN::EnemySpawnData spawnData = HEIN::ActorFactory::CreateEnemy(m_actorManager, gameContext, player != nullptr ? player->GetID() : HEIN::INVALID_ACTOR_ID);



        enemy = m_actorManager.GetActor(spawnData.enemyID);



        m_enemyID = spawnData.enemyID;



    }



    HEIN::CameraController* camera = GetActiveCameraController(gameContext);



    if (player != nullptr)



    {



        player->SetActorType(HEIN::ActorType::Player);



        m_playerID = player->GetID();



        m_isGhost = false;



        m_isDying = false;



        m_deathFreezeTimer = 0.0f;



        m_targetVesselID = HEIN::INVALID_ACTOR_ID;



        if (auto* health = player->GetComponent<HEIN::HealthComponent>())



        {



            health->SetCurrentHealth(health->GetMaxHealth());



            health->SetGameplayInvincible(false);



        }



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



                    // Keep visible for rendering, but deactivate UI gauges



                    if (auto* gauge = actor->GetComponent<HEIN::GaugeComponent>())



                    {



                        gauge->SetVisible(false);



                    }



                    // Disable expensive procedural foot IK



                    if (auto* procAnim = actor->GetComponent<HEIN::ProceduralAnimationComponent>())



                    {



                        procAnim->SetEnabled(false);



                    }



                    // Put rigidbody into kinematic sleep so physics collisions don't tick



                    if (auto* rb = actor->GetComponent<HEIN::RigidBodyComponent>())



                    {



                        rb->SetKinematic(true);



                        rb->SetVelocity(DirectX::SimpleMath::Vector3::Zero);



                    }



                    // Ensure vessel is invincible while dormant



                    if (auto* health = actor->GetComponent<HEIN::HealthComponent>())



                    {



                        health->SetGameplayInvincible(true);



                    }



                    // Ensure dormant vessel is playing Idle animation



                    if (auto* model = actor->GetComponent<HEIN::SkinnedModelComponent>())



                    {



                        model->ChangeAnimation("Idle");



                    }



                }



            }



        }



        // Ensure runtime components exist for earlier saves



        if (!player->GetComponent<HEIN::PlayerBlackBoard>())



            player->AddComponent<HEIN::PlayerBlackBoard>();



        if (!player->GetComponent<HEIN::PlayerInputComponent>())



            player->AddComponent<HEIN::PlayerInputComponent>(&m_actorManager);



        if (!player->GetComponent<HEIN::CharacterMovementComponent>())



            player->AddComponent<HEIN::CharacterMovementComponent>();



        if (!player->GetComponent<HEIN::ShockwaveEmitterComponent>())



            player->AddComponent<HEIN::ShockwaveEmitterComponent>(&m_actorManager);



    }



    if (enemy != nullptr)



    {



        enemy->SetActorType(HEIN::ActorType::Enemy);



        m_enemyID = enemy->GetID();



        if (auto* health = enemy->GetComponent<HEIN::HealthComponent>())



        {



            health->SetCurrentHealth(health->GetMaxHealth());



            health->SetGameplayInvincible(false);



        }



        if (auto* rb = enemy->GetComponent<HEIN::RigidBodyComponent>())



        {



            rb->SetKinematic(false);



            rb->SetVelocity(DirectX::SimpleMath::Vector3::Zero);



        }



        if (auto* cm = enemy->GetComponent<HEIN::CharacterMovementComponent>())



        {



            cm->ClearImpulse();



        }



        if (auto* combat = enemy->GetComponent<HEIN::CombatStateMachineComponent>())



        {



            combat->ChangeState("Idle");



        }



        else if (auto* model = enemy->GetComponent<HEIN::SkinnedModelComponent>())



        {



            if (model->HasAnimation("Idle"))



            {



                model->ChangeAnimation("Idle");



            }



        }



        auto* bb = enemy->GetComponent<HEIN::BossBlackBoard>();



        if (!bb)



        {



            bb = enemy->AddComponent<HEIN::BossBlackBoard>();



        }



        if (bb)



        {



            if (auto* trans = enemy->GetComponent<HEIN::TransformComponent>())



            {



                bb->spawnPosition = trans->GetPosition();



                bb->hasSetSpawnPosition = true;



            }



        }



        if (!enemy->GetComponent<HEIN::CharacterMovementComponent>())



            enemy->AddComponent<HEIN::CharacterMovementComponent>();



        if (!enemy->GetComponent<HEIN::ShockwaveEmitterComponent>())



            enemy->AddComponent<HEIN::ShockwaveEmitterComponent>(&m_actorManager);



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



            HEIN::Actor* targetOwner = nullptr;



            if (!socketComp->GetOwnerTag().empty())



            {



                targetOwner = m_actorManager.GetActorByName(socketComp->GetOwnerTag());



            }



            if (!targetOwner && actor->GetParentID() != HEIN::INVALID_ACTOR_ID)



            {



                targetOwner = m_actorManager.GetActor(actor->GetParentID());



            }



            if (!targetOwner && player != nullptr && actor->GetActorType() != HEIN::ActorType::Enemy)



            {



                targetOwner = player;



            }



            if (targetOwner != nullptr)



            {



                actor->SetOwnerID(targetOwner->GetID());



                socketComp->SetOwnerTag(targetOwner->GetTag());



                socketComp->Initialize(targetOwner->GetID(), socketComp->GetSocketName());



            }



        }



        else if (actor->GetParentID() != HEIN::INVALID_ACTOR_ID)



        {



            actor->SetOwnerID(actor->GetParentID());



        }



    }



    // Automatically link weapons to their wielder using hierarchy or type rather than hardcoded string names



    for (const auto& pair : m_actorManager.GetAllActors())



    {



        HEIN::Actor* actor = pair.second.get();



        if (!actor || !actor->GetComponent<HEIN::DamageDealerComponent>()) continue;



        if (actor->GetOwnerID() == HEIN::INVALID_ACTOR_ID || actor->GetOwnerID() == actor->GetID())



        {



            if (actor->GetParentID() != HEIN::INVALID_ACTOR_ID)



            {



                actor->SetOwnerID(actor->GetParentID());



            }



            else if (player != nullptr && actor->GetActorType() != HEIN::ActorType::Enemy)



            {



                actor->SetOwnerID(player->GetID());



            }



        }



    }



    // Ensure every weapon actor in the scene has a WeaponTrailComponent (preserving serialized customizations if already present)



    for (const auto& pair : m_actorManager.GetAllActors())



    {



        HEIN::Actor* actor = pair.second.get();



        if (!actor || !actor->GetComponent<HEIN::DamageDealerComponent>()) continue;



        if (!actor->GetComponent<HEIN::WeaponTrailComponent>())



        {



            auto* trail = actor->AddComponent<HEIN::WeaponTrailComponent>(&m_actorManager);



            HEIN::Actor* wielder = m_actorManager.GetActor(actor->GetOwnerID());



            if (!wielder && actor->GetParentID() != HEIN::INVALID_ACTOR_ID)



            {



                wielder = m_actorManager.GetActor(actor->GetParentID());



            }



            const bool isBossWeapon = (wielder != nullptr && wielder->GetActorType() == HEIN::ActorType::Enemy);



            if (isBossWeapon)



            {



                trail->ApplyBossBloodflamePreset();



            }



            else



            {



                trail->ApplyTarnishedGoldPreset();



            }



            trail->AutoFitOffsetsFromWeapon();



        }



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



    if (HEIN::Actor* ghostActor = m_actorManager.GetFirstActorByType(HEIN::ActorType::Ghost))



    {



        if (!ghostActor->GetComponent<HEIN::RigidBodyComponent>())



        {



            auto* rb = ghostActor->AddComponent<HEIN::RigidBodyComponent>();



            rb->Initialize(GameConstants::Ghost::PhysicsMass, true, false);



            rb->SetNeedsInitialSnap(false);



        }



        if (!ghostActor->GetComponent<HEIN::CapsuleColliderComponent>())



        {



            auto* col = ghostActor->AddComponent<HEIN::CapsuleColliderComponent>();



            col->Initialize(GameConstants::Ghost::ColliderRadius, GameConstants::Ghost::ColliderHeight);



            col->SetOffset(DirectX::SimpleMath::Vector3(0.0f, GameConstants::Ghost::ColliderVerticalOffset, 0.0f));



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



    // Resolve GameManager from loaded level data (guarantees correct trigger distance and intro state)
    HEIN::Actor* gmActor = m_actorManager.GetActorByName(L"GameManager");
    if (!gmActor)
    {
        gmActor = m_actorManager.CreateActor(L"GameManager");
        gmActor->SetActorType(HEIN::ActorType::Default);
    }
    if (!gmActor->GetComponent<HEIN::GameFlowStateMachineComponent>())
    {
        gmActor->AddComponent<HEIN::GameFlowStateMachineComponent>();
    }
    m_gameManagerID = gmActor->GetID();
    if (auto* flowComp = gmActor->GetComponent<HEIN::GameFlowStateMachineComponent>())
    {
        m_hasPlayedIntro = flowComp->HasPlayedIntro();
    }

    // Re-hook the camera modes to the loaded player model



    if (player != nullptr && camera != nullptr)



    {



        HEIN::SkinnedModelComponent* pModel = player->GetComponent<HEIN::SkinnedModelComponent>();



        SetupCameraModes(gameContext, camera, pModel);



    }



}



void GameScene::TriggerVictory(GameContext& gameContext)

{

    if (m_isVictory) return;



    m_isVictory = true;

    m_victoryTimer = 0.0f;

    // Transition player into Idle state, clear all movement intent and zero external impulses
    if (HEIN::Actor* player = GetPlayerActor())
    {
        if (auto* bb = player->GetComponent<HEIN::CombatBlackBoard>())
        {
            bb->moveIntent = DirectX::SimpleMath::Vector3::Zero;
            bb->localMoveIntent = DirectX::SimpleMath::Vector3::Zero;
            bb->isAttacking = false;
            bb->isJumping = false;
            bb->currentVelocity = DirectX::SimpleMath::Vector3::Zero;
            bb->currentStance = HEIN::CombatStance::Idle;
        }

        if (auto* sm = player->GetComponent<HEIN::CombatStateMachineComponent>())
        {
            sm->ChangeState("Idle", 0.15f);
            sm->ApplyPendingState();
        }
        else
        {
            for (auto* model : player->GetComponents<HEIN::SkinnedModelComponent>())
            {
                if (model && model->HasAnimation("Idle"))
                {
                    model->CrossfadeAnimation("Idle", 0.15f);
                }
            }
        }

        if (auto* moveComp = player->GetComponent<HEIN::CharacterMovementComponent>())
        {
            moveComp->ClearImpulse();
        }

        if (auto* rb = player->GetComponent<HEIN::RigidBodyComponent>())
        {
            rb->SetVelocity(DirectX::SimpleMath::Vector3::Zero);
        }
    }



    // Disengage camera lock-on so camera smoothly returns to spring mode

    if (auto* camera = GetActiveCameraController(gameContext))

    {

        if (camera->GetCurrentCameraType() == HEIN::CameraType::LockOn)

        {

            camera->RequestSwitch(HEIN::CameraType::Spring);

        }

    }



    // Delete the boss actor upon defeat as requested

    if (HEIN::Actor* enemy = GetEnemyActor())

    {

        m_actorManager.DestroyID(enemy->GetID());

    }

    if (m_enemyID != HEIN::INVALID_ACTOR_ID)

    {

        m_actorManager.DestroyID(m_enemyID);

    }

    m_enemyID = HEIN::INVALID_ACTOR_ID;



    // Restore mouse cursor to absolute mode and ensure it is visible for UI interaction
    ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
    ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
    ::SetCursor(::LoadCursor(nullptr, IDC_ARROW));
    DirectX::Mouse::Get().SetMode(DirectX::Mouse::MODE_ABSOLUTE);
    while (ShowCursor(TRUE) < 0);



    // Resolve font path from discovered assets

    std::wstring defaultFontPath = L"";

    const auto discoveredFonts = HEIN::UIButtonComponent::GetDiscoveredFontPaths();

    for (const auto& fontPath : discoveredFonts)

    {

        if (fontPath.find(L"ITZorak") != std::wstring::npos)

        {

            defaultFontPath = fontPath;

            break;

        }

    }

    if (defaultFontPath.empty() && !discoveredFonts.empty())

    {

        defaultFontPath = discoveredFonts.front();

    }



    // Reuse existing persistent VictoryBanner actor from scene data if created in editor
    HEIN::Actor* bannerActor = m_actorManager.GetActor(m_victoryBannerID);
    if (!bannerActor)
    {
        bannerActor = m_actorManager.GetActorByName(L"VictoryBanner");
    }

    if (bannerActor)
    {
        bannerActor->SetVisible(true);
        bannerActor->SetActive(true);
        m_victoryBannerID = bannerActor->GetID();
        if (auto* bannerComp = bannerActor->GetComponent<HEIN::UIButtonComponent>())
        {
            bannerComp->SetVisible(true);
        }
    }
    else
    {
        bannerActor = m_actorManager.CreateActor(L"VictoryBanner");
        m_victoryBannerID = bannerActor->GetID();
        bannerActor->AddComponent<HEIN::TransformComponent>();
        auto* bannerComp = bannerActor->AddComponent<HEIN::UIButtonComponent>();
        bannerComp->SetElementType(HEIN::UIElementType::Text);
        bannerComp->SetText("Y O U   W I N");
        if (!defaultFontPath.empty())
        {
            bannerComp->SetSpriteFontPath(defaultFontPath, gameContext.deviceResources.GetD3DDevice());
        }
        bannerComp->SetFontSize(1.6f);
        bannerComp->SetTextColor(DirectX::SimpleMath::Vector4(0.98f, 0.86f, 0.38f, 1.0f));
        bannerComp->SetPosition(DirectX::SimpleMath::Vector2(440.0f, 210.0f));
        bannerComp->SetSize(DirectX::SimpleMath::Vector2(400.0f, 80.0f));
        bannerComp->Initialize(gameContext, nullptr, nullptr, nullptr);
        bannerActor->Start();
    }

    // Reuse existing persistent BackToTitleButton actor from scene data
    HEIN::Actor* backButtonActor = m_actorManager.GetActor(m_backToTitleButtonID);
    if (!backButtonActor)
    {
        backButtonActor = m_actorManager.GetActorByName(L"BackToTitleButton");
    }

    if (backButtonActor)
    {
        backButtonActor->SetVisible(true);
        backButtonActor->SetActive(true);
        m_backToTitleButtonID = backButtonActor->GetID();
        if (auto* backButtonComp = backButtonActor->GetComponent<HEIN::UIButtonComponent>())
        {
            backButtonComp->SetVisible(false); // Managed by victory reveal timer
        }
    }
    else
    {
        backButtonActor = m_actorManager.CreateActor(L"BackToTitleButton");
        m_backToTitleButtonID = backButtonActor->GetID();
        backButtonActor->AddComponent<HEIN::TransformComponent>();
        auto* backButtonComp = backButtonActor->AddComponent<HEIN::UIButtonComponent>();
        backButtonComp->SetElementType(HEIN::UIElementType::Button);
        backButtonComp->SetText("BACK TO TITLE");
        if (!defaultFontPath.empty())
        {
            backButtonComp->SetSpriteFontPath(defaultFontPath, gameContext.deviceResources.GetD3DDevice());
        }
        backButtonComp->SetFontSize(0.85f);
        backButtonComp->SetUseEldenStyle(true);
        backButtonComp->SetPosition(DirectX::SimpleMath::Vector2(500.0f, 340.0f));
        backButtonComp->SetSize(DirectX::SimpleMath::Vector2(280.0f, 56.0f));
        backButtonComp->SetVisible(false);
        backButtonComp->Initialize(gameContext, nullptr, nullptr, nullptr);
        backButtonActor->Start();
    }

}



HEIN::EncounterTriggerComponent* GameScene::GetActiveEncounterComponent()
{
    if (m_activeEncounterActorID != HEIN::INVALID_ACTOR_ID)
    {
        if (auto* actor = m_actorManager.GetActor(m_activeEncounterActorID))
        {
            if (auto* enc = actor->GetComponent<HEIN::EncounterTriggerComponent>())
            {
                return enc;
            }
        }
    }

    for (const auto& [id, actor] : m_actorManager.GetAllActors())
    {
        if (!actor || !actor->IsActive()) continue;
        if (auto* enc = actor->GetComponent<HEIN::EncounterTriggerComponent>())
        {
            if (enc->IsPlayingCutscene())
            {
                m_activeEncounterActorID = id;
                return enc;
            }
        }
    }

    return nullptr;
}

HEIN::GameFlowStateMachineComponent* GameScene::GetGameFlowComponent()

{

    if (m_gameManagerID != HEIN::INVALID_ACTOR_ID)

    {

        if (auto* actor = m_actorManager.GetActor(m_gameManagerID))

        {

            return actor->GetComponent<HEIN::GameFlowStateMachineComponent>();

        }

    }

    if (auto* actor = m_actorManager.GetActorByName(L"GameManager"))

    {

        m_gameManagerID = actor->GetID();

        return actor->GetComponent<HEIN::GameFlowStateMachineComponent>();

    }

    return nullptr;

}



void GameScene::ChangeFlowState(GameFlowState newState, GameContext& gameContext)

{

    if (m_flowState == newState) return;



    m_flowState = newState;

    m_flowTimer = 0.0f;



    if (auto* flowComp = GetGameFlowComponent())

    {

        flowComp->ChangeState(newState, gameContext);

    }



    switch (m_flowState)

    {

    case GameFlowState::Intro:

        m_hasPlayedIntro = true;

        if (auto* flowComp = GetGameFlowComponent())

        {

            flowComp->SetHasPlayedIntro(true);

            flowComp->FreezeBossAndPlayer(gameContext);

        }

        else

        {

            if (auto* enemy = GetEnemyActor())

            {

                if (auto* bt = enemy->GetComponent<HEIN::BehaviourTreeComponent>())

                {

                    bt->SetPaused(true);

                }

                if (auto* bb = enemy->GetComponent<HEIN::BossBlackBoard>())

                {

                    bb->moveIntent = DirectX::SimpleMath::Vector3::Zero;

                    bb->isLockedOn = false;

                    bb->isAttacking = false;

                    bb->activeNodeName = "Cinematic (Paused)";

                }

                if (auto* rb = enemy->GetComponent<HEIN::RigidBodyComponent>())

                {

                    rb->SetVelocity(DirectX::SimpleMath::Vector3::Zero);

                }

                if (auto* cm = enemy->GetComponent<HEIN::CharacterMovementComponent>())

                {

                    cm->ClearImpulse();

                }

                if (auto* combat = enemy->GetComponent<HEIN::CombatStateMachineComponent>())

                {

                    if (combat->GetCurrentStateName() != "Idle")

                    {

                        combat->ChangeState("Idle", 0.1f);

                    }

                }

                if (auto* model = enemy->GetComponent<HEIN::SkinnedModelComponent>())

                {

                    if (model->HasAnimation("Idle"))

                    {

                        model->ChangeAnimation("Idle");

                    }

                }

            }

        }

        if (auto* camera = GetActiveCameraController(gameContext))

        {

            camera->RequestSwitch(HEIN::CameraType::Cinematic);

            camera->ApplyRequest();

            if (auto* cinematic = dynamic_cast<HEIN::CinematicMode*>(camera->GetCurrentCameraMode()))

            {

                cinematic->Play();

            }

        }

        break;



    case GameFlowState::Gameplay:
        if (auto* enc = GetActiveEncounterComponent())
        {
            // The encounter component manages delayed AI wake-up and unfreezing once camera blend completes
        }
        else if (auto* flowComp = GetGameFlowComponent())
        {
            flowComp->UnfreezeBossAndPlayer(gameContext);
        }
        else
        {
            if (auto* enemy = GetEnemyActor())
            {
                if (auto* bt = enemy->GetComponent<HEIN::BehaviourTreeComponent>())
                {
                    bt->SetPaused(false);
                }
                if (auto* model = enemy->GetComponent<HEIN::SkinnedModelComponent>())
                {
                    if (model->HasAnimation("Idle"))
                    {
                        model->ChangeAnimation("Idle");
                    }
                }
            }
        }

        if (auto* camera = GetActiveCameraController(gameContext))
        {
            auto* player = GetPlayerActor();
            const bool isGhostPlayer = (m_isGhost || (player && player->GetActorType() == HEIN::ActorType::Ghost));
            if (!isGhostPlayer && m_activeEncounterActorID != HEIN::INVALID_ACTOR_ID && m_actorManager.GetActor(m_activeEncounterActorID) != nullptr)
            {
                if (camera->GetCurrentCameraType() != HEIN::CameraType::LockOn)
                {
                    camera->RequestSwitch(HEIN::CameraType::LockOn);
                    camera->ApplyRequest();
                }
                if (auto* lockOn = dynamic_cast<HEIN::LockOnCameraMode*>(camera->GetCurrentCameraMode()))
                {
                    lockOn->SetTargetID(m_activeEncounterActorID);
                }
            }
            else if (camera->GetCurrentCameraType() != HEIN::CameraType::LockOn || isGhostPlayer)
            {
                if (camera->GetCurrentCameraType() != HEIN::CameraType::Spring)
                {
                    camera->RequestSwitch(HEIN::CameraType::Spring);
                }
            }
            camera->UpdateMouseMode();
        }

        break;



    case GameFlowState::Victory:

        TriggerVictory(gameContext);

        break;



    case GameFlowState::GameOver:

        TriggerGameOver(gameContext);

        break;

    }

}



void GameScene::TriggerGameOver(GameContext& gameContext)

{

    // Disengage camera lock-on so camera smoothly returns to spring mode

    if (auto* camera = GetActiveCameraController(gameContext))

    {

        if (camera->GetCurrentCameraType() == HEIN::CameraType::LockOn)

        {

            camera->RequestSwitch(HEIN::CameraType::Spring);

        }

    }



    // Restore mouse cursor to absolute mode and ensure it is visible for UI interaction
    ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
    ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
    ::SetCursor(::LoadCursor(nullptr, IDC_ARROW));
    DirectX::Mouse::Get().SetMode(DirectX::Mouse::MODE_ABSOLUTE);
    while (ShowCursor(TRUE) < 0);



    // Resolve font path from discovered assets

    std::wstring defaultFontPath = L"";

    const auto discoveredFonts = HEIN::UIButtonComponent::GetDiscoveredFontPaths();

    for (const auto& fontPath : discoveredFonts)

    {

        if (fontPath.find(L"ITZorak") != std::wstring::npos)

        {

            defaultFontPath = fontPath;

            break;

        }

    }

    if (defaultFontPath.empty() && !discoveredFonts.empty())

    {

        defaultFontPath = discoveredFonts.front();

    }



    // Reuse existing persistent GameOverBanner actor from scene data if created in editor
    HEIN::Actor* bannerActor = m_actorManager.GetActor(m_gameOverBannerID);
    if (!bannerActor)
    {
        bannerActor = m_actorManager.GetActorByName(L"GameOverBanner");
    }

    if (bannerActor)
    {
        bannerActor->SetVisible(true);
        bannerActor->SetActive(true);
        m_gameOverBannerID = bannerActor->GetID();
        if (auto* bannerComp = bannerActor->GetComponent<HEIN::UIButtonComponent>())
        {
            bannerComp->SetVisible(true);
        }
    }
    else
    {
        bannerActor = m_actorManager.CreateActor(L"GameOverBanner");
        m_gameOverBannerID = bannerActor->GetID();
        bannerActor->AddComponent<HEIN::TransformComponent>();
        auto* bannerComp = bannerActor->AddComponent<HEIN::UIButtonComponent>();
        bannerComp->SetElementType(HEIN::UIElementType::Text);
        bannerComp->SetText("Y O U   D I E D");
        if (!defaultFontPath.empty())
        {
            bannerComp->SetSpriteFontPath(defaultFontPath, gameContext.deviceResources.GetD3DDevice());
        }
        bannerComp->SetFontSize(1.6f);
        bannerComp->SetTextColor(DirectX::SimpleMath::Vector4(0.85f, 0.12f, 0.12f, 1.0f));
        bannerComp->SetPosition(DirectX::SimpleMath::Vector2(440.0f, 210.0f));
        bannerComp->SetSize(DirectX::SimpleMath::Vector2(400.0f, 80.0f));
        bannerComp->Initialize(gameContext, nullptr, nullptr, nullptr);
        bannerActor->Start();
    }

    // Reuse existing persistent RetryButton actor from scene data
    HEIN::Actor* retryButtonActor = m_actorManager.GetActor(m_retryButtonID);
    if (!retryButtonActor)
    {
        retryButtonActor = m_actorManager.GetActorByName(L"RetryButton");
    }

    if (retryButtonActor)
    {
        retryButtonActor->SetVisible(true);
        retryButtonActor->SetActive(true);
        m_retryButtonID = retryButtonActor->GetID();
        if (auto* retryComp = retryButtonActor->GetComponent<HEIN::UIButtonComponent>())
        {
            retryComp->SetVisible(true);
        }
    }
    else
    {
        retryButtonActor = m_actorManager.CreateActor(L"RetryButton");
        m_retryButtonID = retryButtonActor->GetID();
        retryButtonActor->AddComponent<HEIN::TransformComponent>();
        auto* retryComp = retryButtonActor->AddComponent<HEIN::UIButtonComponent>();
        retryComp->SetElementType(HEIN::UIElementType::Button);
        retryComp->SetText("RETRY");
        if (!defaultFontPath.empty())
        {
            retryComp->SetSpriteFontPath(defaultFontPath, gameContext.deviceResources.GetD3DDevice());
        }
        retryComp->SetFontSize(0.85f);
        retryComp->SetUseEldenStyle(true);
        retryComp->SetPosition(DirectX::SimpleMath::Vector2(500.0f, 320.0f));
        retryComp->SetSize(DirectX::SimpleMath::Vector2(280.0f, 50.0f));
        retryComp->Initialize(gameContext, nullptr, nullptr, nullptr);
        retryButtonActor->Start();
    }

    // Reuse existing persistent BackToTitleButton actor from scene data
    HEIN::Actor* backButtonActor = m_actorManager.GetActor(m_backToTitleButtonID);
    if (!backButtonActor)
    {
        backButtonActor = m_actorManager.GetActorByName(L"BackToTitleButton");
    }

    if (backButtonActor)
    {
        backButtonActor->SetVisible(true);
        backButtonActor->SetActive(true);
        m_backToTitleButtonID = backButtonActor->GetID();
        if (auto* backButtonComp = backButtonActor->GetComponent<HEIN::UIButtonComponent>())
        {
            backButtonComp->SetVisible(true);
        }
    }
    else
    {
        backButtonActor = m_actorManager.CreateActor(L"BackToTitleButton");
        m_backToTitleButtonID = backButtonActor->GetID();
        backButtonActor->AddComponent<HEIN::TransformComponent>();
        auto* backButtonComp = backButtonActor->AddComponent<HEIN::UIButtonComponent>();
        backButtonComp->SetElementType(HEIN::UIElementType::Button);
        backButtonComp->SetText("BACK TO TITLE");
        if (!defaultFontPath.empty())
        {
            backButtonComp->SetSpriteFontPath(defaultFontPath, gameContext.deviceResources.GetD3DDevice());
        }
        backButtonComp->SetFontSize(0.85f);
        backButtonComp->SetUseEldenStyle(true);
        backButtonComp->SetPosition(DirectX::SimpleMath::Vector2(500.0f, 385.0f));
        backButtonComp->SetSize(DirectX::SimpleMath::Vector2(280.0f, 50.0f));
        backButtonComp->Initialize(gameContext, nullptr, nullptr, nullptr);
        backButtonActor->Start();
    }

}



void GameScene::ResetMatch(GameContext& gameContext)

{

    // Hide victory and game over UI elements without destroying user-created scene actors

    if (auto* victoryAct = m_actorManager.GetActor(m_victoryBannerID)) { victoryAct->SetVisible(false); victoryAct->SetActive(false); }

    else if (auto* vAct = m_actorManager.GetActorByName(L"VictoryBanner")) { vAct->SetVisible(false); vAct->SetActive(false); }



    if (auto* gameOverAct = m_actorManager.GetActor(m_gameOverBannerID)) { gameOverAct->SetVisible(false); gameOverAct->SetActive(false); }

    else if (auto* goAct = m_actorManager.GetActorByName(L"GameOverBanner")) { goAct->SetVisible(false); goAct->SetActive(false); }



    if (auto* retryAct = m_actorManager.GetActor(m_retryButtonID)) { retryAct->SetVisible(false); retryAct->SetActive(false); }

    else if (auto* rAct = m_actorManager.GetActorByName(L"RetryButton")) { rAct->SetVisible(false); rAct->SetActive(false); }



    if (auto* backBtn = m_actorManager.GetActor(m_backToTitleButtonID)) { backBtn->SetVisible(false); backBtn->SetActive(false); }

    else if (auto* bAct = m_actorManager.GetActorByName(L"BackToTitleButton")) { bAct->SetVisible(false); bAct->SetActive(false); }



    m_isVictory = false;

    m_victoryTimer = 0.0f;

    m_isGhost = false;

    m_isDying = false;

    m_deathFreezeTimer = 0.0f;

    m_isDeathAnimPlaying = false;

    m_deathAnimTimer = 0.0f;



    // Reset actors and re-link post scene load

    PostSceneLoadInit(gameContext);

    // - Reset all encounter components in scene
    for (const auto& [id, actor] : m_actorManager.GetAllActors())
    {
        if (!actor) continue;
        if (auto* enc = actor->GetComponent<HEIN::EncounterTriggerComponent>())
        {
            enc->ResetEncounter();
        }
    }
    m_activeEncounterActorID = HEIN::INVALID_ACTOR_ID;

    ChangeFlowState(GameFlowState::Gameplay, gameContext);



    if (HEIN::CameraController* camera = GetActiveCameraController(gameContext))

    {

        if (camera->GetSavedCameraType().has_value())

        {

            camera->RequestSwitch(*camera->GetSavedCameraType());

            camera->ApplyRequest();

        }

        camera->UpdateMouseMode();

    }

}

