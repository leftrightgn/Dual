#include "pch.h"
#include "CombatStateMachineComponent.h"
#include <States/ICombatState.h>
#include "../../../External/Engine/Entities/Actor.h"
#include "../../External/Engine/Components/ColliderComponent/ColliderComponent.h"
#include "../../External/Engine/Message/Messenger.h" 
#include <BlackBoard/CombatBlackBoard.h>
#include "../../External/Engine/Components/DamageDealerComponent.h"
#include "../../External/Engine/Components/HealthComponent.h"
#include "../../External/Engine/Components/TargetTrackingComponent.h"
#include <ImGui/imgui.h>

HEIN::CombatStateMachineComponent::CombatStateMachineComponent(Actor* owner) : IComponent(owner) {}

void HEIN::CombatStateMachineComponent::Start()
{
    Messenger::GetInstance()->Register(m_owner->GetID(), this);
}

void HEIN::CombatStateMachineComponent::Update(float deltaTime)
{
    ApplyPendingState();
    ProcessBuffer(deltaTime);

    if (m_pendingState != nullptr) return;

    if (m_currentState)
    {
        m_currentState->Update(m_owner, this, deltaTime);
    }

    Messenger::GetInstance()->Notify(m_owner->GetID(), IsAttacking() ? Message::SET_WEAPON_ACTIVE : Message::SET_WEAPON_INACTIVE);

    HEIN::HealthComponent* health = m_owner->GetComponent<HEIN::HealthComponent>();
    if (health) health->SetGameplayInvincible(IsBlocking());

    HEIN::CombatBlackBoard* blackboard = m_owner->GetComponent<CombatBlackBoard>();
    if (blackboard)
    {
        if (!IsBlocking() && blackboard->currentBlockStamina < blackboard->maxBlockStamina)
        {
            blackboard->currentBlockStamina += blackboard->blockRecoveryRate * deltaTime;
            if (blackboard->currentBlockStamina >= blackboard->maxBlockStamina)
            {
                blackboard->currentBlockStamina = blackboard->maxBlockStamina;
                blackboard->isBlockBroken = false;
            }
        }

        if (blackboard->dodgeCooldownTimer > 0.0f)
        {
            blackboard->dodgeCooldownTimer -= deltaTime;
            if (blackboard->dodgeCooldownTimer < 0.0f) blackboard->dodgeCooldownTimer = 0.0f;
        }

        HEIN::TargetTrackingComponent* tracking = m_owner->GetComponent<HEIN::TargetTrackingComponent>();
        if (tracking)
        {
            tracking->SetLockedOn(blackboard->isLockedOn);
            blackboard->lockedTargetID = tracking->GetTargetID();
            blackboard->dirToTarget = tracking->GetDirToTarget();
            blackboard->distanceToTarget = tracking->GetDistanceToTarget();
            blackboard->isLockedOn = tracking->IsLockedOn();
        }
    }
}

void HEIN::CombatStateMachineComponent::ChangeState(const std::string& stateName, float blendDuration)
{
    auto it = m_states.find(stateName);
    if (it != m_states.end())
    {
        m_pendingState = it->second.get();
        m_pendingBlendDuration = blendDuration;
    }
}

void HEIN::CombatStateMachineComponent::ApplyPendingState()
{
    if (m_pendingState == nullptr) return;

    if (m_currentState != nullptr)
    {
        m_currentState->OnExit(m_owner, this);
    }

    m_currentState = m_pendingState;
    m_pendingState = nullptr;

    for (const auto& [name, statePtr] : m_states)
    {
        if (statePtr.get() == m_currentState)
        {
            m_currentStateName = name;
            break;
        }
    }

    if (m_currentState != nullptr)
    {
        // Inject the precise blend duration specific to the arrow that caused this transition
        m_currentState->OnEnter(m_owner, this, m_pendingBlendDuration);
    }
}

void HEIN::CombatStateMachineComponent::AddState(const StateConfig& config)
{
    m_stateConfigs[config.stateName] = config;
    m_states[config.stateName] = CreateCombatStateFromConfig(config);

    if (m_currentState == nullptr && m_pendingState == nullptr)
    {
        m_defaultStateName = config.stateName;
        ChangeState(config.stateName, 0.0f); // Instant snap on first frame spawn
    }
}

void HEIN::CombatStateMachineComponent::AddState(const std::string& stateName, std::unique_ptr<ICombatState> state, const StateConfig& config)
{
    m_stateConfigs[stateName] = config;
    m_states[stateName] = std::move(state);

    if (m_currentState == nullptr && m_pendingState == nullptr)
    {
        m_defaultStateName = stateName;
        ChangeState(stateName, 0.0f);
    }
}

void HEIN::CombatStateMachineComponent::RebuildAllStates()
{
    m_states.clear();
    m_currentState = nullptr;
    m_pendingState = nullptr;

    for (const auto& [name, config] : m_stateConfigs)
    {
        m_states[name] = CreateCombatStateFromConfig(config);
    }

    if (!m_defaultStateName.empty() && m_states.contains(m_defaultStateName))
    {
        ChangeState(m_defaultStateName, 0.0f);
        ApplyPendingState();
    }
}

void HEIN::CombatStateMachineComponent::OnMessageAccepted(Message::MessageID messageID)
{
    if (messageID == Message::PLAYER_ACTION_ATTACK ||
        messageID == Message::PLAYER_ACTION_BLOCK ||
        messageID == Message::PLAYER_STOP_BLOCK ||
        messageID == Message::PLAYER_ACTION_DODGE)
    {
        if (messageID == Message::PLAYER_ACTION_DODGE)
        {
            HEIN::CombatBlackBoard* blackboard = m_owner->GetComponent<CombatBlackBoard>();
            if (blackboard && blackboard->dodgeCooldownTimer > 0.0f) return;
        }

        m_messageBuffer.clear();
        m_messageBuffer.push_back(messageID);
        m_bufferTime = MAX_BUFFER_TIME;
    }
}

void HEIN::CombatStateMachineComponent::ProcessBuffer(float deltaTime)
{
    if (m_messageBuffer.empty()) return;

    m_bufferTime -= deltaTime;
    if (m_bufferTime <= 0.0f)
    {
        m_messageBuffer.clear();
        return;
    }

    Message::MessageID currentMessage = m_messageBuffer.front();
    if (m_currentState != nullptr)
    {
        bool wasHandled = m_currentState->HandleMessage(m_owner, this, currentMessage);
        if (wasHandled)
        {
            m_messageBuffer.clear();
        }
    }
}

bool HEIN::CombatStateMachineComponent::IsAttacking() const
{
    if (m_currentState != nullptr) return m_currentState->IsAttackState();
    return false;
}

bool HEIN::CombatStateMachineComponent::IsBlocking() const
{
    if (m_currentState != nullptr) return m_currentState->IsBlockState();
    return false;
}

void HEIN::CombatStateMachineComponent::OnTriggerOverLap(const HEIN::TriggerEventPayLoad& payLoad)
{
    // Keeping your original hit detection payload stub
    if (payLoad.triggerA->GetOwner() != GetOwner() && payLoad.triggerB->GetOwner() != GetOwner()) return;

    HEIN::ColliderComponent* myHurtBox = nullptr;
    HEIN::ColliderComponent* enemyHitBox = nullptr;

    if (payLoad.triggerA->GetOwner() == GetOwner())
    {
        myHurtBox = payLoad.triggerA;
        enemyHitBox = payLoad.triggerB;
    }
    else
    {
        myHurtBox = payLoad.triggerB;
        enemyHitBox = payLoad.triggerA;
    }

    if (enemyHitBox->GetColliderTag() == L"SwordHitbox") {}
}

nlohmann::json HEIN::CombatStateMachineComponent::Serialize()
{
    nlohmann::json data = IComponent::Serialize();

    data["defaultState"] = m_defaultStateName;
    data["states"] = nlohmann::json::array();

    for (const auto& [name, cfg] : m_stateConfigs)
    {
        nlohmann::json stateJson;
        stateJson["name"] = cfg.stateName;
        stateJson["type"] = cfg.stateType;
        stateJson["animationName"] = cfg.animationName;
        stateJson["secondaryAnimationName"] = cfg.secondaryAnimationName;
        stateJson["moveSpeed"] = cfg.moveSpeed;
        stateJson["stateDuration"] = cfg.stateDuration;
        stateJson["turnSpeed"] = cfg.turnSpeed;
        stateJson["isLooping"] = cfg.isLooping;
        stateJson["isAttack"] = cfg.isAttack;
        stateJson["isBlock"] = cfg.isBlock;
        stateJson["lockMovementDirection"] = cfg.lockMovementDirection;
        stateJson["invincibilityStart"] = cfg.invincibilityStart;
        stateJson["invincibilityEnd"] = cfg.invincibilityEnd;

        stateJson["transitions"] = nlohmann::json::object();
        for (const auto& [eventKey, transition] : cfg.transitions)
        {
            stateJson["transitions"][eventKey] = {
                {"targetState", transition.targetState},
                {"blendDuration", transition.blendDuration},
                {"triggerEvent", transition.triggerEvent},
                {"hasExitTime", transition.hasExitTime},
                {"windowStart", transition.windowStart},
                {"windowEnd", transition.windowEnd},
                {"canInterrupt", transition.canInterrupt}
            };
        }

        stateJson["comboAnimationNames"] = cfg.comboAnimationNames;
        stateJson["comboEndTimes"] = cfg.comboEndTimes;
        stateJson["comboWindowStarts"] = cfg.comboWindowStarts;
        stateJson["comboBlendDurations"] = cfg.comboBlendDurations;
        stateJson["comboExitBlendDuration"] = cfg.comboExitBlendDuration;

        data["states"].push_back(stateJson);
    }
    return data;
}

void HEIN::CombatStateMachineComponent::Deserialize(const nlohmann::json& data)
{
    IComponent::Deserialize(data);

    m_stateConfigs.clear();

    if (data.contains("defaultState") && data["defaultState"].is_string())
    {
        m_defaultStateName = data["defaultState"].get<std::string>();
    }

    if (data.contains("states") && data["states"].is_array())
    {
        for (const auto& stateJson : data["states"])
        {
            StateConfig cfg;

            if (stateJson.contains("name")) cfg.stateName = stateJson["name"].get<std::string>();
            if (stateJson.contains("type")) cfg.stateType = stateJson["type"].get<std::string>();
            if (stateJson.contains("animationName")) cfg.animationName = stateJson["animationName"].get<std::string>();
            if (stateJson.contains("secondaryAnimationName")) cfg.secondaryAnimationName = stateJson["secondaryAnimationName"].get<std::string>();
            if (stateJson.contains("moveSpeed")) cfg.moveSpeed = stateJson["moveSpeed"].get<float>();
            if (stateJson.contains("stateDuration")) cfg.stateDuration = stateJson["stateDuration"].get<float>();
            if (stateJson.contains("turnSpeed")) cfg.turnSpeed = stateJson["turnSpeed"].get<float>();
            if (stateJson.contains("isLooping")) cfg.isLooping = stateJson["isLooping"].get<bool>();
            if (stateJson.contains("isAttack")) cfg.isAttack = stateJson["isAttack"].get<bool>();
            if (stateJson.contains("isBlock")) cfg.isBlock = stateJson["isBlock"].get<bool>();
            if (stateJson.contains("lockMovementDirection")) cfg.lockMovementDirection = stateJson["lockMovementDirection"].get<bool>();
            if (stateJson.contains("invincibilityStart")) cfg.invincibilityStart = stateJson["invincibilityStart"].get<float>();
            if (stateJson.contains("invincibilityEnd")) cfg.invincibilityEnd = stateJson["invincibilityEnd"].get<float>();

            if (stateJson.contains("transitions") && stateJson["transitions"].is_object())
            {
                for (auto it = stateJson["transitions"].begin(); it != stateJson["transitions"].end(); ++it)
                {
                    TransitionData tData;
                    if (it.value().contains("targetState")) tData.targetState = it.value()["targetState"].get<std::string>();
                    if (it.value().contains("blendDuration")) tData.blendDuration = it.value()["blendDuration"].get<float>();
                    if (it.value().contains("triggerEvent")) tData.triggerEvent = it.value()["triggerEvent"].get<std::string>();
                    else tData.triggerEvent = it.key();
                    if (it.value().contains("hasExitTime")) tData.hasExitTime = it.value()["hasExitTime"].get<bool>();
                    if (it.value().contains("windowStart")) tData.windowStart = it.value()["windowStart"].get<float>();
                    if (it.value().contains("windowEnd")) tData.windowEnd = it.value()["windowEnd"].get<float>();
                    if (it.value().contains("canInterrupt")) tData.canInterrupt = it.value()["canInterrupt"].get<bool>();

                    cfg.transitions[it.key()] = tData;
                }
            }

            if (stateJson.contains("comboAnimationNames") && stateJson["comboAnimationNames"].is_array())
            {
                cfg.comboAnimationNames = stateJson["comboAnimationNames"].get<std::vector<std::string>>();
            }

            if (stateJson.contains("comboEndTimes") && stateJson["comboEndTimes"].is_array())
            {
                cfg.comboEndTimes = stateJson["comboEndTimes"].get<std::vector<float>>();
            }

            if (stateJson.contains("comboWindowStarts") && stateJson["comboWindowStarts"].is_array())
            {
                cfg.comboWindowStarts = stateJson["comboWindowStarts"].get<std::vector<float>>();
            }

            if (stateJson.contains("comboBlendDurations") && stateJson["comboBlendDurations"].is_array())
            {
                cfg.comboBlendDurations = stateJson["comboBlendDurations"].get<std::vector<float>>();
            }

            if (stateJson.contains("comboExitBlendDuration") && stateJson["comboExitBlendDuration"].is_number())
            {
                cfg.comboExitBlendDuration = stateJson["comboExitBlendDuration"].get<float>();
            }

            m_stateConfigs[cfg.stateName] = cfg;
        }
    }
    RebuildAllStates();
}

void HEIN::CombatStateMachineComponent::OnInspectorGUI(GameContext& /*gameContext*/)
{
    if (ImGui::CollapsingHeader("Combat State Machine Component", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Active State: %s", m_currentStateName.c_str());
        ImGui::Separator();

        bool configChanged = false;
        std::string stateToRemove = "";

        for (auto& [name, cfg] : m_stateConfigs)
        {
            ImGui::PushID(name.c_str());
            if (ImGui::TreeNode(name.c_str()))
            {
                char animBuf[128];
                strcpy_s(animBuf, cfg.animationName.c_str());
                if (ImGui::InputText("Primary Animation", animBuf, sizeof(animBuf)))
                {
                    cfg.animationName = animBuf;
                    configChanged = true;
                }

                char secAnimBuf[128];
                strcpy_s(secAnimBuf, cfg.secondaryAnimationName.c_str());
                if (ImGui::InputText("Secondary Animation", secAnimBuf, sizeof(secAnimBuf)))
                {
                    cfg.secondaryAnimationName = secAnimBuf;
                    configChanged = true;
                }

                if (ImGui::DragFloat("Move Speed", &cfg.moveSpeed, 0.5f, 0.0f, 100.0f)) configChanged = true;
                if (ImGui::DragFloat("State Duration", &cfg.stateDuration, 0.05f, 0.0f, 10.0f)) configChanged = true;
                if (ImGui::DragFloat("Turn Speed", &cfg.turnSpeed, 0.5f, 0.0f, 50.0f)) configChanged = true;

                if (ImGui::Checkbox("Is Looping", &cfg.isLooping)) configChanged = true;
                ImGui::SameLine();
                if (ImGui::Checkbox("Is Attack", &cfg.isAttack)) configChanged = true;
                ImGui::SameLine();
                if (ImGui::Checkbox("Is Block", &cfg.isBlock)) configChanged = true;
                if (ImGui::Checkbox("Lock Direction", &cfg.lockMovementDirection)) configChanged = true;

                if (ImGui::DragFloatRange2("Invincibility Window", &cfg.invincibilityStart, &cfg.invincibilityEnd, 0.05f, 0.0f, 10.0f, "Start: %.2f", "End: %.2f"))
                {
                    configChanged = true;
                }

                if (ImGui::TreeNode("Transitions (Arrows / Links)"))
                {
                    if (ImGui::Button("Quick: Set ALL to Instant Cancel (Skip Anim)"))
                    {
                        for (auto& [k, t] : cfg.transitions)
                        {
                            if (!t.hasExitTime) t.canInterrupt = true;
                        }
                        configChanged = true;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Quick: Reset to Timed Windows"))
                    {
                        for (auto& [k, t] : cfg.transitions)
                        {
                            if (!t.hasExitTime) { t.canInterrupt = false; t.windowStart = 0.4f; t.windowEnd = 1.1f; }
                        }
                        configChanged = true;
                    }

                    std::string transitionToRemove = "";
                    for (auto& [eventTag, transition] : cfg.transitions)
                    {
                        ImGui::PushID(eventTag.c_str());
                        ImGui::TextColored(ImVec4(1, 0.8f, 0, 1), "[ %s ]", eventTag.c_str());

                        char targetBuf[128];
                        strcpy_s(targetBuf, transition.targetState.c_str());
                        if (ImGui::InputText("Target State", targetBuf, sizeof(targetBuf)))
                        {
                            transition.targetState = targetBuf;
                            configChanged = true;
                        }

                        char triggerBuf[128];
                        strcpy_s(triggerBuf, transition.triggerEvent.c_str());
                        if (ImGui::InputText("Trigger Event", triggerBuf, sizeof(triggerBuf)))
                        {
                            transition.triggerEvent = triggerBuf;
                            configChanged = true;
                        }

                        if (ImGui::DragFloat("Blend Duration", &transition.blendDuration, 0.01f, 0.0f, 2.0f, "%.2f s"))
                        {
                            configChanged = true;
                        }

                        if (ImGui::Checkbox("Has Exit Time", &transition.hasExitTime))
                        {
                            configChanged = true;
                        }

                        if (!transition.hasExitTime)
                        {
                            if (ImGui::Checkbox("Instant Cancel (Skip Animation on Press)", &transition.canInterrupt))
                            {
                                configChanged = true;
                            }

                            if (transition.canInterrupt)
                            {
                                ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.4f, 1.0f), "  -> Mode: Instant Skip on Input");
                            }
                            else
                            {
                                if (ImGui::DragFloatRange2("Combo Window", &transition.windowStart, &transition.windowEnd, 0.05f, 0.0f, 10.0f, "Open: %.2f", "Close: %.2f"))
                                {
                                    configChanged = true;
                                }
                            }
                        }

                        if (ImGui::Button("Delete Transition"))
                        {
                            transitionToRemove = eventTag;
                        }

                        ImGui::Separator();
                        ImGui::PopID();
                    }

                    if (!transitionToRemove.empty())
                    {
                        cfg.transitions.erase(transitionToRemove);
                        configChanged = true;
                    }

                    static char newEventTagBuf[64] = "OnAttack";
                    static char newTargetStateBuf[64] = "Slash1";
                    ImGui::InputText("New Event Key", newEventTagBuf, sizeof(newEventTagBuf));
                    ImGui::InputText("New Target State", newTargetStateBuf, sizeof(newTargetStateBuf));
                    if (ImGui::Button("+ Add Transition Link"))
                    {
                        if (strlen(newEventTagBuf) > 0 && strlen(newTargetStateBuf) > 0)
                        {
                            TransitionData tData;
                            tData.targetState = newTargetStateBuf;
                            tData.triggerEvent = newEventTagBuf;
                            tData.blendDuration = 0.2f;
                            cfg.transitions[newEventTagBuf] = tData;
                            configChanged = true;
                        }
                    }

                    ImGui::TreePop();
                }

                bool isComboState = (cfg.stateType == "OneHand" || !cfg.comboAnimationNames.empty() || !cfg.comboEndTimes.empty());
                if (isComboState && ImGui::TreeNode("Combo Settings (Multi-Stage)"))
                {
                    size_t stageCount = cfg.comboAnimationNames.size();
                    if (stageCount == 0 && !cfg.comboEndTimes.empty())
                    {
                        stageCount = cfg.comboEndTimes.size();
                    }

                    int toRemoveIndex = -1;
                    for (size_t i = 0; i < stageCount; ++i)
                    {
                        ImGui::PushID(static_cast<int>(i));
                        ImGui::Text("Stage %zu:", i + 1);

                        // Animation name
                        if (i < cfg.comboAnimationNames.size())
                        {
                            char animBuf[64];
                            strcpy_s(animBuf, cfg.comboAnimationNames[i].c_str());
                            if (ImGui::InputText("Anim Name", animBuf, sizeof(animBuf)))
                            {
                                cfg.comboAnimationNames[i] = animBuf;
                                configChanged = true;
                            }
                        }

                        // Window Start
                        if (i >= cfg.comboWindowStarts.size())
                        {
                            cfg.comboWindowStarts.resize(i + 1, 1.0f);
                        }
                        if (ImGui::DragFloat("Window Open", &cfg.comboWindowStarts[i], 0.05f, 0.0f, 10.0f))
                            configChanged = true;

                        // Stage End
                        if (i >= cfg.comboEndTimes.size())
                        {
                            cfg.comboEndTimes.resize(i + 1, 1.5f);
                        }
                        if (ImGui::DragFloat("Stage End", &cfg.comboEndTimes[i], 0.05f, 0.0f, 10.0f))
                            configChanged = true;

                        // Blend duration
                        if (i >= cfg.comboBlendDurations.size())
                        {
                            cfg.comboBlendDurations.resize(i + 1, 0.1f);
                        }
                        if (ImGui::DragFloat("Blend In", &cfg.comboBlendDurations[i], 0.02f, 0.0f, 2.0f))
                            configChanged = true;

                        if (ImGui::Button("Remove Stage"))
                        {
                            toRemoveIndex = static_cast<int>(i);
                        }

                        ImGui::Separator();
                        ImGui::PopID();
                    }

                    if (toRemoveIndex >= 0)
                    {
                        if (toRemoveIndex < static_cast<int>(cfg.comboAnimationNames.size()))
                            cfg.comboAnimationNames.erase(cfg.comboAnimationNames.begin() + toRemoveIndex);
                        if (toRemoveIndex < static_cast<int>(cfg.comboWindowStarts.size()))
                            cfg.comboWindowStarts.erase(cfg.comboWindowStarts.begin() + toRemoveIndex);
                        if (toRemoveIndex < static_cast<int>(cfg.comboEndTimes.size()))
                            cfg.comboEndTimes.erase(cfg.comboEndTimes.begin() + toRemoveIndex);
                        if (toRemoveIndex < static_cast<int>(cfg.comboBlendDurations.size()))
                            cfg.comboBlendDurations.erase(cfg.comboBlendDurations.begin() + toRemoveIndex);
                        configChanged = true;
                    }

                    if (ImGui::Button("+ Add Combo Stage"))
                    {
                        std::string newAnim = "Slash" + std::to_string(cfg.comboAnimationNames.size() + 1);
                        cfg.comboAnimationNames.push_back(newAnim);
                        cfg.comboWindowStarts.push_back(1.0f);
                        cfg.comboEndTimes.push_back(1.5f);
                        cfg.comboBlendDurations.push_back(0.1f);
                        configChanged = true;
                    }

                    if (ImGui::DragFloat("Combo Exit Blend", &cfg.comboExitBlendDuration, 0.02f, 0.0f, 2.0f))
                        configChanged = true;

                    ImGui::TreePop();
                }

                if (ImGui::Button("Delete This State"))
                {
                    stateToRemove = name;
                }

                ImGui::TreePop();
            }
            ImGui::PopID();
        }

        if (!stateToRemove.empty())
        {
            m_stateConfigs.erase(stateToRemove);
            configChanged = true;
        }

        static char newStateNameBuf[64] = "Slash4";
        ImGui::Separator();
        ImGui::InputText("New State Name", newStateNameBuf, sizeof(newStateNameBuf));
        if (ImGui::Button("+ Add New Combat State"))
        {
            if (strlen(newStateNameBuf) > 0 && !m_stateConfigs.contains(newStateNameBuf))
            {
                StateConfig newCfg;
                newCfg.stateName = newStateNameBuf;
                newCfg.stateType = "DataDriven";
                newCfg.animationName = newStateNameBuf;
                newCfg.isAttack = true;
                newCfg.stateDuration = 1.0f;
                TransitionData exitT;
                exitT.targetState = "Idle";
                exitT.hasExitTime = true;
                exitT.blendDuration = 0.2f;
                newCfg.transitions["Exit"] = exitT;

                m_stateConfigs[newStateNameBuf] = newCfg;
                configChanged = true;
            }
        }

        if (configChanged)
        {
            RebuildAllStates();
        }
    }
}