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
#include <Windows.h>
#include <DirectXMath.h>

using namespace DirectX;

void TitleScene::OnEnter(GameContext& gameContext)
{
    // Initialize Skybox
    m_skybox = std::make_unique<HEIN::Skybox>();
    m_skybox->Initialize(gameContext, L"Resources/Textures/skybox.dds");

    // Viewport & Projection Matrix
    D3D11_VIEWPORT viewport = gameContext.deviceResources.GetScreenViewport();
    float aspectRatio = (viewport.Height > 0.0f) ? (static_cast<float>(viewport.Width) / static_cast<float>(viewport.Height)) : 1.777f;
    m_proj = SimpleMath::Matrix::CreatePerspectiveFieldOfView(DirectX::XM_PI / 4.0f, aspectRatio, 0.01f, 1000.0f);

    // Initialize Component Factory
    HEIN::ComponentFactory::Initialize();

    // Auto-load last saved title scene changes if available
    bool hasAutoSave = false;
    std::ifstream autoSaveFile("TitleAutoSave.json");
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

    // Initialize Debug Display / Editor
    m_debugDisplay = std::make_unique<HEIN::DebugDisplayController>();
    m_debugDisplay->Initialize();
    m_debugDisplay->SetDebugTargets(m_cameraID, m_startButtonID, m_exitButtonID);

    // Ensure absolute mouse mode for title menu
    Mouse::Get().SetMode(Mouse::MODE_ABSOLUTE);
}

void TitleScene::OnExit(GameContext& gameContext)
{
    // Auto-save on exit
    std::ofstream autoSave("TitleAutoSave.json");
    if (autoSave.is_open())
    {
        nlohmann::json j = m_actorManager.Serialize();
        autoSave << j.dump(4);
    }

    gameContext.mainCamera = nullptr;
    m_actorManager.ClearAllActors();
}

void TitleScene::Update(GameContext& gameContext)
{
    float deltaTime = static_cast<float>(gameContext.timer.GetElapsedSeconds());
    m_titlePulseTimer += deltaTime;

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
            std::ofstream file(szFile);
            if (file.is_open())
            {
                nlohmann::json j = m_actorManager.Serialize();
                file << j.dump(4);
            }
            std::ofstream autoSave("TitleAutoSave.json");
            if (autoSave.is_open())
            {
                nlohmann::json j = m_actorManager.Serialize();
                autoSave << j.dump(4);
            }
        }
    }
    else if (uiAction == HEIN::EditorAction::AutoSavePressed)
    {
        std::ofstream autoSave("TitleAutoSave.json");
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
    }

    // Periodic Background Auto-Save while in editor mode
    if (m_debugDisplay != nullptr && m_debugDisplay->isMagnified())
    {
        m_autoSaveTimer += deltaTime;
        if (m_autoSaveTimer >= 30.0f)
        {
            m_autoSaveTimer = 0.0f;
            std::ofstream autoSave("TitleAutoSave.json");
            if (autoSave.is_open())
            {
                nlohmann::json j = m_actorManager.Serialize();
                autoSave << j.dump(4);
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

void TitleScene::Render(GameContext& gameContext)
{
    ID3D11DeviceContext* context = gameContext.deviceResources.GetD3DDeviceContext();
    SimpleMath::Matrix view = SimpleMath::Matrix::Identity;

    if (m_cameraController != nullptr && m_actorManager.HasActor(m_cameraID))
    {
        view = m_cameraController->GetView();
    }

    if (m_skybox)
    {
        m_skybox->Draw(gameContext, view, m_proj);
    }

    // Draw all actors (including UI button/image/text SpriteBatch and typography passes)
    m_actorManager.DrawAll(gameContext, view, m_proj);



    // Reset pipeline state
    context->RSSetState(gameContext.commonStates.CullCounterClockwise());
    context->OMSetBlendState(gameContext.commonStates.Opaque(), nullptr, 0xFFFFFFFF);
    context->OMSetDepthStencilState(gameContext.commonStates.DepthDefault(), 0);

    // ImGui / Debug Pass (Hierarchy, Inspector, Gizmos, Viewport, Toolbar)
    m_debugDisplay->Render(gameContext, m_actorManager, m_skybox.get(), view, m_proj);
}
