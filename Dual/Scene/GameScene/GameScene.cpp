
#include "pch.h"
#include "GameScene.h"
#include <Camera/DebugCameraMode.h>
#include <Components/TransformComponent.h>
#include <Components/SkinnedModelComponent.h>
#include <Camera/ThirdPersonMode.h>
#include <Camera/FirstPersonMode.h>
#include <BlackBoard/CombatBlackBoard.h>
#include <Components/PlayerInputComponent.h>
#include <Components/CharacterMovementComponent.h>
#include <Components/CombatStateMachineComponent.h>
#include <Components/SocketComponent.h>
#include <Components/StaticModelComponent.h>
#include <Camera/SpringCameraMode.h>




using namespace DirectX;

// 更新
void GameScene::Update(Imase::ISceneController<SceneId>& /*sceneController*/, GameContext& gameContext)
{
	Imase::DebugRenderer& debugRenderer = gameContext.debugRenderer;

	debugRenderer.DrawText({ 0.0f, 0.0f }, L"GameScene");

    float deltaTime = static_cast<float>(gameContext.timer.GetElapsedSeconds());

    if (m_player != nullptr)
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
    // [LOGIC: DEBUG TOOL]
    // Real-time socket tweaking. (Delete or comment out before shipping the game!)
    /*HEIN::SocketComponent* pSocketComp = m_player->GetComponent<HEIN::SocketComponent>();
    if (pSocketComp != nullptr)
    {
        HEIN::Socket* weaponSocket = pSocketComp->GetSocket(L"WeaponSocket");
        if (weaponSocket != nullptr)
        {
            float moveSpeed = 0.05f;
            if (gameContext.keyboardState.Up)    weaponSocket->localPosition.x += moveSpeed;
            if (gameContext.keyboardState.Down)  weaponSocket->localPosition.x -= moveSpeed;
            if (gameContext.keyboardState.Left)  weaponSocket->localPosition.z += moveSpeed;
            if (gameContext.keyboardState.Right) weaponSocket->localPosition.z -= moveSpeed;

            if (gameContext.keyboardState.F1)
            {
                wchar_t debugMsg[256];
                swprintf_s(debugMsg, L"PERFECT SOCKET: Pos(%.3f, %.3f, %.3f), Rot(%.3f, %.3f, %.3f)\n",
                    weaponSocket->localPosition.x, weaponSocket->localPosition.y, weaponSocket->localPosition.z,
                    weaponSocket->localRotation.x, weaponSocket->localRotation.y, weaponSocket->localRotation.z);
                OutputDebugString(debugMsg);
            }
        }
    }*/
    // [LOGIC: CAMERA TRACKING]
    // Read the bone position safely to update the camera target
    if (m_player != nullptr)
    {
        HEIN::TransformComponent* pTransform = m_player->GetComponent<HEIN::TransformComponent>();
        HEIN::SkinnedModelComponent* pModel = m_player->GetComponent<HEIN::SkinnedModelComponent>();

        if (pTransform != nullptr && pModel != nullptr)
        {
            DirectX::SimpleMath::Matrix worldMatrix = pTransform->GetWorldMatrix();
            m_targetPos = pModel->GetBoneWorldPosition(L"mixamorig:Head", worldMatrix);
        }
    }
    if (m_cameraController)
    {
        m_cameraController->Update(deltaTime);
        D3D11_VIEWPORT viewport = gameContext.deviceResources.GetScreenViewport();
        float aspectRatio = static_cast<float>(viewport.Width) / static_cast<float>(viewport.Height);

        m_proj = DirectX::SimpleMath::Matrix::CreatePerspectiveFieldOfView(
            m_cameraController->GetFov(),
            aspectRatio,
            0.01f,
            1000.0f
        );
    }

    //  Update Environment
    if (m_water)
    {
        m_water->Update(deltaTime);
    }

    if (m_swordActor != nullptr && m_player != nullptr)
    {
        HEIN::SocketComponent* pSocketComp = m_player->GetComponent<HEIN::SocketComponent>();
        HEIN::TransformComponent* swordTrans = m_swordActor->GetComponent<HEIN::TransformComponent>();

        if (pSocketComp != nullptr && pSocketComp->HasSocket(L"WeaponSocket") && swordTrans != nullptr)
        {
           
            DirectX::SimpleMath::Matrix socketWorld = pSocketComp->GetSocketWorldMatrix(L"WeaponSocket");

            swordTrans->SetParentMatrix(socketWorld);
        }
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
        m_water->Draw(gameContext, view, m_proj, camPos);
    }

    // Cleanup and reset states back to normal for the next frame
    context->RSSetState(gameContext.commonStates.CullCounterClockwise());
    context->OMSetBlendState(gameContext.commonStates.Opaque(), nullptr, 0xFFFFFFFF);
    context->OMSetDepthStencilState(gameContext.commonStates.DepthDefault(), 0);

    
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
    m_proj = DirectX::SimpleMath::Matrix::CreatePerspectiveFieldOfView(
        DirectX::XM_PI / 4.0f, aspectRatio, 0.01f, 1000.0f);

    // Water
    m_water = std::make_unique<Water>();
    m_water->Initialize(gameContext, L"Resources/Textures/water.dds", L"Resources/Textures/waternormal.dds", L"Resources/Textures/waternoise.dds");

    std::unique_ptr<HEIN::Actor> playerActor = std::make_unique<HEIN::Actor>(L"Player");

    HEIN::TransformComponent* ptransform = playerActor->AddComponent<HEIN::TransformComponent>();
    ptransform->SetPosition(DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f));
    ptransform->SetScale(DirectX::SimpleMath::Vector3(0.1f));
   
    DirectX::SimpleMath::Vector3 pos = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f);
    HEIN::SkinnedModelComponent* m_tpsModel = playerActor->AddComponent<HEIN::SkinnedModelComponent>();
    
    // ThirdPersonCamera model
    m_tpsModel->Initialize(gameContext,
        L"Resources/Models/knight/knight.sdkmesh", // normal model
        L"Resources/Models/knight");
    m_tpsModel->LoadAnimation("Idle", L"Resources/Models/knight/knightidle.sdkmesh_anim");
    m_tpsModel->LoadAnimation("Walk", L"Resources/Models/knight/knight.sdkmesh_anim");
    m_tpsModel->LoadAnimation("OneHand", L"Resources/Models/knight/OneHand.sdkmesh_anim");

    // FirstPersonCamera model
    HEIN::SkinnedModelComponent* m_fpsModel = playerActor->AddComponent<HEIN::SkinnedModelComponent>();
    m_fpsModel->Initialize(gameContext,
        L"Resources/Models/knight/headless.sdkmesh", // headless/arms model
        L"Resources/Models/knight");
    m_fpsModel->LoadAnimation("Idle", L"Resources/Models/knight/knightidle.sdkmesh_anim");
    m_fpsModel->LoadAnimation("Walk", L"Resources/Models/knight/knight.sdkmesh_anim");
    m_fpsModel->LoadAnimation("OneHand", L"Resources/Models/knight/OneHand.sdkmesh_anim");


    HEIN::SocketComponent* socketComp = playerActor->AddComponent<HEIN::SocketComponent>();
    HEIN::Socket weaponSocket(
        L"WeaponSocket",
        L"mixamorig:RightHand",
        DirectX::SimpleMath::Vector3(-7.800f, -2.150f, 13.7f),
        DirectX::SimpleMath::Vector3(DirectX::XM_PIDIV2, 0.0f, 0.0f)
    );
    socketComp->AddSocket(weaponSocket);

    std::unique_ptr<HEIN::Actor> sword = std::make_unique<HEIN::Actor>(L"Sword");
    HEIN::TransformComponent* swordTransform = sword->AddComponent<HEIN::TransformComponent>();

    swordTransform->SetScale(DirectX::SimpleMath::Vector3(2.0f));

    HEIN::StaticModelComponent* swordModel = sword->AddComponent<HEIN::StaticModelComponent>();
    swordModel->Initialize(gameContext, L"Resources/Models/knight/sword.sdkmesh", L"Resources/Models/knight");

    sword->Start();


    m_cameraController = std::make_unique<HEIN::CameraController>();

    m_cameraController->RegisterCamera(HEIN::CameraType::Debug, []() { return std::make_unique<HEIN::DebugCameraMode>(); });
    DirectX::SimpleMath::Matrix worldMatrix = ptransform->GetWorldMatrix();
    m_targetPos = m_fpsModel->GetBoneWorldPosition(L"mixamorig:Head", worldMatrix);
    m_cameraController->RegisterCamera(HEIN::CameraType::FirstPerson, [this, m_fpsModel, m_tpsModel]() 
        { return std::make_unique<HEIN::FirstPersonMode>(&m_targetPos, m_fpsModel, m_tpsModel); });
    m_cameraController->RegisterCamera(HEIN::CameraType::ThirdPerson, [this, m_fpsModel, m_tpsModel]() 
        { return std::make_unique<HEIN::ThirdPersonMode>(&m_targetPos, m_fpsModel, m_tpsModel); });
    m_cameraController->RegisterCamera(HEIN::CameraType::Spring, [this, ptransform]()
        { return std::make_unique<HEIN::SpringCameraMode>(ptransform, &m_targetPos); });

    m_cameraController->SetFirstCamera(HEIN::CameraType::Debug);

    playerActor->AddComponent<HEIN::CombatBlackBoard>();
    playerActor->AddComponent<HEIN::PlayerInputComponent>(m_cameraController.get()); // Requires the camera controller
    playerActor->AddComponent<HEIN::CharacterMovementComponent>();
    playerActor->AddComponent<HEIN::CombatStateMachineComponent>();
   
    playerActor->Start();
    m_swordActor = sword.get();
    m_player = playerActor.get();
    m_actors.push_back(std::move(playerActor));
    m_actors.push_back(std::move(sword));
    

   
}
