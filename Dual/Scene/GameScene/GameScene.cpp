#include "pch.h"
#include "Common/InputManager.h"
#include "GameScene.h"
#include <Camera/DebugCameraMode.h>
#include <Camera/ThirdPersonMode.h>
#include <Camera/FirstPersonMode.h>
#include <Camera/LockOnCameraMode.h>
#include <Camera/SpringCameraMode.h>
#include <Components/TransformComponent.h>
#include <Components/SkinnedModelComponent.h>
#include <Components/PlayerInputComponent.h>
#include <Factory/ActorFactory.h>
#include <Components/HealthComponent.h>
#include <BlackBoard/CombatBlackBoard.h>
#include <ImGui/imgui.h>

using namespace DirectX;

// --------------------------------------------------------------------------------------
// シーン切り替え時に呼び出される関数 (OnEnter)
// --------------------------------------------------------------------------------------
void GameScene::OnEnter(GameContext& gameContext)
{
    m_physicsSystem = std::make_unique<HEIN::PhysicsSystem>();
    m_damageSystem = std::make_unique<HEIN::DamageSystem>();

    // Skybox
    m_skybox = std::make_unique<HEIN::Skybox>();
    m_skybox->Initialize(gameContext, L"Resources/Textures/skybox.dds");

    // Default Projection
    D3D11_VIEWPORT viewport = gameContext.deviceResources.GetScreenViewport();
    float aspectRatio = static_cast<float>(viewport.Width) / static_cast<float>(viewport.Height);
    m_proj = SimpleMath::Matrix::CreatePerspectiveFieldOfView(DirectX::XM_PI / 4.0f, aspectRatio, 0.01f, 1000.0f);


    // -------------------------------------------------------
   // Camera Registration
   // -------------------------------------------------------

    m_cameraID = HEIN::ActorFactory::CreateMainCamera(m_actorManager);
    // -------------------------------------------------------
    // Entity Spawn (Passing the ActorManager)
    // -------------------------------------------------------

    // Build Player
    HEIN::PlayerSpawnData playerData = HEIN::ActorFactory::CreateKnight(
        m_actorManager,
        gameContext,
        &m_targetPos
    );
    m_playerID = playerData.playerID;

    // Build Sword
    m_playerSwordID = HEIN::ActorFactory::CreateSword(m_actorManager, gameContext, m_playerID, 5.0f);

    // Build Enemy
    HEIN::EnemySpawnData enemyData = HEIN::ActorFactory::CreateEnemy(m_actorManager, gameContext, m_playerID);
    m_enemyID = enemyData.enemyID;
   
    m_enemySwordID = HEIN::ActorFactory::CreateAxe(m_actorManager, gameContext, m_enemyID, 20.0f);

    // Build Stage
    m_stageID = HEIN::ActorFactory::CreateStage(m_actorManager, gameContext);

   

    // Grab the Camera Component so we can register the modes
    HEIN::Actor* cameraActor = m_actorManager.GetActor(m_cameraID);
    HEIN::CameraController* cameraComp = cameraActor->GetComponent<HEIN::CameraController>();
    gameContext.mainCamera = cameraComp;
    //HEIN::SkinnedModelComponent* fpsModelPointer = playerData.fpsModel;
    HEIN::SkinnedModelComponent* ModelPointer = playerData.tpsModel;

    HEIN::Actor* player = m_actorManager.GetActor(m_playerID);
    if (player != nullptr)
    {
        HEIN::TransformComponent* playerTransform = player->GetComponent<HEIN::TransformComponent>();
        ModelPointer->Update(0.0f);
        m_targetPos = ModelPointer->GetBoneWorldPosition(L"mixamorig:HeadTop_End", playerTransform->GetWorldMatrix());

        // First Person Mode
        cameraComp->RegisterCamera(
            HEIN::CameraType::FirstPerson,
            [this, ModelPointer]()
            {
                return std::make_unique<HEIN::FirstPersonMode>(
                    &m_actorManager, m_playerID, &m_targetPos, ModelPointer, ModelPointer);
            }
        );

        // Third Person Mode
        cameraComp->RegisterCamera(
            HEIN::CameraType::ThirdPerson,
            [this, ModelPointer]()
            {
                return std::make_unique<HEIN::ThirdPersonMode>(
                    &m_actorManager, m_playerID, &m_targetPos, ModelPointer, ModelPointer);
            }
        );

        // Spring Camera Mode
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

    // Debug Mode
    cameraComp->RegisterCamera(
        HEIN::CameraType::Debug,
        []() 
        { return std::make_unique<HEIN::DebugCameraMode>(); }
    );
    cameraComp->SetFirstCamera(HEIN::CameraType::Spring);
    

    // -------------------------------------------------------
    // UI Tools
    // -------------------------------------------------------
    m_debugDisplay = std::make_unique<HEIN::DebugDisplayController>();
    m_debugDisplay->Initialize();

    
    m_debugDisplay->SetDebugTargets(m_playerID, m_playerSwordID, m_enemySwordID, m_stageID, m_enemyID);

    gameContext.eventManager->AddTriggerListener(
        [this](const HEIN::TriggerEventPayLoad& payLoad)
        {
            m_damageSystem->HandlTriggerHit(payLoad, m_actorManager);
        }
    );
}


// --------------------------------------------------------------------------------------
// 更新 (Update)
// --------------------------------------------------------------------------------------
void GameScene::Update(Imase::ISceneController<SceneId>& /*sceneController*/, GameContext& gameContext)
{
    float deltaTime = static_cast<float>(gameContext.timer.GetElapsedSeconds());

    m_debugDisplay->Update(gameContext, m_actorManager);

    HEIN::Actor* player = m_actorManager.GetActor(m_playerID);

    // CAMERA INPUT PHASE
    if (!m_debugDisplay->isMagnified() && gameContext.mainCamera != nullptr)
    {
        HEIN::CameraInputState cameraInput;
        const DirectX::Mouse::State& mouseState = gameContext.mouseState;

        cameraInput.mouseX = static_cast<float>(mouseState.x);
        cameraInput.mouseY = static_cast<float>(mouseState.y);
        cameraInput.isLeftMouseDown = mouseState.leftButton;
        cameraInput.scrollWheelDelta = static_cast<float>(mouseState.scrollWheelValue);

        gameContext.mainCamera->ProcessInput(cameraInput);

        // Handle Camera Switching cleanly
        HEIN::CameraType targetCameraType;
        if (gameContext.inputManager->WasCameraSwitchPressed(gameContext, targetCameraType))
        {
            gameContext.mainCamera->RequestSwitch(targetCameraType);
        }
    }

    // PLAYER INPUT PHASE
    if (player != nullptr && !m_debugDisplay->isMagnified())
    {
        gameContext.inputManager->BroadCastPlayerInput(gameContext, m_playerID);

        HEIN::PlayerInputComponent* inputComp = player->GetComponent<HEIN::PlayerInputComponent>();
        if (inputComp)
        {
            inputComp->ProcessInput(gameContext);
        }
    }

    // CORE ENGINE LOOP (Data-Oriented Math Pipeline)
    m_actorManager.UpdateAll(deltaTime);
    m_physicsSystem->UpdateMovement(gameContext, m_actorManager, deltaTime);
    m_actorManager.UpdateAllHierarchies(); //  Math Cascades Downwards
    m_actorManager.LateUpdateAll(deltaTime);
    m_physicsSystem->UpdateCollisions(gameContext, m_actorManager, deltaTime);

    // CAMERA TRACKING
    if (player != nullptr)
    {
        HEIN::TransformComponent* pTransform = player->GetComponent<HEIN::TransformComponent>();
        HEIN::SkinnedModelComponent* pModel = player->GetComponent<HEIN::SkinnedModelComponent>();

        if (pTransform != nullptr && pModel != nullptr)
        {
            // Instantly grab pre-calculated bone position for the camera to use safely
            m_targetPos = pModel->GetBoneWorldPosition(L"mixamorig:HeadTop_End", pTransform->GetWorldMatrix());
        }
    }

    HEIN::CameraController* activeCamera = gameContext.mainCamera;
    if (activeCamera != nullptr)
    {
        D3D11_VIEWPORT viewport = gameContext.deviceResources.GetScreenViewport();
        float aspectRatio = static_cast<float>(viewport.Width) / static_cast<float>(viewport.Height);
        m_proj = DirectX::SimpleMath::Matrix::CreatePerspectiveFieldOfView(activeCamera->GetFov(), aspectRatio, 0.1f, 1000.0f);
    }

    // // ---------------------------------------------------------
    // MEMORY CLEANUP (Garbage Collection)
    // ---------------------------------------------------------
    
    // Loop through every actor safely
    for (const std::pair<const HEIN::ActorID, std::unique_ptr<HEIN::Actor>>& pair : m_actorManager.GetAllActors())
    {
        HEIN::Actor* currentActor = pair.second.get();
        HEIN::HealthComponent* health = currentActor->GetComponent<HEIN::HealthComponent>();

        // If the actor has a HealthComponent AND its health is 0 or less
        if (health != nullptr && health->isDead())
        {
            // Tell the manager to queue this actor for destruction!
            m_actorManager.DestroyID(currentActor->GetID());
        }
    }
    // Delete any Actors whose health dropped to 0 this frame.
    m_actorManager.CleanUpDestroyedActors();
}


// --------------------------------------------------------------------------------------
// 描画 (Render)
// --------------------------------------------------------------------------------------
void GameScene::Render(GameContext& gameContext)
{
    ID3D11DeviceContext* context = gameContext.deviceResources.GetD3DDeviceContext();
    DirectX::SimpleMath::Matrix view = DirectX::SimpleMath::Matrix::Identity;
    
    HEIN::CameraController* activeCamera = gameContext.mainCamera;
    if (activeCamera != nullptr)
    {
        view = activeCamera->GetView();
    }
    if (m_skybox)
    {
        m_skybox->Draw(gameContext, view, m_proj);
    }

    // Tell the Manager to draw all active entities
    m_actorManager.DrawAll(gameContext, view, m_proj);
    HEIN::Actor* player = m_actorManager.GetActor(m_playerID);
    HEIN::Actor* enemy = m_actorManager.GetActor(m_enemyID);
    // COMBAT UI
    ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_Always);

    // Set up flags to make the window static (No moving, No resizing, No collapsing)
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;

    // Begin the window with the new flags
    ImGui::Begin("Combat Status", nullptr, flags);
    if (player != nullptr)
    {
        HEIN::HealthComponent* pHealth = player->GetComponent<HEIN::HealthComponent>();
        if (pHealth != nullptr)
        {
            ImGui::Text("Player Health");
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
            ImGui::ProgressBar(pHealth->GetCurrentHealth() / pHealth->GetMaxHealth(), ImVec2(200.0f, 20.0f));
            ImGui::PopStyleColor();
        }

        HEIN::CombatBlackBoard* pBB = player->GetComponent<HEIN::CombatBlackBoard>();
        if (pBB != nullptr)
        {
            if (pBB->isBlockBroken)
            {
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(1.0f, 0.0f, 0.0f, 1.0f)); // Red bar
                ImGui::ProgressBar(pBB->currentBlockStamina / pBB->maxBlockStamina, ImVec2(200.0f, 20.0f), "");
                ImGui::PopStyleColor();
            }
            else
            {
                ImGui::Text("Block Stamina");
                ImGui::ProgressBar(pBB->currentBlockStamina / pBB->maxBlockStamina, ImVec2(200.0f, 20.0f), "");
            }

            ImGui::Separator();

            if (pBB->dodgeCooldownTimer > 0.0f)
            {
                ImGui::Text("Dodge Recharging...");
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
                ImGui::ProgressBar(1.0f - (pBB->dodgeCooldownTimer / pBB->maxDodgeCooldown), ImVec2(200.0f, 20.0f), "");
                ImGui::PopStyleColor();
            }
            else
            {
                ImGui::Text("Dodge Ready");
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.0f, 0.8f, 1.0f, 1.0f));
                ImGui::ProgressBar(1.0f, ImVec2(200.0f, 20.0f), "");
                ImGui::PopStyleColor();
            }
        }
    }
    else ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "PLAYER DEAD");

    ImGui::Separator();

    if (enemy != nullptr)
    {
        HEIN::HealthComponent* eHealth = enemy->GetComponent<HEIN::HealthComponent>();
        if (eHealth != nullptr)
        {
            ImGui::Text("Enemy Health");
            ImGui::ProgressBar(eHealth->GetCurrentHealth() / eHealth->GetMaxHealth(), ImVec2(200.0f, 20.0f));
            
           /* HEIN::CombatBlackBoard* eBB = enemy->GetComponent<HEIN::CombatBlackBoard>();
            if (eBB != nullptr)
            {
                ImGui::Text("Enemy Node: %s", eBB->activeNodeName.c_str());
            }*/
        }
    }
    else ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "ENEMY DEAD");
    ImGui::End();
    // Transparent Pipeline Setup
    ID3D11SamplerState* wrapSampler = gameContext.commonStates.LinearWrap();
    context->RSSetState(gameContext.commonStates.CullNone());
    context->PSSetSamplers(0, 1, &wrapSampler);
    context->OMSetBlendState(gameContext.commonStates.AlphaBlend(), nullptr, 0xFFFFFFFF);
    context->OMSetDepthStencilState(gameContext.commonStates.DepthRead(), 0);

    // Cleanup and reset states back to normal for the next frame
    context->RSSetState(gameContext.commonStates.CullCounterClockwise());
    context->OMSetBlendState(gameContext.commonStates.Opaque(), nullptr, 0xFFFFFFFF);
    context->OMSetDepthStencilState(gameContext.commonStates.DepthDefault(), 0);

    // ImGui / Debug Pass
    m_debugDisplay->Render(gameContext, m_actorManager, m_skybox.get(), view, m_proj);
}
