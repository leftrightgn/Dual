//--------------------------------------------------------------------------------------
// File: GameScene.cpp
//
// 新規シーン作成時の元にするファイル
//
// Date: 2026.4.13
// Author: Hideyasu Imase
//--------------------------------------------------------------------------------------
#include "pch.h"
#include "GameScene.h"
#include <Camera/DebugCameraMode.h>
#include <Components/TransfromComponent.h>
#include <Components/SkinnedModelComponent.h>
#include <Camera/ThirdPersonMode.h>
#include <Camera/FirstPersonMode.h>
#include <BlackBoard/CombatBlackBoard.h>
#include <Components/PlayerInputComponent.h>
#include <Components/CharacterMovementComponent.h>
#include <Components/CombatStateMachineComponent.h>




using namespace DirectX;

// 更新
void GameScene::Update(Imase::ISceneController<SceneId>& /*sceneController*/, GameContext& gameContext)
{
	Imase::DebugRenderer& debugRenderer = gameContext.debugRenderer;

	debugRenderer.DrawText({ 0.0f, 0.0f }, L"GameScene");

    float deltaTime = static_cast<float>(gameContext.timer.GetElapsedSeconds());


    if (!m_actors.empty())
    {
        HEIN::PlayerInputComponent* inputComp = m_actors[0]->GetComponent<HEIN::PlayerInputComponent>();
        if (inputComp)
        {
            inputComp->ProcessInput(gameContext);
        }
    }
   

    // Update Actors FIRST! 

    for (std::unique_ptr<HEIN::Actor>& actor : m_actors)
    {
        actor->Update(deltaTime);

        HEIN::TransformComponent* transform = actor->GetComponent<HEIN::TransformComponent>();
        if (transform != nullptr)
        {
            //DirectX::SimpleMath::Vector3 pos = transform->GetPosition();
            //pos.z -= 10.0f * deltaTime;
            //transform->SetPosition(pos);
        }
    }

    // NOW read the bone position safely
    if (!m_actors.empty())
    {
        HEIN::TransformComponent* pTransform = m_actors[0]->GetComponent<HEIN::TransformComponent>();
        HEIN::SkinnedModelComponent* pModel = m_actors[0]->GetComponent<HEIN::SkinnedModelComponent>();

        if (pTransform != nullptr && pModel != nullptr)
        {
            DirectX::SimpleMath::Matrix worldMatrix = pTransform->GetWorldMatrix();
            m_targetPos = pModel->GetBoneWorldPosition(L"mixamorig:Head", worldMatrix);
        }
    }

    //  Update the Camera using the valid target position
    if (m_cameraController)
    {
        m_cameraController->Update(deltaTime);
        D3D11_VIEWPORT viewport = gameContext.deviceResources.GetScreenViewport();
        float aspectRatio = static_cast<float>(viewport.Width) / static_cast<float>(viewport.Height);

        m_proj = DirectX::SimpleMath::Matrix::CreatePerspectiveFieldOfView(
            m_cameraController->GetFov(),  // gets FOV from current camera mode
            aspectRatio,
            0.01f,
            1000.0f
        );

        m_effect->SetProjection(m_proj);
    }

    //  Update Environment
    if (m_water)
    {
        m_water->Update(deltaTime);
    }

    
  
}

// 描画
void GameScene::Render(GameContext& gameContext)
{
    ID3D11DeviceContext* context = gameContext.deviceResources.GetD3DDeviceContext();


    // Turn OFF depth writing and face culling
    context->OMSetDepthStencilState(gameContext.commonStates.DepthRead(), 0);
    context->RSSetState(gameContext.commonStates.CullNone());
    if (m_cameraController)
    {
        SimpleMath::Matrix view = m_cameraController->GetView();
        m_effect->SetView(view);


    }
    /* if (m_debugCamera)
     {
         SimpleMath::Matrix view = m_debugCamera->GetCameraMatrix();
         m_effect->SetView(view);
     }*/

    m_sky->Draw(m_effect.get(), m_skyInputLayout.Get());

    context->OMSetDepthStencilState(gameContext.commonStates.DepthDefault(), 0);

    ID3D11SamplerState* wrapSampler = gameContext.commonStates.LinearWrap();
    context->RSSetState(gameContext.commonStates.CullNone());
    context->PSSetSamplers(0, 1, &wrapSampler);
    context->OMSetBlendState(gameContext.commonStates.AlphaBlend(), nullptr, 0xFFFFFFFF);
    context->OMSetDepthStencilState(gameContext.commonStates.DepthRead(), 0);
    SimpleMath::Matrix view = m_cameraController->GetView();
    //SimpleMath::Matrix view = m_debugCamera->GetCameraMatrix();

    SimpleMath::Vector3 camPos = m_cameraController->GetPosition();
    //SimpleMath::Vector3 camPos = m_debugCamera->GetEyePosition();

    m_water->Draw(gameContext, view, m_proj, camPos);
    context->RSSetState(gameContext.commonStates.CullCounterClockwise());
    // Reset Blend State
    context->OMSetBlendState(gameContext.commonStates.Opaque(), nullptr, 0xFFFFFFFF);
    context->OMSetDepthStencilState(gameContext.commonStates.DepthDefault(), 0);

    for (std::unique_ptr<HEIN::Actor>& actor : m_actors)
    {
        HEIN::TransformComponent* transformComp = actor->GetComponent<HEIN::TransformComponent>();

        // Grab ALL models attached to this actor
        std::vector<HEIN::SkinnedModelComponent*> models = actor->GetComponents<HEIN::SkinnedModelComponent>();

        if (transformComp != nullptr && !models.empty())
        {
            DirectX::SimpleMath::Matrix world = transformComp->GetWorldMatrix();

            // Loop through and draw them (the SetVisible check we added will hide the inactive one!)
            for (HEIN::SkinnedModelComponent* modelComp : models)
            {
                modelComp->Draw(gameContext, world, view, m_proj);
            }
        }
    }
}

// シーン切り替え時に呼び出される関数
void GameScene::OnEnter(GameContext& gameContext)
{
    ID3D11Device* device = gameContext.deviceResources.GetD3DDevice();

	m_sky = DirectX::GeometricPrimitive::CreateGeoSphere(gameContext.deviceResources.GetD3DDeviceContext(), 2.f, 3, false /*invert for being inside the shape*/);

	m_effect = std::make_unique<DX::SkyboxEffect>(gameContext.deviceResources.GetD3DDevice());

    // array of descriptors — essentially a list of instructions telling the GPU how to read vertex data
    const D3D11_INPUT_ELEMENT_DESC inputElementDesc[] =
    {
        // SemanticName, SemanticIndex, Format, InputSlot, AlignedByteOffset, ...
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    // DXGI_FORMAT_R32G32B32_FLOAT is data type/ R32G32B32 means three 32-bit channels
    // FLOAT means they're floating point numbers. So this reads three floats — your x, y, and z. 
    // If you needed a fourth w component it would be R32G32B32A32_FLOAT.
    // D3D11_INPUT_PER_VERTEX_DATA — tells the GPU to advance to the next vertex's data for each vertex
    // drawn, which is the normal behaviour. The alternative D3D11_INPUT_PER_INSTANCE_DATA is for a more
    // advanced technique called instancing where you draw many copies of a mesh at once.


    // Get the shader bytecode from your effect
    const void* shaderByteCode = nullptr;
    size_t byteCodeLength = 0;
    m_effect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

    // Manually create the layout
    DX::ThrowIfFailed(
        device->CreateInputLayout(
            inputElementDesc,
            std::size(inputElementDesc),
            shaderByteCode,
            byteCodeLength,
            m_skyInputLayout.ReleaseAndGetAddressOf()
        )
    );
    DX::ThrowIfFailed(
        DirectX::CreateDDSTextureFromFile(device, L"Resources/Textures/sky.dds",
            nullptr, m_cubemap.ReleaseAndGetAddressOf()));

    m_effect->SetTexture(m_cubemap.Get());

    D3D11_VIEWPORT viewport = gameContext.deviceResources.GetScreenViewport();
    int width = static_cast<int>(viewport.Width);
    int height = static_cast<int>(viewport.Height);

    //m_debugCamera = std::make_unique<Imase::DebugCamera>(width, height);
    
    float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
    m_proj = DirectX::SimpleMath::Matrix::CreatePerspectiveFieldOfView(
        DirectX::XM_PI / 4.0f, aspectRatio, 0.01f, 1000.0f);

    m_effect->SetProjection(m_proj);
   


    // Water
    m_water = std::make_unique<Water>();
    m_water->Initialize(gameContext, L"Resources/Textures/water.dds", L"Resources/Textures/waternormal.dds", L"Resources/Textures/waternoise.dds");

    m_player = std::make_unique<HEIN::Actor>(L"Player");

    HEIN::TransformComponent* ptransform = m_player->AddComponent<HEIN::TransformComponent>();
    ptransform->SetPosition(DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f));
    ptransform->SetScale(DirectX::SimpleMath::Vector3(0.1f));
   
    DirectX::SimpleMath::Vector3 pos = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f);
    HEIN::SkinnedModelComponent* m_tpsModel = m_player->AddComponent<HEIN::SkinnedModelComponent>();
    
    // ThirdPersonCamera model
    m_tpsModel->Initialize(gameContext,
        L"Resources/Models/knight/knight.sdkmesh", // normal model
        L"Resources/Models/knight");
    m_tpsModel->LoadAnimation("Idle", L"Resources/Models/knight/knightidle.sdkmesh_anim");
    m_tpsModel->LoadAnimation("Walk", L"Resources/Models/knight/knight.sdkmesh_anim");

    // FirstPersonCamera model
    HEIN::SkinnedModelComponent* m_fpsModel = m_player->AddComponent<HEIN::SkinnedModelComponent>();
    m_fpsModel->Initialize(gameContext,
        L"Resources/Models/knight/headless.sdkmesh", // headless/arms model
        L"Resources/Models/knight");
    m_fpsModel->LoadAnimation("Idle", L"Resources/Models/knight/knightidle.sdkmesh_anim");
    m_fpsModel->LoadAnimation("Walk", L"Resources/Models/knight/knight.sdkmesh_anim");


    m_cameraController = std::make_unique<HEIN::CameraController>();

    m_cameraController->RegisterCamera(HEIN::CameraType::Debug, []() { return std::make_unique<HEIN::DebugCameraMode>(); });
    DirectX::SimpleMath::Matrix worldMatrix = ptransform->GetWorldMatrix();
    m_targetPos = m_fpsModel->GetBoneWorldPosition(L"mixamorig:Head", worldMatrix);
    m_cameraController->RegisterCamera(HEIN::CameraType::FirstPerson, [this, m_fpsModel, m_tpsModel]() 
        { return std::make_unique<HEIN::FirstPersonMode>(&m_targetPos, m_fpsModel, m_tpsModel); });
    m_cameraController->RegisterCamera(HEIN::CameraType::ThirdPerson, [this, m_fpsModel, m_tpsModel]() 
        { return std::make_unique<HEIN::ThirdPersonMode>(&m_targetPos, m_fpsModel, m_tpsModel); });

    m_cameraController->SetFirstCamera(HEIN::CameraType::Debug);

    m_player->AddComponent<HEIN::CombatBlackBoard>();
    m_player->AddComponent<HEIN::PlayerInputComponent>(m_cameraController.get()); // Requires the camera controller
    m_player->AddComponent<HEIN::CharacterMovementComponent>();
    m_player->AddComponent<HEIN::CombatStateMachineComponent>();
   
    m_player->Start();

    m_actors.push_back(std::move(m_player));

   
}
