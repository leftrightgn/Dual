//--------------------------------------------------------------------------------------
// File: TitleScene.cpp
// Author: HEIN SOE KHANT
//--------------------------------------------------------------------------------------
#include "pch.h"
#include "TitleScene.h"
#include "../../../External/Engine/Camera/DebugCameraMode.h"
#include "../../../External/Engine/Camera/ManualCameraMode.h"
#include "../../../External/Engine/Camera/CameraController.h"
#include "../../../External/Engine/Components/TransformComponent.h"
#include "../../../External/Engine/Framework/GameContext.h"
#include "../../../External/Engine/Scene/SceneManager.h"
#include "../../../External/Engine/ImGui/imgui.h"
#include "../../../External/Engine/ImGui/ImGuizmo.h"
#include "../../../External/Engine/Common/json.hpp"
#include "../../../External/Engine/Factory/ComponentFactory.h"
#include "../../Factory/ActorFactory.h"
#include <commdlg.h>
#include <fstream>
#include <filesystem>
#include <Windows.h>
#include <DirectXMath.h>
#include "../../../External/Engine/Components/LightComponent.h"

namespace
{
    // Primary scene configuration path (DOD scene layout, transforms, actor instances)
    std::string GetTitleSceneDataWritePath()
    {
        namespace fs = std::filesystem;
        std::error_code ec;
        std::string target = "Resources/Data/TitleScene.json";
        if (!fs::exists("Resources", ec) && fs::exists("Dual/Resources", ec))
        {
            target = "Dual/Resources/Data/TitleScene.json";
        }
        fs::create_directories(fs::path(target).parent_path(), ec);
        return target;
    }

    std::string GetTitleAutoSaveWritePath()
    {
        namespace fs = std::filesystem;
        std::error_code ec;
        std::string target = "Resources/Data/TitleAutoSave.json";
        if (!fs::exists("Resources", ec) && fs::exists("Dual/Resources", ec))
        {
            target = "Dual/Resources/Data/TitleAutoSave.json";
        }
        fs::create_directories(fs::path(target).parent_path(), ec);
        return target;
    }
}

using namespace DirectX;

void TitleScene::OnEnter(GameContext& gameContext)
{
    // Initialize Core Pipeline & Shadow System
    m_shadowSystem.Initialize(gameContext.deviceResources.GetD3DDevice(), 2048, 2048);

    // Initialize Skybox
    m_skybox = std::make_unique<HEIN::Skybox>();
    m_skybox->Initialize(gameContext, L"Resources/Textures/skybox.dds");

    // Viewport & Projection Matrix
    D3D11_VIEWPORT viewport = gameContext.deviceResources.GetScreenViewport();
    float aspectRatio = (viewport.Height > 0.0f) ? (static_cast<float>(viewport.Width) / static_cast<float>(viewport.Height)) : 1.777f;
    m_proj = SimpleMath::Matrix::CreatePerspectiveFieldOfView(DirectX::XM_PI / 4.0f, aspectRatio, 0.01f, 1000.0f);

    // Initialize Component Factory and register all game components for reflection/deserialization
    HEIN::ComponentFactory::Initialize();
    HEIN::ActorFactory::RegisterGameComponents();

    gameContext.actorManager = &m_actorManager;
    gameContext.shadowSystem = &m_shadowSystem;

    // Auto-load last saved title scene changes if available
    bool hasAutoSave = false;
    std::vector<std::string> candidates = {
        "Resources/Data/TitleScene.json",
        "Dual/Resources/Data/TitleScene.json",
        "../Dual/Resources/Data/TitleScene.json",
        "Resources/Data/TitleAutoSave.json",
        "Dual/Resources/Data/TitleAutoSave.json",
        "../Dual/Resources/Data/TitleAutoSave.json"
    };

    std::ifstream autoSaveFile;
    for (const auto& path : candidates)
    {
        autoSaveFile.open(path);
        if (autoSaveFile.is_open())
        {
            break;
        }
    }
    if (autoSaveFile.is_open())
    {
        try
        {
            nlohmann::json j;
            autoSaveFile >> j;
            if (j.contains("Actors") && !j["Actors"].empty())
            {
                m_actorManager.ClearAllActors();
                m_actorManager.Deserialize(j);
                m_actorManager.InitializeAfterDeserialize(gameContext);
                for (const auto& pair : m_actorManager.GetAllActors())
                {
                    if (pair.second)
                    {
                        pair.second->Start();
                    }
                }
                hasAutoSave = true;
            }
        }
        catch (...)
        {
            hasAutoSave = false;
        }
    }

    if (!hasAutoSave)
    {
        // Setup Camera Actor
        HEIN::Actor* cameraActor = m_actorManager.CreateActor(L"TitleCamera");
        m_cameraID = cameraActor->GetID();
        cameraActor->AddComponent<HEIN::TransformComponent>();
        m_cameraController = cameraActor->AddComponent<HEIN::CameraController>();
        cameraActor->Start();

        // Centered coordinates
        float screenW = viewport.Width;
        float screenH = viewport.Height;
        float btnW = 280.0f;
        float btnH = 64.0f;
        float btnX = (screenW - btnW) * 0.5f;
        float startBtnY = screenH * 0.55f;
        float exitBtnY = startBtnY + btnH + 24.0f;

        // Setup Title Logo / Banner Text Element
        HEIN::Actor* titleActor = m_actorManager.CreateActor(L"TitleBanner");
        titleActor->AddComponent<HEIN::TransformComponent>();
        auto* titleComp = titleActor->AddComponent<HEIN::UIButtonComponent>();
        titleComp->SetElementType(HEIN::UIElementType::Text);
        titleComp->SetText("D U A L");
        titleComp->SetFontSize(3.0f);
        titleComp->SetTextColor(SimpleMath::Vector4(0.95f, 0.98f, 1.0f, 1.0f));
        titleComp->SetPosition(SimpleMath::Vector2(screenW * 0.38f, screenH * 0.20f));
        titleComp->SetSize(SimpleMath::Vector2(300.0f, 80.0f));
        titleComp->Initialize(gameContext, nullptr, nullptr, nullptr);
        titleActor->Start();

        // Setup Subtitle Text Element
        HEIN::Actor* subtitleActor = m_actorManager.CreateActor(L"Subtitle");
        subtitleActor->AddComponent<HEIN::TransformComponent>();
        auto* subtitleComp = subtitleActor->AddComponent<HEIN::UIButtonComponent>();
        subtitleComp->SetElementType(HEIN::UIElementType::Text);
        subtitleComp->SetText("Action Combat RPG System");
        subtitleComp->SetFontSize(1.2f);
        subtitleComp->SetTextColor(SimpleMath::Vector4(0.7f, 0.8f, 0.95f, 0.9f));
        subtitleComp->SetPosition(SimpleMath::Vector2(screenW * 0.37f, screenH * 0.30f));
        subtitleComp->SetSize(SimpleMath::Vector2(320.0f, 40.0f));
        subtitleComp->Initialize(gameContext, nullptr, nullptr, nullptr);
        subtitleActor->Start();

        // Setup Start Game Button Actor
        HEIN::Actor* startButtonActor = m_actorManager.CreateActor(L"StartButton");
        m_startButtonID = startButtonActor->GetID();
        startButtonActor->AddComponent<HEIN::TransformComponent>();
        m_startButton = startButtonActor->AddComponent<HEIN::UIButtonComponent>();
        m_startButton->SetElementType(HEIN::UIElementType::Button);
        m_startButton->SetText("START GAME");
        m_startButton->SetPosition(SimpleMath::Vector2(btnX, startBtnY));
        m_startButton->SetSize(SimpleMath::Vector2(btnW, btnH));
        m_startButton->Initialize(gameContext, nullptr, nullptr, nullptr);
        startButtonActor->Start();

        // Setup Exit Game Button Actor
        HEIN::Actor* exitButtonActor = m_actorManager.CreateActor(L"ExitButton");
        m_exitButtonID = exitButtonActor->GetID();
        exitButtonActor->AddComponent<HEIN::TransformComponent>();
        m_exitButton = exitButtonActor->AddComponent<HEIN::UIButtonComponent>();
        m_exitButton->SetElementType(HEIN::UIElementType::Button);
        m_exitButton->SetText("QUIT GAME");
        m_exitButton->SetPosition(SimpleMath::Vector2(btnX, exitBtnY));
        m_exitButton->SetSize(SimpleMath::Vector2(btnW, btnH));
        m_exitButton->Initialize(gameContext, nullptr, nullptr, nullptr);
        exitButtonActor->Start();
    }

    // Rebind pointers and camera
    auto startAct = m_actorManager.GetActorByName(L"StartButton");
    if (startAct) { m_startButtonID = startAct->GetID(); m_startButton = startAct->GetComponent<HEIN::UIButtonComponent>(); }
    auto exitAct = m_actorManager.GetActorByName(L"ExitButton");
    if (exitAct) { m_exitButtonID = exitAct->GetID(); m_exitButton = exitAct->GetComponent<HEIN::UIButtonComponent>(); }
    auto camAct = m_actorManager.GetActorByName(L"TitleCamera");
    if (camAct)
    {
        m_cameraID = camAct->GetID();
        m_cameraController = camAct->GetComponent<HEIN::CameraController>();
        if (m_cameraController)
        {
            m_cameraController->RegisterCamera(
                HEIN::CameraType::Manual,
                []() { return std::make_unique<HEIN::ManualCameraMode>(); }
            );
            m_cameraController->RegisterCamera(
                HEIN::CameraType::Debug,
                []() { return std::make_unique<HEIN::DebugCameraMode>(); }
            );
            if (!m_cameraController->GetCurrentCameraType().has_value())
            {
                m_cameraController->SetFirstCamera(HEIN::CameraType::Manual);
            }
            gameContext.mainCamera = m_cameraController;
        }
    }

    // Ensure Ghost mode and animation playback on Ghost model if present
    auto ghostAct = m_actorManager.GetActorByName(L"Ghost");
    if (ghostAct)
    {
        if (auto* model = ghostAct->GetComponent<HEIN::SkinnedModelComponent>())
        {
            model->SetGhostMode(true);
            if (model->HasAnimation("Anim0"))
            {
                model->ChangeAnimation("Anim0");
            }
        }
    }

    // Ensure Player animation playback if present
    auto playerAct = m_actorManager.GetActorByName(L"Player");
    if (playerAct)
    {
        if (auto* model = playerAct->GetComponent<HEIN::SkinnedModelComponent>())
        {
            if (model->HasAnimation("Anim0"))
            {
                model->ChangeAnimation("Anim0");
            }
        }
    }

    // Initialize Debug Display / Editor
    m_debugDisplay = std::make_unique<HEIN::DebugDisplayController>();
    m_debugDisplay->Initialize();
    m_debugDisplay->SetDebugTargets(m_cameraID, m_startButtonID, m_exitButtonID);

    // Ensure absolute mouse mode for title menu
    Mouse::Get().SetMode(Mouse::MODE_ABSOLUTE);
}

void TitleScene::OnExit(GameContext& gameContext)
{
    // Auto-save on exit to primary DOD path and fallback path
    nlohmann::json j = m_actorManager.Serialize();
    std::string serializedStr = j.dump(4);

    std::ofstream primaryFile(GetTitleSceneDataWritePath());
    if (primaryFile.is_open())
    {
        primaryFile << serializedStr;
    }

    std::ofstream autoSave(GetTitleAutoSaveWritePath());
    if (autoSave.is_open())
    {
        autoSave << serializedStr;
    }

    gameContext.actorManager = nullptr;
    gameContext.shadowSystem = nullptr;
    gameContext.mainCamera = nullptr;
    m_actorManager.ClearAllActors();
}

void TitleScene::Update(GameContext& gameContext)
{
    float deltaTime = static_cast<float>(gameContext.timer.GetElapsedSeconds());
    m_titlePulseTimer += deltaTime;

    gameContext.actorManager = &m_actorManager;
    gameContext.shadowSystem = &m_shadowSystem;
    gameContext.isEditorMode = (m_debugDisplay != nullptr && m_debugDisplay->isMagnified()) || !m_isPlaying;

    // Update Debug Display / Editor UI
    m_debugDisplay->Update(gameContext, m_actorManager);

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
                m_actorManager.ClearAllActors();
                m_actorManager.Deserialize(j);
                m_actorManager.InitializeAfterDeserialize(gameContext);
                for (const auto& pair : m_actorManager.GetAllActors())
                {
                    if (pair.second)
                    {
                        pair.second->Start();
                    }
                }

                auto startAct = m_actorManager.GetActorByName(L"StartButton");
                if (startAct) { m_startButtonID = startAct->GetID(); m_startButton = startAct->GetComponent<HEIN::UIButtonComponent>(); }
                auto exitAct = m_actorManager.GetActorByName(L"ExitButton");
                if (exitAct) { m_exitButtonID = exitAct->GetID(); m_exitButton = exitAct->GetComponent<HEIN::UIButtonComponent>(); }
                auto camAct = m_actorManager.GetActorByName(L"TitleCamera");
                if (camAct)
                {
                    m_cameraID = camAct->GetID();
                    m_cameraController = camAct->GetComponent<HEIN::CameraController>();
                    if (m_cameraController)
                    {
                        m_cameraController->RegisterCamera(
                            HEIN::CameraType::Debug,
                            []() { return std::make_unique<HEIN::DebugCameraMode>(); }
                        );
                        m_cameraController->SetFirstCamera(HEIN::CameraType::Debug);
                        gameContext.mainCamera = m_cameraController;
                    }
                }
                m_isPlaying = false;
            }
        }
    }
    else if (uiAction == HEIN::EditorAction::NewScenePressed)
    {
        gameContext.mainCamera = nullptr;
        m_actorManager.ClearAllActors();
        m_startButtonID = HEIN::INVALID_ACTOR_ID;
        m_exitButtonID = HEIN::INVALID_ACTOR_ID;
        m_cameraID = HEIN::INVALID_ACTOR_ID;
        m_startButton = nullptr;
        m_exitButton = nullptr;
        m_cameraController = nullptr;
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
            std::string serializedStr = j.dump(4);

            std::ofstream file(szFile);
            if (file.is_open())
            {
                file << serializedStr;
            }
            std::ofstream primaryFile(GetTitleSceneDataWritePath());
            if (primaryFile.is_open())
            {
                primaryFile << serializedStr;
            }
            std::ofstream autoSave(GetTitleAutoSaveWritePath());
            if (autoSave.is_open())
            {
                autoSave << serializedStr;
            }
        }
    }
    else if (uiAction == HEIN::EditorAction::AutoSavePressed)
    {
        nlohmann::json j = m_actorManager.Serialize();
        std::string serializedStr = j.dump(4);

        std::ofstream primaryFile(GetTitleSceneDataWritePath());
        if (primaryFile.is_open())
        {
            primaryFile << serializedStr;
        }
        std::ofstream autoSave(GetTitleAutoSaveWritePath());
        if (autoSave.is_open())
        {
            autoSave << serializedStr;
        }
    }
    else if (uiAction == HEIN::EditorAction::CreateStagePressed)
    {
        HEIN::ActorID stageID = HEIN::ActorFactory::CreateStage(m_actorManager, gameContext);
        if (m_debugDisplay != nullptr && stageID != HEIN::INVALID_ACTOR_ID)
        {
            m_debugDisplay->GetDebugUI().SetSelectedActor(m_actorManager.GetActor(stageID));
        }
    }

    // Periodic Background Auto-Save while in editor mode
    if (m_debugDisplay != nullptr && m_debugDisplay->isMagnified())
    {
        m_autoSaveTimer += deltaTime;
        if (m_autoSaveTimer >= 30.0f)
        {
            m_autoSaveTimer = 0.0f;
            nlohmann::json j = m_actorManager.Serialize();
            std::string serializedStr = j.dump(4);

            std::ofstream primaryFile(GetTitleSceneDataWritePath());
            if (primaryFile.is_open())
            {
                primaryFile << serializedStr;
            }
            std::ofstream autoSave(GetTitleAutoSaveWritePath());
            if (autoSave.is_open())
            {
                autoSave << serializedStr;
            }
        }
    }

    // Safely sync button and camera pointers from ActorManager handles
    HEIN::Actor* startActor = m_actorManager.GetActor(m_startButtonID);
    m_startButton = startActor ? startActor->GetComponent<HEIN::UIButtonComponent>() : nullptr;

    HEIN::Actor* exitActor = m_actorManager.GetActor(m_exitButtonID);
    m_exitButton = exitActor ? exitActor->GetComponent<HEIN::UIButtonComponent>() : nullptr;

    HEIN::Actor* camActor = m_actorManager.GetActor(m_cameraID);
    m_cameraController = camActor ? camActor->GetComponent<HEIN::CameraController>() : nullptr;
    gameContext.mainCamera = m_cameraController;

    // Update all UI and actors
    m_actorManager.UpdateAll(deltaTime);
    m_actorManager.UpdateAllHierarchies();

    // Check Start Button or Enter/Space keys ONLY when playing in game mode (NOT in editor mode)
    bool isEditorMode = (m_debugDisplay != nullptr && m_debugDisplay->isMagnified()) || !m_isPlaying;
    if (!isEditorMode)
    {
        bool startPressed = (m_startButton && m_startButton->IsClicked());

        if (startPressed)
        {
            if (gameContext.sceneManager)
            {
                gameContext.sceneManager->LoadScene("GameScene");
                return;
            }
        }

        // Check Exit Button or Escape key
        bool exitPressed = (m_exitButton && m_exitButton->IsClicked()) ||
                           gameContext.keyboardTracker.pressed.Escape;

        if (exitPressed)
        {
            PostQuitMessage(0);
            return;
        }
    }

    // Clean up any destroyed actors safely at the end of update
    m_actorManager.CleanUpDestroyedActors();
}

void TitleScene::RenderShadowPhase(GameContext& gameContext, ID3D11DeviceContext* context)
{
    HEIN::LightComponent* activeLight = nullptr;
    for (auto& pair : m_actorManager.GetAllActors())
    {
        activeLight = pair.second->GetComponent<HEIN::LightComponent>();
        if (activeLight && activeLight->CastsShadows()) break;
    }

    DirectX::SimpleMath::Vector3 lightDir(0.5f, -1.0f, 0.5f);
    DirectX::SimpleMath::Vector3 lightPos(-500.0f, 1000.0f, -500.0f);
    float projSize = 500.0f;

    DirectX::SimpleMath::Vector3 targetCenter = DirectX::SimpleMath::Vector3::Zero;
    if (HEIN::Actor* player = m_actorManager.GetActorByName(L"Player"))
    {
        if (auto* trans = player->GetComponent<HEIN::TransformComponent>())
        {
            targetCenter = trans->GetPosition();
        }
    }
    else if (HEIN::Actor* ghost = m_actorManager.GetActorByName(L"Ghost"))
    {
        if (auto* trans = ghost->GetComponent<HEIN::TransformComponent>())
        {
            targetCenter = trans->GetPosition();
        }
    }
    else if (m_cameraController != nullptr && m_actorManager.HasActor(m_cameraID))
    {
        if (HEIN::Actor* camActor = m_actorManager.GetActor(m_cameraID))
        {
            if (auto* trans = camActor->GetComponent<HEIN::TransformComponent>())
            {
                targetCenter = trans->GetPosition() + trans->GetForward() * 50.0f;
            }
        }
    }

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
    else
    {
        lightPos = targetCenter - lightDir * 1000.0f;
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
        DirectX::SimpleMath::Matrix lightViewOrigin = DirectX::SimpleMath::Matrix::CreateLookAt(
            DirectX::SimpleMath::Vector3::Zero,
            lightDir * 1000.0f,
            up
        );

        DirectX::SimpleMath::Vector3 targetInLight = DirectX::SimpleMath::Vector3::Transform(targetCenter, lightViewOrigin);

        const float shadowMapResolution = 2048.0f;
        float worldUnitsPerTexel = projSize / shadowMapResolution;
        targetInLight.x = std::floor(targetInLight.x / worldUnitsPerTexel) * worldUnitsPerTexel;
        targetInLight.y = std::floor(targetInLight.y / worldUnitsPerTexel) * worldUnitsPerTexel;

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

void TitleScene::Render(GameContext& gameContext)
{
    ID3D11DeviceContext* context = gameContext.deviceResources.GetD3DDeviceContext();
    SimpleMath::Matrix view = SimpleMath::Matrix::Identity;

    if (m_cameraController != nullptr && m_actorManager.HasActor(m_cameraID))
    {
        view = m_cameraController->GetView();
        D3D11_VIEWPORT vp = gameContext.deviceResources.GetScreenViewport();
        if (vp.Height > 0.0f)
        {
            float aspectRatio = static_cast<float>(vp.Width) / static_cast<float>(vp.Height);
            m_proj = DirectX::SimpleMath::Matrix::CreatePerspectiveFieldOfView(m_cameraController->GetFov(), aspectRatio, 0.1f, 5000.0f);
        }
    }

    gameContext.actorManager = &m_actorManager;
    gameContext.shadowSystem = &m_shadowSystem;
    gameContext.isEditorMode = (m_debugDisplay != nullptr && m_debugDisplay->isMagnified()) || !m_isPlaying;

    // Pass 1: Shadow Depth Map Pass
    RenderShadowPhase(gameContext, context);

    // Pass 2: Main Color Pass
    ID3D11RenderTargetView* rtv = gameContext.deviceResources.GetRenderTargetView();
    ID3D11DepthStencilView* dsv = gameContext.deviceResources.GetDepthStencilView();
    context->OMSetRenderTargets(1, &rtv, dsv);
    D3D11_VIEWPORT viewport = gameContext.deviceResources.GetScreenViewport();
    context->RSSetViewports(1, &viewport);

    if (m_skybox)
    {
        m_skybox->Draw(gameContext, view, m_proj);
    }

    // Bind Shadow Map SRV (slot 4) & Comparison Sampler (slot 1) for StaticModel / SkinnedModel / Terrain
    ID3D11ShaderResourceView* shadowSRV = m_shadowSystem.GetShadowMapSRV();
    context->PSSetShaderResources(4, 1, &shadowSRV);

    ID3D11SamplerState* shadowSampler = m_shadowSystem.GetShadowSampler();
    context->PSSetSamplers(1, 1, &shadowSampler);

    // Draw all actors (including UI button/image/text SpriteBatch and typography passes)
    m_actorManager.DrawAll(gameContext, view, m_proj);

    // Unbind Shadow Map SRV and Sampler
    ID3D11ShaderResourceView* nullSRV = nullptr;
    context->PSSetShaderResources(4, 1, &nullSRV);

    ID3D11SamplerState* nullSampler = nullptr;
    context->PSSetSamplers(1, 1, &nullSampler);

    // Reset pipeline state
    context->RSSetState(gameContext.commonStates.CullCounterClockwise());
    context->OMSetBlendState(gameContext.commonStates.Opaque(), nullptr, 0xFFFFFFFF);
    context->OMSetDepthStencilState(gameContext.commonStates.DepthDefault(), 0);

    // ImGui / Debug Pass (Hierarchy, Inspector, Gizmos, Viewport, Toolbar)
    m_debugDisplay->Render(gameContext, m_actorManager, m_skybox.get(), view, m_proj);
}
