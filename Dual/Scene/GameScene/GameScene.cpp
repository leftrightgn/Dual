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


using namespace DirectX;

// 更新
void GameScene::Update(Imase::ISceneController<SceneId>& sceneController, GameContext& gameContext)
{
	Imase::DebugRenderer& debugRenderer = gameContext.debugRenderer;

	debugRenderer.DrawText({ 0.0f, 0.0f }, L"GameScene");

    float deltaTime = static_cast<float>(gameContext.timer.GetElapsedSeconds());

    m_targetPos = m_world.Translation();


    if (m_cameraController)
    {
        m_cameraController->Update(deltaTime);
    }
    m_water->Update(gameContext.timer.GetElapsedSeconds());
    m_animation.Update(gameContext.timer.GetElapsedSeconds());
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
   
    m_sky->Draw(m_effect.get(), m_skyInputLayout.Get());

    context->OMSetDepthStencilState(gameContext.commonStates.DepthDefault(), 0);

    ID3D11SamplerState* wrapSampler = gameContext.commonStates.LinearWrap();
    context->RSSetState(gameContext.commonStates.CullNone());
    context->PSSetSamplers(0, 1, &wrapSampler);
    context->OMSetBlendState(gameContext.commonStates.AlphaBlend(), nullptr, 0xFFFFFFFF);
    context->OMSetDepthStencilState(gameContext.commonStates.DepthRead(), 0);
    SimpleMath::Matrix view = m_cameraController->GetView();

    SimpleMath::Vector3 camPos = m_cameraController->GetPosition();
 
    m_water->Draw(context, view, m_proj, camPos);
    context->RSSetState(gameContext.commonStates.CullCounterClockwise());
    // Reset Blend State
    context->OMSetBlendState(gameContext.commonStates.Opaque(), nullptr, 0xFFFFFFFF);
    context->OMSetDepthStencilState(gameContext.commonStates.DepthDefault(), 0);

    size_t nbones = m_model->bones.size();

    m_animation.Apply(*m_model, nbones, m_drawBones.get());



    m_model->DrawSkinned(context, gameContext.commonStates, nbones, m_drawBones.get(),
        m_world, view, m_proj);

    
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

    m_cameraController = std::make_unique<CameraController>();

    m_targetPos = DirectX::SimpleMath::Vector3(0.0f, 1.5f, 0.0f);

    m_cameraController->SetMode(std::make_unique<CinematicMode>(m_targetPos, 8.0f, 0.0f, 1.0f, 10.0f));

    float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
    m_proj = DirectX::SimpleMath::Matrix::CreatePerspectiveFieldOfView(
        DirectX::XM_PI / 4.0f, aspectRatio, 0.1f, 1000.0f);

    m_effect->SetProjection(m_proj);
    m_world = DirectX::SimpleMath::Matrix::CreateScale(10.1f);


    // Water
    m_water = std::make_unique<Water>();
    m_water->Initialize(device, gameContext.deviceResources.GetD3DDeviceContext(), L"Resources/Textures/water.dds", L"Resources/Textures/waternormal.dds", L"Resources/Textures/waternoise.dds");

    m_fxFactory = std::make_unique<EffectFactory>(device);
    static_cast<EffectFactory*>(m_fxFactory.get())->SetDirectory(L"Resources/Models/soldier");

    
    m_model = Model::CreateFromSDKMESH(device, L"Resources/Models/soldier/soldier.sdkmesh",
        *m_fxFactory,
        static_cast<ModelLoaderFlags>(ModelLoader_Clockwise | ModelLoader_IncludeBones));

    DX::ThrowIfFailed(
        m_animation.Load(L"Resources/Models/soldier/soldier.sdkmesh_anim")
    );
    m_animation.Bind(*m_model);

    m_drawBones = ModelBone::MakeArray(m_model->bones.size());
    

}
