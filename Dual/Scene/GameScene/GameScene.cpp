#include "pch.h"
#include "GameScene.h"
#include <Camera/DebugCameraMode.h>
#include <Camera/ThirdPersonMode.h>
#include <Camera/FirstPersonMode.h>
#include <Camera/SpringCameraMode.h>
#include <Components/TransformComponent.h>
#include <Components/SkinnedModelComponent.h>
#include <Components/PlayerInputComponent.h>
#include <Components/SocketComponent.h>
#include <Factory/ActorFactory.h>

using namespace DirectX;

// --------------------------------------------------------------------------------------
// シーン切り替え時に呼び出される関数 (OnEnter)
// --------------------------------------------------------------------------------------
void GameScene::OnEnter(GameContext& gameContext)
{
    m_physicsSystem = std::make_unique<HEIN::PhysicsSystem>();

    // Skybox
    m_skybox = std::make_unique<HEIN::Skybox>();
    m_skybox->Initialize(gameContext, L"Resources/Textures/sky.dds");

    // Default Projection
    D3D11_VIEWPORT viewport = gameContext.deviceResources.GetScreenViewport();
    float aspectRatio = static_cast<float>(viewport.Width) / static_cast<float>(viewport.Height);
    m_proj = SimpleMath::Matrix::CreatePerspectiveFieldOfView(DirectX::XM_PI / 4.0f, aspectRatio, 0.01f, 1000.0f);

    m_cameraController = std::make_unique<HEIN::CameraController>();

    // -------------------------------------------------------
    // Entity Spawn (Passing the ActorManager)
    // -------------------------------------------------------

    // Build Player
    HEIN::PlayerSpawnData playerData = HEIN::ActorFactory::CreateKnight(
        m_actorManager,
        gameContext,
        m_cameraController.get(),
        &m_targetPos
    );
    m_playerID = playerData.playerID;

    // Build Sword
    m_swordID = HEIN::ActorFactory::CreateSword(m_actorManager, gameContext, m_playerID);

    // Build Enemy
    HEIN::EnemySpawnData enemyData = HEIN::ActorFactory::CreateEnemy(m_actorManager, gameContext);
    m_enemyID = enemyData.enemyID;

    // Build Stage
    m_stageID = HEIN::ActorFactory::CreateStage(m_actorManager, gameContext);

    // -------------------------------------------------------
    // Camera Registration
    // -------------------------------------------------------
    HEIN::SkinnedModelComponent* fpsModelPointer = playerData.fpsModel;
    HEIN::SkinnedModelComponent* tpsModelPointer = playerData.tpsModel;

    HEIN::Actor* player = m_actorManager.GetActor(m_playerID);
    if (player != nullptr)
    {
        HEIN::TransformComponent* playerTransform = player->GetComponent<HEIN::TransformComponent>();
        m_targetPos = fpsModelPointer->GetBoneWorldPosition(L"mixamorig:Head", playerTransform->GetWorldMatrix());

        // First Person Mode
        m_cameraController->RegisterCamera(
            HEIN::CameraType::FirstPerson,
            [this, fpsModelPointer, tpsModelPointer]()
            {
                return std::make_unique<HEIN::FirstPersonMode>(
                    &m_actorManager, m_playerID, &m_targetPos, fpsModelPointer, tpsModelPointer);
            }
        );

        // Third Person Mode
        m_cameraController->RegisterCamera(
            HEIN::CameraType::ThirdPerson,
            [this, fpsModelPointer, tpsModelPointer]()
            {
                return std::make_unique<HEIN::ThirdPersonMode>(
                    &m_actorManager, m_playerID, &m_targetPos, fpsModelPointer, tpsModelPointer);
            }
        );

        // Spring Camera Mode
        m_cameraController->RegisterCamera(
            HEIN::CameraType::Spring,
            [this, playerTransform]()
            {
                return std::make_unique<HEIN::SpringCameraMode>(
                    &m_actorManager, m_playerID, playerTransform, &m_targetPos);
            }
        );
    }

    // Debug Mode
    m_cameraController->RegisterCamera(
        HEIN::CameraType::Debug,
        []() 
        { return std::make_unique<HEIN::DebugCameraMode>(); }
    );

    m_cameraController->SetFirstCamera(HEIN::CameraType::Debug);

    // -------------------------------------------------------
    // UI Tools
    // -------------------------------------------------------
    m_debugDisplay = std::make_unique<HEIN::DebugDisplayController>();
    m_debugDisplay->Initialize();

    
    m_debugDisplay->SetDebugTargets(m_playerID, m_swordID, m_stageID);
}


// --------------------------------------------------------------------------------------
// 更新 (Update)
// --------------------------------------------------------------------------------------
void GameScene::Update(Imase::ISceneController<SceneId>& /*sceneController*/, GameContext& gameContext)
{
    float deltaTime = static_cast<float>(gameContext.timer.GetElapsedSeconds());

    m_debugDisplay->Update(gameContext);

    HEIN::Actor* player = m_actorManager.GetActor(m_playerID);

    // 1. INPUT PHASE
    if (player != nullptr && !m_debugDisplay->isMagnified())
    {
        HEIN::PlayerInputComponent* inputComp = player->GetComponent<HEIN::PlayerInputComponent>();
        if (inputComp)
        {
            inputComp->ProcessInput(gameContext);
        }
    }

    // 2. CORE ENGINE LOOP (Data-Oriented Math Pipeline)
    m_actorManager.UpdateAll(deltaTime);
    m_physicsSystem->UpdateMovement(gameContext, m_actorManager, deltaTime);
    m_actorManager.UpdateAllHierarchies(); // <--- Math Cascades Downwards
    m_actorManager.LateUpdateAll(deltaTime);
    m_physicsSystem->UpdateCollisions(gameContext, m_actorManager, deltaTime);

    // 3. CAMERA TRACKING
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

    if (m_cameraController)
    {
        m_cameraController->Update(deltaTime);
        D3D11_VIEWPORT viewport = gameContext.deviceResources.GetScreenViewport();
        float aspectRatio = static_cast<float>(viewport.Width) / static_cast<float>(viewport.Height);

        m_proj = SimpleMath::Matrix::CreatePerspectiveFieldOfView(m_cameraController->GetFov(), aspectRatio, 0.1f, 1000.0f);
    }

    // 4. MEMORY CLEANUP
    // Delete any Actors whose health dropped to 0 this frame.
    m_actorManager.CleanUpDestroyedActors();
}


// --------------------------------------------------------------------------------------
// 描画 (Render)
// --------------------------------------------------------------------------------------
void GameScene::Render(GameContext& gameContext)
{
    ID3D11DeviceContext* context = gameContext.deviceResources.GetD3DDeviceContext();
    SimpleMath::Matrix view = m_cameraController->GetView();

    if (m_skybox)
    {
        m_skybox->Draw(gameContext, view, m_proj);
    }

    // Tell the Manager to draw all active entities
    m_actorManager.DrawAll(gameContext, view, m_proj);

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