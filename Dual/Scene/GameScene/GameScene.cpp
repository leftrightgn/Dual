#include "pch.h"
#include "GameScene.h"
#include <Camera/DebugCameraMode.h>
#include <Components/TransformComponent.h>
#include <Components/SkinnedModelComponent.h>
#include <Camera/ThirdPersonMode.h>
#include <Camera/FirstPersonMode.h>
#include <Components/PlayerInputComponent.h>
#include <Components/SocketComponent.h>
#include <Components/StaticModelComponent.h>
#include <Camera/SpringCameraMode.h>
#include <Components/ColliderComponent/OBBColliderComponent.h>
#include <Factory/ActorFactory.h>


using namespace DirectX;

// 更新
void GameScene::Update(Imase::ISceneController<SceneId>& /*sceneController*/, GameContext& gameContext)
{

    float deltaTime = static_cast<float>(gameContext.timer.GetElapsedSeconds());

    m_debugDisplay->Update(gameContext);
    if (m_player != nullptr && !m_debugDisplay->isMagnified())
    {
        HEIN::PlayerInputComponent* inputComp = m_player->GetComponent<HEIN::PlayerInputComponent>();
        if (inputComp)
        {
            inputComp->ProcessInput(gameContext);
        }
    }
    for (std::unique_ptr<HEIN::Actor>& actor : m_actors)
    {
        actor->Update(deltaTime);
    }
    
    // [LOGIC: CAMERA TRACKING]
    // Read the bone position safely to update the camera target
    if (m_player != nullptr)
    {
        HEIN::TransformComponent* pTransform = m_player->GetComponent<HEIN::TransformComponent>();
        HEIN::SkinnedModelComponent* pModel = m_player->GetComponent<HEIN::SkinnedModelComponent>();

        if (pTransform != nullptr && pModel != nullptr)
        {
            SimpleMath::Matrix worldMatrix = pTransform->GetWorldMatrix();
            m_targetPos = pModel->GetBoneWorldPosition(L"mixamorig:HeadTop_End", worldMatrix);
        }
    }
   
    //  Update Environment
    if (m_water)
    {
        m_water->Update(deltaTime);
    }

    if (m_cameraController)
    {
        m_cameraController->Update(deltaTime);
        D3D11_VIEWPORT viewport = gameContext.deviceResources.GetScreenViewport();
        float aspectRatio = static_cast<float>(viewport.Width) / static_cast<float>(viewport.Height);

        m_proj = SimpleMath::Matrix::CreatePerspectiveFieldOfView(
            m_cameraController->GetFov(),
            aspectRatio,
            0.001f,
            1000.0f
        );
    }
}

// 描画
void GameScene::Render(GameContext& gameContext)
{
    ID3D11DeviceContext* context = gameContext.deviceResources.GetD3DDeviceContext();
    SimpleMath::Matrix view = m_cameraController->GetView();
    //SimpleMath::Matrix view = m_debugCamera->GetCameraMatrix();

    SimpleMath::Vector3 camPos = m_cameraController->GetPosition();
    //SimpleMath::Vector3 camPos = m_debugCamera->GetEyePosition();

    if (m_skybox)
    {
        m_skybox->Draw(gameContext, view, m_proj);
    }

    for (std::unique_ptr<HEIN::Actor>& actor : m_actors)
    {
        actor->Draw(gameContext, view, m_proj);
    }
    ID3D11SamplerState* wrapSampler = gameContext.commonStates.LinearWrap();
    context->RSSetState(gameContext.commonStates.CullNone());
    context->PSSetSamplers(0, 1, &wrapSampler);
    context->OMSetBlendState(gameContext.commonStates.AlphaBlend(), nullptr, 0xFFFFFFFF);
    context->OMSetDepthStencilState(gameContext.commonStates.DepthRead(), 0); 

    if (m_water)
    {
        //m_water->Draw(gameContext, view, m_proj, camPos);
    }

    // Cleanup and reset states back to normal for the next frame
    context->RSSetState(gameContext.commonStates.CullCounterClockwise());
    context->OMSetBlendState(gameContext.commonStates.Opaque(), nullptr, 0xFFFFFFFF);
    context->OMSetDepthStencilState(gameContext.commonStates.DepthDefault(), 0);

    m_debugDisplay->Render(gameContext, m_actors, m_skybox.get(), view, m_proj);
}

// シーン切り替え時に呼び出される関数
void GameScene::OnEnter(GameContext& gameContext)
{
    m_skybox = std::make_unique<HEIN::Skybox>();
    m_skybox->Initialize(gameContext, L"Resources/Textures/sky.dds");

    D3D11_VIEWPORT viewport = gameContext.deviceResources.GetScreenViewport();
    int width = static_cast<int>(viewport.Width);
    int height = static_cast<int>(viewport.Height);
    float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
    m_proj = SimpleMath::Matrix::CreatePerspectiveFieldOfView(
        DirectX::XM_PI / 4.0f, aspectRatio, 0.01f, 1000.0f);

    // Water
    m_water = std::make_unique<Water>();
    m_water->Initialize(
        gameContext,
        L"Resources/Textures/water.dds",
        L"Resources/Textures/waternormal.dds",
        L"Resources/Textures/waternoise.dds"
    );

    // Camera
    m_cameraController = std::make_unique<HEIN::CameraController>();

    // Player
    HEIN::PlayerSpawnData playerData = HEIN::ActorFactory::CreateKnight(
        gameContext,
        m_cameraController.get(),
        &m_targetPos
        );

    m_player = playerData.playerActor.get();
    HEIN::SkinnedModelComponent* fpsModelPointer = playerData.fpsModel;
    HEIN::SkinnedModelComponent* tpsModelPointer = playerData.tpsModel;
    
    // Sword
    HEIN::SocketComponent* playerSocket = m_player->GetComponent<HEIN::SocketComponent>();
    std::unique_ptr<HEIN::Actor> sword = HEIN::ActorFactory::CreateSword(gameContext, playerSocket);
    m_swordActor = sword.get();
    
    // Debug Camera Registration
    m_cameraController->RegisterCamera(
        HEIN::CameraType::Debug, 
        []() 
        { return std::make_unique<HEIN::DebugCameraMode>(); }
    );
    HEIN::TransformComponent* playerTransform = m_player->GetComponent<HEIN::TransformComponent>();
    DirectX::SimpleMath::Matrix worldMatrix = playerTransform->GetWorldMatrix();

    m_targetPos = fpsModelPointer->GetBoneWorldPosition(L"mixamorig:Head", worldMatrix);

    // First Person Camera Registration
    m_cameraController->RegisterCamera(
        HEIN::CameraType::FirstPerson, 
        [this, fpsModelPointer, tpsModelPointer]() 
        { return std::make_unique<HEIN::FirstPersonMode>(&m_targetPos, fpsModelPointer, tpsModelPointer); }
    );

    // Third Person Camera Registration
    m_cameraController->RegisterCamera(
        HEIN::CameraType::ThirdPerson,
        [this, fpsModelPointer, tpsModelPointer]()
        { return std::make_unique<HEIN::ThirdPersonMode>(&m_targetPos, fpsModelPointer, tpsModelPointer); }
    );

    // Spring Camera Registration
    m_cameraController->RegisterCamera(
        HEIN::CameraType::Spring,
        [this, playerTransform]()
        { return std::make_unique<HEIN::SpringCameraMode>(playerTransform, &m_targetPos); }
    );

    // Set the Default Camera 
    m_cameraController->SetFirstCamera(HEIN::CameraType::Debug);

    // Stage
    std::unique_ptr<HEIN::Actor> stageActor = std::make_unique<HEIN::Actor>(L"Stage");
    HEIN::TransformComponent* tranStage = stageActor->AddComponent<HEIN::TransformComponent>();

    tranStage->SetPosition(DirectX::SimpleMath::Vector3(0.0f, 1.0f, 0.0f));
    tranStage->SetScale(DirectX::SimpleMath::Vector3(10.0f));

    HEIN::StaticModelComponent* stageModel = stageActor->AddComponent<HEIN::StaticModelComponent>();

    stageModel->Initialize(gameContext, L"Resources/Models/stage/stage.sdkmesh", L"Resources/Models/stage");

    stageActor->Start();

    //
    m_debugDisplay = std::make_unique<HEIN::DebugDisplayController>();
    m_debugDisplay->Initialize();
   

    m_stageActor = stageActor.get();
    m_actors.push_back(std::move(playerData.playerActor));
    m_actors.push_back(std::move(sword));
    m_actors.push_back(std::move(stageActor));
   

    m_debugDisplay->SetDebugTargets(m_player, m_swordActor);
}
