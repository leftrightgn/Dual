#include "pch.h"
#include "CombatStateMachineComponent.h"
#include "BlackBoard/BossBlackBoard.h"
#include "BlackBoard/CombatBlackBoard.h"
#include "BlackBoard/PlayerBlackBoard.h"
#include "States/CombatStates.h"
#include "States/ICombatState.h"
#include "../../External/Engine/Entities/Actor.h"
#include "../../External/Engine/Components/ColliderComponent/ColliderComponent.h"
#include "../../External/Engine/Components/DamageDealerComponent.h"
#include "../../External/Engine/Components/HealthComponent.h"
#include "../../External/Engine/Components/SoundComponent.h"
#include "../../External/Engine/Components/TargetTrackingComponent.h"
#include "../../External/Engine/Message/Messenger.h"
#if HEIN_WITH_EDITOR
#include "ImGui/imgui.h"
#include "ImGui/imNodes.h"
#endif
#include <filesystem>
#include <fstream>

namespace HEIN
{
    static std::string CombatStateTypeToString(CombatStateType type)
    {
        switch (type)
        {
        case CombatStateType::Attack: return "Attack";
        case CombatStateType::ComboFinisher: return "ComboFinisher";
        case CombatStateType::Block: return "Block";
        case CombatStateType::Dodge: return "Dodge";
        case CombatStateType::Stun: return "Stun";
        case CombatStateType::Jump: return "Jump";
        case CombatStateType::GuardCounter: return "GuardCounter";
        case CombatStateType::Death: return "Death";
        default: return "Normal";
        }
    }

    static CombatStateType StringToCombatStateType(const std::string& str)
    {
        if (str == "Attack") return CombatStateType::Attack;
        if (str == "ComboFinisher") return CombatStateType::ComboFinisher;
        if (str == "Block") return CombatStateType::Block;
        if (str == "Dodge") return CombatStateType::Dodge;
        if (str == "Stun") return CombatStateType::Stun;
        if (str == "Jump") return CombatStateType::Jump;
        if (str == "GuardCounter") return CombatStateType::GuardCounter;
        if (str == "Death") return CombatStateType::Death;
        return CombatStateType::Normal;
    }

    static std::string StateEffectTypeToString(StateEffectType type)
    {
        switch (type)
        {
        case StateEffectType::Shockwave: return "Shockwave";
        case StateEffectType::CameraShake: return "CameraShake";
        case StateEffectType::SoundCue: return "SoundCue";
        case StateEffectType::VFX: return "VFX";
        default: return "Shockwave";
        }
    }

    static StateEffectType StringToStateEffectType(const std::string& str)
    {
        if (str == "CameraShake") return StateEffectType::CameraShake;
        if (str == "SoundCue") return StateEffectType::SoundCue;
        if (str == "VFX") return StateEffectType::VFX;
        return StateEffectType::Shockwave;
    }

CombatStateMachineComponent::CombatStateMachineComponent(Actor* owner) : IComponent(owner) {}

HEIN::CombatStateMachineComponent::~CombatStateMachineComponent()
{
    if (Messenger* msg = Messenger::GetInstance())
    {
        msg->UnRegisterObserver(this);
    }
}

void HEIN::CombatStateMachineComponent::Start()
{
    if (Messenger* msg = Messenger::GetInstance())
    {
        msg->Register(m_owner->GetID(), this);
    }
    EnsureDefaultCombatSoundCues();

    // Ensure the default state animation is applied now that SkinnedModelComponent has loaded its animations
    std::string startState = m_defaultStateName;
    if (startState.empty() || !m_states.contains(startState))
    {
        if (m_states.contains("Idle")) startState = "Idle";
        else if (!m_states.empty()) startState = m_states.begin()->first;
    }

    if (!startState.empty() && m_states.contains(startState))
    {
        m_currentState = nullptr;
        ChangeState(startState, 0.0f);
        ApplyPendingState();
    }
}

void HEIN::CombatStateMachineComponent::Update(float deltaTime)
{
    float rawDt = (m_owner != nullptr && m_owner->GetUnscaledDeltaTime() > 0.0f)
        ? m_owner->GetUnscaledDeltaTime()
        : deltaTime;

    // Automatic death transition for any actor when health component reaches zero
    auto* health = m_owner ? m_owner->GetComponent<HealthComponent>() : nullptr;
    if (health && health->isDead() && m_currentStateName != "Death" && m_states.contains("Death"))
    {
        m_messageBuffer.clear();
        ChangeState("Death", 0.15f);
        ApplyPendingState();
        return;
    }

    ApplyPendingState();
    // Tick input buffer on unscaled real time so inputs pressed during Hitstop or Wind-Up Pause are never dropped
    ProcessBuffer(rawDt);

    if (m_pendingState != nullptr)
    {
        ApplyPendingState();
    }

    if (m_currentState)
    {
        m_currentState->Update(m_owner, this, deltaTime);
    }

    if (m_pendingState != nullptr)
    {
        ApplyPendingState();
    }

    Messenger::GetInstance()->Notify(m_owner->GetID(), IsWeaponHitboxActive() ? Message::SET_WEAPON_ACTIVE : Message::SET_WEAPON_INACTIVE);

    HEIN::CombatBlackBoard* blackboard = m_owner->GetComponent<CombatBlackBoard>();
    HEIN::PlayerBlackBoard* playerBB = m_owner->GetComponent<PlayerBlackBoard>();
    HEIN::BossBlackBoard* bossBB = m_owner->GetComponent<BossBlackBoard>();

    if (bossBB)
    {
        if (bossBB->parryDecayTimer > 0.0f)
        {
            bossBB->parryDecayTimer -= deltaTime;
            if (bossBB->parryDecayTimer <= 0.0f)
            {
                bossBB->parryDecayTimer = 0.0f;
                bossBB->currentParryCount = 0;
            }
        }

        // Boss never consumes or depletes stamina (Elden Ring boss design)
        if (blackboard)
        {
            blackboard->currentStamina = 100.0f;
            blackboard->maxStamina = 100.0f;
        }
    }

    if (blackboard)
    {
        if (playerBB)
        {
            if (IsBlocking())
            {
                playerBB->blockActiveTimer += deltaTime;

                if (playerBB->blockCooldownTimer <= 0.0f && playerBB->currentBlockStamina < playerBB->maxBlockStamina)
                {
                    playerBB->currentBlockStamina += playerBB->blockHeldRecoveryRate * deltaTime;
                    if (playerBB->currentBlockStamina >= playerBB->maxBlockStamina)
                    {
                        playerBB->currentBlockStamina = playerBB->maxBlockStamina;
                    }
                }
            }
            else
            {
                playerBB->blockActiveTimer = 0.0f;

                // Do NOT regenerate stamina while actively Dodging, Attacking, or Staggered
                bool isStaminaRegenLocked = (blackboard->currentStance == CombatStance::Dodging) ||
                                            (blackboard->currentStance == CombatStance::Jumping) ||
                                            blackboard->isJumping ||
                                            IsAttacking() ||
                                            (blackboard->currentStance == CombatStance::Staggered);

                if (!isStaminaRegenLocked)
                {
                    if (playerBB->staminaRegenDelayTimer > 0.0f)
                    {
                        playerBB->staminaRegenDelayTimer -= deltaTime;
                        if (playerBB->staminaRegenDelayTimer < 0.0f) playerBB->staminaRegenDelayTimer = 0.0f;
                    }

                    if (playerBB->staminaRegenDelayTimer <= 0.0f && playerBB->currentBlockStamina < playerBB->maxBlockStamina)
                    {
                        playerBB->currentBlockStamina += playerBB->blockRecoveryRate * deltaTime;

                        if (playerBB->currentBlockStamina >= playerBB->maxBlockStamina)
                        {
                            playerBB->currentBlockStamina = playerBB->maxBlockStamina;
                        }

                        if (playerBB->currentBlockStamina >= 30.0f)
                        {
                            playerBB->isBlockBroken = false;
                        }
                    }
                }
            }

            if (playerBB->blockCooldownTimer > 0.0f)
            {
                playerBB->blockCooldownTimer -= deltaTime;
                if (playerBB->blockCooldownTimer < 0.0f) playerBB->blockCooldownTimer = 0.0f;
            }

            if (playerBB->parryLockoutTimer > 0.0f)
            {
                playerBB->parryLockoutTimer -= deltaTime;
                if (playerBB->parryLockoutTimer < 0.0f) playerBB->parryLockoutTimer = 0.0f;
            }

            if (playerBB->guardCounterWindowTimer > 0.0f)
            {
                playerBB->guardCounterWindowTimer -= deltaTime;
                if (playerBB->guardCounterWindowTimer < 0.0f) playerBB->guardCounterWindowTimer = 0.0f;
            }

            // Sync unified stamina with base CombatBlackBoard
            blackboard->currentStamina = std::max(0.0f, playerBB->currentBlockStamina);
            blackboard->maxStamina = playerBB->maxBlockStamina;
        }
        else if (blackboard->useStamina)
        {
            bool isStaminaRegenLocked = (blackboard->currentStance == CombatStance::Dodging) ||
                                        (blackboard->currentStance == CombatStance::Jumping) ||
                                        blackboard->isJumping ||
                                        IsAttacking() ||
                                        (blackboard->currentStance == CombatStance::Staggered);

            if (!isStaminaRegenLocked)
            {
                if (blackboard->baseStaminaRegenTimer > 0.0f)
                {
                    blackboard->baseStaminaRegenTimer -= deltaTime;
                    if (blackboard->baseStaminaRegenTimer < 0.0f) blackboard->baseStaminaRegenTimer = 0.0f;
                }

                if (blackboard->baseStaminaRegenTimer <= 0.0f && blackboard->currentStamina < blackboard->maxStamina)
                {
                    blackboard->currentStamina = std::min(
                        blackboard->maxStamina,
                        blackboard->currentStamina + blackboard->baseStaminaRecoveryRate * deltaTime
                    );
                }
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
            if (tracking->IsLockedOn())
            {
                blackboard->lockedTargetID = tracking->GetTargetID();
                blackboard->dirToTarget = tracking->GetDirToTarget();
                blackboard->distanceToTarget = tracking->GetDistanceToTarget();
            }
            blackboard->isLockedOn = tracking->IsLockedOn();
        }
    }

    if (m_transitionPulseTimer > 0.0f)
    {
        m_transitionPulseTimer -= rawDt;
        if (m_transitionPulseTimer < 0.0f)
        {
            m_transitionPulseTimer = 0.0f;
        }
    }
}

void HEIN::CombatStateMachineComponent::LateUpdate(float /*deltaTime*/)
{
#if HEIN_WITH_EDITOR
    if (m_showNodeEditor)
    {
        RenderNodeGraph();
    }
    if (m_showStateDetailsEditor)
    {
        RenderStateDetailsWindow();
    }
#endif
}

bool HEIN::CombatStateMachineComponent::ChangeState(const std::string& stateName, float blendDuration)
{
    auto it = m_states.find(stateName);
    if (it != m_states.end())
    {
        m_pendingState = it->second.get();
        m_pendingBlendDuration = blendDuration;
        return true;
    }
    return false;
}

void HEIN::CombatStateMachineComponent::ApplyPendingState()
{
    if (m_pendingState == nullptr) return;

    std::string previousStateName = m_currentStateName;

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

    if (!previousStateName.empty() && previousStateName != m_currentStateName)
    {
        m_lastTransitionFrom = previousStateName;
        m_lastTransitionTo = m_currentStateName;
        m_transitionPulseTimer = 0.65f;
    }
}

void HEIN::CombatStateMachineComponent::AddState(const StateConfig& config)
{
    m_stateConfigs[config.stateName] = config;
    m_states[config.stateName] = std::make_unique<UniversalCombatState>(&m_stateConfigs[config.stateName]);

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
        m_states[name] = std::make_unique<UniversalCombatState>(&config);
    }

    if (!m_defaultStateName.empty() && m_states.contains(m_defaultStateName))
    {
        ChangeState(m_defaultStateName, 0.0f);
        ApplyPendingState();
    }
}

void HEIN::CombatStateMachineComponent::OnMessageAccepted(Message::MessageID messageID)
{
    // Discard any incoming commands or input buffers while in Death state
    if (m_currentStateName == "Death")
    {
        return;
    }

    if (messageID == Message::ATTACK_HIT_CONFIRMED)
    {
        HEIN::CombatBlackBoard* blackboard = m_owner->GetComponent<CombatBlackBoard>();
        if (blackboard)
        {
            blackboard->lastAttackHit = true;
        }
        if (m_currentState != nullptr)
        {
            m_currentState->HandleMessage(m_owner, this, messageID);
        }
        return;
    }

    if (messageID == Message::HIT_STUN)
    {
        auto* health = m_owner->GetComponent<HealthComponent>();
        if (health && health->isDead()) return;

        // Try transitioning via current state's transitions first if defined
        if (m_currentState != nullptr && m_currentState->HandleMessage(m_owner, this, messageID))
        {
            m_messageBuffer.clear();
            return;
        }

        // Default fallback: force transition to Stun state if registered
        auto itStun = m_states.find("Stun");
        if (itStun != m_states.end() && m_currentStateName != "Stun")
        {
            ChangeState("Stun", 0.02f);
            ApplyPendingState();
            m_messageBuffer.clear();
        }
        return;
    }

    if (messageID == Message::PLAYER_ACTION_ATTACK ||
        messageID == Message::ENEMY_ACTION_HEAVY_ATTACK ||
        messageID == Message::PLAYER_ACTION_BLOCK ||
        messageID == Message::PLAYER_STOP_BLOCK ||
        messageID == Message::PLAYER_ACTION_DODGE ||
        messageID == Message::PLAYER_ACTION_JUMP)
    {
        if (messageID == Message::PLAYER_ACTION_ATTACK)
        {
            HEIN::PlayerBlackBoard* playerBB = m_owner->GetComponent<PlayerBlackBoard>();
            if (playerBB && (playerBB->currentBlockStamina <= 0.0f || playerBB->isBlockBroken)) return;
        }
        if (messageID == Message::PLAYER_ACTION_DODGE)
        {
            HEIN::CombatBlackBoard* blackboard = m_owner->GetComponent<CombatBlackBoard>();
            HEIN::PlayerBlackBoard* playerBB = m_owner->GetComponent<PlayerBlackBoard>();
            if (blackboard && blackboard->dodgeCooldownTimer > 0.0f) return;
            if (playerBB && (playerBB->currentBlockStamina <= 0.0f || playerBB->isBlockBroken)) return;
        }
        if (messageID == Message::PLAYER_ACTION_BLOCK)
        {
            HEIN::PlayerBlackBoard* playerBB = m_owner->GetComponent<PlayerBlackBoard>();
            if (playerBB && (playerBB->isBlockBroken || playerBB->currentBlockStamina <= 0.0f)) return;
        }
        if (messageID == Message::PLAYER_ACTION_JUMP)
        {
            HEIN::CombatBlackBoard* blackboard = m_owner->GetComponent<CombatBlackBoard>();
            HEIN::PlayerBlackBoard* playerBB = m_owner->GetComponent<PlayerBlackBoard>();
            if (blackboard && (blackboard->isJumping || blackboard->currentStance == CombatStance::Jumping)) return;
            if (playerBB && (playerBB->currentBlockStamina <= 0.0f || playerBB->isBlockBroken)) return;
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
    if (currentMessage == Message::PLAYER_ACTION_ATTACK ||
        currentMessage == Message::PLAYER_ACTION_DODGE ||
        currentMessage == Message::PLAYER_ACTION_JUMP)
    {
        auto* blackboard = m_owner->GetComponent<CombatBlackBoard>();
        auto* playerBB = m_owner->GetComponent<PlayerBlackBoard>();
        if (currentMessage == Message::PLAYER_ACTION_DODGE && blackboard && blackboard->dodgeCooldownTimer > 0.0f)
        {
            m_messageBuffer.clear();
            return;
        }
        if (currentMessage == Message::PLAYER_ACTION_JUMP && blackboard &&
            (blackboard->isJumping || blackboard->currentStance == CombatStance::Jumping))
        {
            m_messageBuffer.clear();
            return;
        }
        if (playerBB && (playerBB->currentBlockStamina <= 0.0f || playerBB->isBlockBroken))
        {
            m_messageBuffer.clear();
            return;
        }
    }

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

bool HEIN::CombatStateMachineComponent::IsWeaponHitboxActive() const
{
    if (m_currentState != nullptr) return m_currentState->IsWeaponHitboxActive();
    return false;
}

int HEIN::CombatStateMachineComponent::GetCurrentHitstopFrames() const
{
    if (m_currentState != nullptr) return m_currentState->GetHitstopFrames();
    return 5;
}

bool HEIN::CombatStateMachineComponent::IsBlocking() const
{
    if (m_currentState != nullptr) return m_currentState->IsBlockState();
    return false;
}

bool HEIN::CombatStateMachineComponent::IsGuardCounter() const
{
    if (m_currentState != nullptr) return m_currentState->IsGuardCounterState();
    return false;
}

void CombatStateMachineComponent::EnsureDefaultCombatSoundCues()
{
    bool modified = false;
    for (auto& [name, cfg] : m_stateConfigs)
    {
        bool hasSoundCue = false;
        for (auto& fx : cfg.effects)
        {
            if (fx.type == StateEffectType::SoundCue)
            {
                hasSoundCue = true;
                // - Normalize legacy WAV stem keys to match SoundComponent preset cue names
                if (fx.assetKey == "Slash_Swing1")
                {
                    fx.assetKey = "Slash1";
                    modified = true;
                }
                else if (fx.assetKey == "Slash_Swing2")
                {
                    fx.assetKey = "Slash2";
                    modified = true;
                }
                else if (fx.assetKey == "Slash_Heavy")
                {
                    fx.assetKey = (name == "HeavyAttack" || name == "HighSpinAttack") ? "HeavySlam" : "Slash3";
                    modified = true;
                }
                else if (fx.assetKey == "Dodge_Roll")
                {
                    fx.assetKey = "Dodge";
                    modified = true;
                }
                break;
            }
        }
        if (hasSoundCue)
        {
            continue;
        }

        if (name == "HeavyAttack" || name == "HighSpinAttack")
        {
            StateEffectConfig sfx;
            sfx.type = StateEffectType::SoundCue;
            sfx.assetKey = "HeavySlam";
            sfx.triggerTime = 0.12f;
            cfg.effects.push_back(sfx);
            modified = true;
        }
        else if (cfg.stateTypeEnum == CombatStateType::ComboFinisher || cfg.isGuardCounter || name == "Slash3" || name == "Combo3")
        {
            StateEffectConfig sfx;
            sfx.type = StateEffectType::SoundCue;
            sfx.assetKey = "Slash3";
            sfx.triggerTime = 0.10f;
            cfg.effects.push_back(sfx);
            modified = true;
        }
        else if (cfg.isAttack || cfg.IsAttackState())
        {
            StateEffectConfig sfx;
            sfx.type = StateEffectType::SoundCue;
            sfx.assetKey = (name == "Slash2" || name == "Combo2") ? "Slash2" : "Slash1";
            sfx.triggerTime = 0.08f;
            cfg.effects.push_back(sfx);
            modified = true;
        }
        else if (cfg.IsDodgeState() || name == "Dodge" || name == "Roll")
        {
            StateEffectConfig sfx;
            sfx.type = StateEffectType::SoundCue;
            sfx.assetKey = "Dodge";
            sfx.triggerTime = 0.04f;
            cfg.effects.push_back(sfx);
            modified = true;
        }
    }

    if (modified)
    {
        RebuildAllStates();
    }
}

void HEIN::CombatStateMachineComponent::OnTriggerOverLap(const HEIN::TriggerEventPayLoad& payLoad)
{
    // Keeping original hit detection payload stub
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

bool HEIN::CombatStateMachineComponent::LoadFromConfigFile(const std::string& path)
{
    std::vector<std::string> candidates = {
        path,
        "Dual/" + path,
        "../" + path,
        "../Dual/" + path
    };

    std::ifstream file;
    for (const auto& c : candidates)
    {
        file.open(c);
        if (file.is_open())
        {
            break;
        }
    }

    if (!file.is_open())
    {
        OutputDebugStringA(("Failed to open Combat Config file: " + path + "\n").c_str());
        return false;
    }

    try
    {
        nlohmann::json j;
        file >> j;
        if (j.is_object())
        {
            m_configPath = path;
            DeserializeStatesFromJSON(j);
            return true;
        }
    }
    catch (const std::exception& e)
    {
        OutputDebugStringA(("JSON parse error in Combat Config file: " + std::string(e.what()) + "\n").c_str());
    }

    return false;
}

bool HEIN::CombatStateMachineComponent::SaveToConfigFile(const std::string& path) const
{
    std::string targetPath = path.empty() ? m_configPath : path;
    if (targetPath.empty()) return false;

    std::error_code ec;
    std::string writePath = targetPath;
    if (!std::filesystem::exists("Resources", ec) && std::filesystem::exists("Dual/Resources", ec))
    {
        if (writePath.find("Dual/") != 0)
        {
            writePath = "Dual/" + writePath;
        }
    }
    std::filesystem::create_directories(std::filesystem::path(writePath).parent_path(), ec);

    std::ofstream file(writePath);
    if (!file.is_open()) return false;

    nlohmann::json data;
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
        stateJson["isGuardCounter"] = cfg.isGuardCounter;
        stateJson["lockMovementDirection"] = cfg.lockMovementDirection;
        stateJson["invincibilityStart"] = cfg.invincibilityStart;
        stateJson["invincibilityEnd"] = cfg.invincibilityEnd;
        stateJson["windupPauseStart"] = cfg.windupPauseStart;
        stateJson["windupPauseDuration"] = cfg.windupPauseDuration;
        stateJson["windupTimeScale"] = cfg.windupTimeScale;
        stateJson["strikeSpeedMultiplier"] = cfg.strikeSpeedMultiplier;
        stateJson["hitstopFrames"] = cfg.hitstopFrames;
        stateJson["hitboxStart"] = cfg.hitboxStart;
        stateJson["hitboxEnd"] = cfg.hitboxEnd;
        stateJson["combatStateType"] = CombatStateTypeToString(cfg.stateTypeEnum);
        stateJson["staminaCost"] = cfg.staminaCost;
        stateJson["staminaRegenDelay"] = cfg.staminaRegenDelay;
        stateJson["staminaDamage"] = cfg.staminaDamage;
        stateJson["comboPermutationStep"] = cfg.comboPermutationStep;
        stateJson["enterImpulse"] = { cfg.enterImpulse.x, cfg.enterImpulse.y, cfg.enterImpulse.z };
        stateJson["colliderModifiers"] = nlohmann::json::array();
        for (const auto& mod : cfg.colliderModifiers)
        {
            nlohmann::json modJson;
            modJson["colliderTag"] = mod.colliderTag;
            modJson["startTime"] = mod.startTime;
            modJson["endTime"] = mod.endTime;
            modJson["enableCollider"] = mod.enableCollider;
            modJson["followRootMotionY"] = mod.followRootMotionY;
            modJson["heightScale"] = mod.heightScale;
            modJson["radiusScale"] = mod.radiusScale;
            modJson["offsetDelta"] = { mod.offsetDelta.x, mod.offsetDelta.y, mod.offsetDelta.z };
            stateJson["colliderModifiers"].push_back(modJson);
        }

        stateJson["effects"] = nlohmann::json::array();
        for (const auto& eff : cfg.effects)
        {
            nlohmann::json effJson;
            effJson["type"] = StateEffectTypeToString(eff.type);
            effJson["triggerTime"] = eff.triggerTime;
            if (eff.type == StateEffectType::Shockwave)
            {
                effJson["radius"] = eff.radius;
                effJson["duration"] = eff.duration;
                effJson["damage"] = eff.damage;
                effJson["knockbackForce"] = eff.knockbackForce;
                effJson["forwardOffset"] = eff.forwardOffset;
                effJson["isGroundOnly"] = eff.isGroundOnly;
            }
            else if (eff.type == StateEffectType::CameraShake)
            {
                effJson["shakeIntensity"] = eff.shakeIntensity;
                effJson["shakeDuration"] = eff.shakeDuration;
            }
            else
            {
                effJson["assetKey"] = eff.assetKey;
            }
            stateJson["effects"].push_back(effJson);
        }

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
                {"canInterrupt", transition.canInterrupt},
                {"requireHit", transition.requireHit}
            };
        }

        stateJson["comboAnimationNames"] = cfg.comboAnimationNames;
        stateJson["comboEndTimes"] = cfg.comboEndTimes;
        stateJson["comboWindowStarts"] = cfg.comboWindowStarts;
        stateJson["comboBlendDurations"] = cfg.comboBlendDurations;
        stateJson["comboExitBlendDuration"] = cfg.comboExitBlendDuration;

        data["states"].push_back(stateJson);
    }

    file << data.dump(4);
    return true;
}

nlohmann::json HEIN::CombatStateMachineComponent::Serialize()
{
    nlohmann::json data = IComponent::Serialize();

    data["defaultState"] = m_defaultStateName;

    // MODULAR COMBAT ASSET ARCHITECTURE:
    // If a standalone combat asset (e.g. "Resources/Data/Combat/Knight_Combat.json") is bound,
    // only save the file reference path! This decouples scene layout from character combat tuning
    // and prevents auto-save from ever polluting combat configs with mid-fight runtime states.
    if (!m_configPath.empty())
    {
        data["ConfigPath"] = m_configPath;
        return data;
    }

    // Monolithic fallback (embeds states array if no standalone asset is used)
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
        stateJson["isGuardCounter"] = cfg.isGuardCounter;
        stateJson["lockMovementDirection"] = cfg.lockMovementDirection;
        stateJson["invincibilityStart"] = cfg.invincibilityStart;
        stateJson["invincibilityEnd"] = cfg.invincibilityEnd;
        stateJson["windupPauseStart"] = cfg.windupPauseStart;
        stateJson["windupPauseDuration"] = cfg.windupPauseDuration;
        stateJson["windupTimeScale"] = cfg.windupTimeScale;
        stateJson["strikeSpeedMultiplier"] = cfg.strikeSpeedMultiplier;
        stateJson["hitstopFrames"] = cfg.hitstopFrames;
        stateJson["hitboxStart"] = cfg.hitboxStart;
        stateJson["hitboxEnd"] = cfg.hitboxEnd;
        stateJson["combatStateType"] = CombatStateTypeToString(cfg.stateTypeEnum);
        stateJson["staminaCost"] = cfg.staminaCost;
        stateJson["staminaRegenDelay"] = cfg.staminaRegenDelay;
        stateJson["staminaDamage"] = cfg.staminaDamage;
        stateJson["comboPermutationStep"] = cfg.comboPermutationStep;
        stateJson["enterImpulse"] = { cfg.enterImpulse.x, cfg.enterImpulse.y, cfg.enterImpulse.z };

        stateJson["effects"] = nlohmann::json::array();
        for (const auto& eff : cfg.effects)
        {
            nlohmann::json effJson;
            effJson["type"] = StateEffectTypeToString(eff.type);
            effJson["triggerTime"] = eff.triggerTime;
            if (eff.type == StateEffectType::Shockwave)
            {
                effJson["radius"] = eff.radius;
                effJson["duration"] = eff.duration;
                effJson["damage"] = eff.damage;
                effJson["knockbackForce"] = eff.knockbackForce;
                effJson["forwardOffset"] = eff.forwardOffset;
                effJson["isGroundOnly"] = eff.isGroundOnly;
            }
            else if (eff.type == StateEffectType::CameraShake)
            {
                effJson["shakeIntensity"] = eff.shakeIntensity;
                effJson["shakeDuration"] = eff.shakeDuration;
            }
            else
            {
                effJson["assetKey"] = eff.assetKey;
            }
            stateJson["effects"].push_back(effJson);
        }

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
                {"canInterrupt", transition.canInterrupt},
                {"requireHit", transition.requireHit}
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

    // Modular asset loading: check for standalone ConfigPath
    if (data.contains("ConfigPath") && data["ConfigPath"].is_string())
    {
        std::string cfgPath = data["ConfigPath"].get<std::string>();
        if (!cfgPath.empty() && LoadFromConfigFile(cfgPath))
        {
            if (data.contains("defaultState") && data["defaultState"].is_string())
            {
                m_defaultStateName = data["defaultState"].get<std::string>();
            }
            return;
        }
    }

    // Legacy / Monolithic fallback: parse inline states from scene JSON
    DeserializeStatesFromJSON(data);
}

void HEIN::CombatStateMachineComponent::DeserializeStatesFromJSON(const nlohmann::json& data)
{
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
            if (cfg.stateName == "OneHand" || cfg.stateType == "OneHand")
            {
                cfg.isAttack = true;
            }
            if (stateJson.contains("isBlock")) cfg.isBlock = stateJson["isBlock"].get<bool>();
            if (cfg.stateName == "Block" || cfg.stateType == "Block")
            {
                cfg.isBlock = true;
                cfg.isLooping = true;
                cfg.moveSpeed = 0.0f;
            }
            if (stateJson.contains("isGuardCounter")) cfg.isGuardCounter = stateJson["isGuardCounter"].get<bool>();
            if (stateJson.contains("isDeath")) cfg.isDeath = stateJson["isDeath"].get<bool>();
            if (cfg.stateName == "GuardCounter" || cfg.stateType == "GuardCounter")
            {
                cfg.isGuardCounter = true;
                cfg.isAttack = true;
            }
            if (stateJson.contains("lockMovementDirection")) cfg.lockMovementDirection = stateJson["lockMovementDirection"].get<bool>();
            if (stateJson.contains("invincibilityStart")) cfg.invincibilityStart = stateJson["invincibilityStart"].get<float>();
            if (stateJson.contains("invincibilityEnd")) cfg.invincibilityEnd = stateJson["invincibilityEnd"].get<float>();
            if (cfg.stateName == "Dodge" || cfg.stateType == "Dodge")
            {
                cfg.lockMovementDirection = true;
                if (!stateJson.contains("invincibilityEnd") || cfg.invincibilityEnd <= 0.0f)
                {
                    cfg.invincibilityStart = 0.0f;
                    cfg.invincibilityEnd = (cfg.stateDuration > 0.0f) ? (cfg.stateDuration * 0.7f) : 0.6f;
                }
            }

            // Animation Anticipation Wind-Up & Hitstop Config
            if (stateJson.contains("windupPauseStart")) cfg.windupPauseStart = stateJson["windupPauseStart"].get<float>();
            if (stateJson.contains("windupPauseDuration")) cfg.windupPauseDuration = stateJson["windupPauseDuration"].get<float>();
            if (stateJson.contains("windupTimeScale")) cfg.windupTimeScale = stateJson["windupTimeScale"].get<float>();
            if (stateJson.contains("strikeSpeedMultiplier")) cfg.strikeSpeedMultiplier = stateJson["strikeSpeedMultiplier"].get<float>();
            if (stateJson.contains("hitstopFrames")) cfg.hitstopFrames = stateJson["hitstopFrames"].get<int>();
            if (stateJson.contains("hitboxStart")) cfg.hitboxStart = stateJson["hitboxStart"].get<float>();
            if (stateJson.contains("hitboxEnd")) cfg.hitboxEnd = stateJson["hitboxEnd"].get<float>();
            if (stateJson.contains("combatStateType"))
            {
                cfg.stateTypeEnum = StringToCombatStateType(stateJson["combatStateType"].get<std::string>());
            }
            else if (cfg.stateType == "Block" || cfg.stateName == "Block")
            {
                cfg.stateTypeEnum = CombatStateType::Block;
            }
            else if (cfg.stateType == "Dodge" || cfg.stateName == "Dodge")
            {
                cfg.stateTypeEnum = CombatStateType::Dodge;
            }
            else if (cfg.stateType == "Stun" || cfg.stateName == "Stun")
            {
                cfg.stateTypeEnum = CombatStateType::Stun;
            }
            else if (cfg.stateName.find("Jump") != std::string::npos)
            {
                cfg.stateTypeEnum = CombatStateType::Jump;
            }
            else if (cfg.stateType == "Death" || cfg.stateName == "Death" || cfg.isDeath)
            {
                cfg.stateTypeEnum = CombatStateType::Death;
            }
            else if (cfg.isGuardCounter || cfg.stateName == "GuardCounter")
            {
                cfg.stateTypeEnum = CombatStateType::GuardCounter;
            }
            else if (cfg.isAttack)
            {
                cfg.stateTypeEnum = (cfg.stateName.find("3") != std::string::npos)
                    ? CombatStateType::ComboFinisher
                    : CombatStateType::Attack;
            }

            if (stateJson.contains("staminaCost")) cfg.staminaCost = stateJson["staminaCost"].get<float>();
            if (stateJson.contains("staminaRegenDelay")) cfg.staminaRegenDelay = stateJson["staminaRegenDelay"].get<float>();
            if (stateJson.contains("staminaDamage")) cfg.staminaDamage = stateJson["staminaDamage"].get<float>();
            if (stateJson.contains("comboPermutationStep")) cfg.comboPermutationStep = stateJson["comboPermutationStep"].get<int>();
            if (stateJson.contains("enterImpulse") && stateJson["enterImpulse"].is_array() && stateJson["enterImpulse"].size() == 3)
            {
                cfg.enterImpulse.x = stateJson["enterImpulse"][0].get<float>();
                cfg.enterImpulse.y = stateJson["enterImpulse"][1].get<float>();
                cfg.enterImpulse.z = stateJson["enterImpulse"][2].get<float>();
            }
            cfg.colliderModifiers.clear();
            if (stateJson.contains("colliderModifiers") && stateJson["colliderModifiers"].is_array())
            {
                for (const auto& modJson : stateJson["colliderModifiers"])
                {
                    StateColliderModifier mod;
                    mod.colliderTag = modJson.value("colliderTag", "");
                    mod.startTime = modJson.value("startTime", 0.0f);
                    mod.endTime = modJson.value("endTime", 999.0f);
                    mod.enableCollider = modJson.value("enableCollider", true);
                    mod.followRootMotionY = modJson.value("followRootMotionY", false);
                    mod.heightScale = modJson.value("heightScale", 1.0f);
                    mod.radiusScale = modJson.value("radiusScale", 1.0f);
                    if (modJson.contains("offsetDelta") && modJson["offsetDelta"].is_array() && modJson["offsetDelta"].size() == 3)
                    {
                        mod.offsetDelta.x = modJson["offsetDelta"][0].get<float>();
                        mod.offsetDelta.y = modJson["offsetDelta"][1].get<float>();
                        mod.offsetDelta.z = modJson["offsetDelta"][2].get<float>();
                    }
                    cfg.colliderModifiers.push_back(mod);
                }
            }

            // Deserialize modular state effects
            cfg.effects.clear();
            if (stateJson.contains("effects") && stateJson["effects"].is_array())
            {
                for (const auto& effJson : stateJson["effects"])
                {
                    StateEffectConfig eff;
                    std::string tStr = effJson.value("type", "Shockwave");
                    eff.type = StringToStateEffectType(tStr);
                    eff.triggerTime = effJson.value("triggerTime", 0.0f);

                    if (eff.type == StateEffectType::Shockwave)
                    {
                        eff.radius = effJson.value("radius", 24.0f);
                        eff.duration = effJson.value("duration", 1.1f);
                        eff.damage = effJson.value("damage", 45.0f);
                        eff.knockbackForce = effJson.value("knockbackForce", 35.0f);
                        eff.forwardOffset = effJson.value("forwardOffset", 18.0f);
                        eff.isGroundOnly = effJson.value("isGroundOnly", true);
                    }
                    else if (eff.type == StateEffectType::CameraShake)
                    {
                        eff.shakeIntensity = effJson.value("shakeIntensity", 0.5f);
                        eff.shakeDuration = effJson.value("shakeDuration", 0.3f);
                    }
                    else
                    {
                        eff.assetKey = effJson.value("assetKey", "");
                    }
                    cfg.effects.push_back(eff);
                }
            }
            else if (stateJson.value("emitShockwave", false))
            {
                StateEffectConfig legacyWave;
                legacyWave.type = StateEffectType::Shockwave;
                legacyWave.triggerTime = (cfg.hitboxStart > 0.0f) ? cfg.hitboxStart : 1.45f;
                legacyWave.radius = stateJson.value("shockwaveRadius", 24.0f);
                legacyWave.duration = stateJson.value("shockwaveDuration", 1.1f);
                legacyWave.damage = stateJson.value("shockwaveDamage", 45.0f);
                legacyWave.knockbackForce = stateJson.value("shockwaveKnockback", 35.0f);
                legacyWave.forwardOffset = stateJson.value("shockwaveForwardOffset", 18.0f);
                legacyWave.isGroundOnly = stateJson.value("shockwaveIsGroundOnly", true);
                cfg.effects.push_back(legacyWave);
            }

            // Provide responsive Souls-like defaults for existing attack states if not yet configured in JSON
            if (cfg.isAttack && !stateJson.contains("windupPauseDuration"))
            {
                bool isFinisherOrHeavy = (cfg.stateName.find("3") != std::string::npos ||
                                          cfg.stateName.find("Heavy") != std::string::npos ||
                                          cfg.stateName.find("Slam") != std::string::npos);
                if (isFinisherOrHeavy)
                {
                    cfg.windupPauseStart = 0.42f;
                    cfg.windupPauseDuration = 0.12f; // ~7 frames wind-up freeze at peak
                    cfg.windupTimeScale = 0.0f;
                    cfg.strikeSpeedMultiplier = 1.22f;
                    cfg.hitstopFrames = 8;           // Heavy impact freeze (8 frames @ 60fps)
                    if (!stateJson.contains("hitboxStart")) cfg.hitboxStart = 0.46f;
                    if (!stateJson.contains("hitboxEnd")) cfg.hitboxEnd = 0.82f;
                }
                else
                {
                    cfg.windupPauseStart = 0.42f;
                    cfg.windupPauseDuration = 0.08f; // ~5 frames wind-up freeze at peak
                    cfg.windupTimeScale = 0.0f;
                    cfg.strikeSpeedMultiplier = 1.15f;
                    cfg.hitstopFrames = 5;           // Standard impact freeze (5 frames @ 60fps)
                    if (!stateJson.contains("hitboxStart")) cfg.hitboxStart = 0.48f;
                    if (!stateJson.contains("hitboxEnd")) cfg.hitboxEnd = 0.85f;
                }
            }

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
                    if (it.value().contains("requireHit")) tData.requireHit = it.value()["requireHit"].get<bool>();

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
    EnsureDefaultCombatSoundCues();
    RebuildAllStates();
}

#if HEIN_WITH_EDITOR
void HEIN::CombatStateMachineComponent::RenderNodeGraph()
{
    GenericGraphDesc graph;
    graph.graphTitle = "Combat State Machine - Node View";
    graph.activeNodeName = m_currentStateName;
    graph.onRenderToolbarExtra = [this]()
    {
        if (ImGui::Button("Open State Editor Window"))
        {
            m_showStateDetailsEditor = true;
        }
    };

    int nodeId = 1;
    int pinId = 1000;
    int linkId = 5000;

    std::unordered_map<int, std::string> nodeIdToName;
    std::unordered_map<std::string, int> stateToInputPin;
    std::unordered_map<std::string, int> transitionToOutputPin;

    // Build Generic Nodes
    for (const auto& [name, cfg] : m_stateConfigs)
    {
        GenericGraphNode node;
        node.id = nodeId;
        node.title = name;
        node.isActive = (name == m_currentStateName);

        if (node.isActive && m_currentState)
        {
            if (auto* universalState = dynamic_cast<UniversalCombatState*>(m_currentState))
            {
                float duration = universalState->GetDuration();
                float timer = universalState->GetTimer();
                node.progress = (duration > 0.0f) ? (timer / duration) : 0.0f;
            }
        }

        char subBuf[128];
        sprintf_s(subBuf, "Anim: %s | Dur: %.2fs | Spd: %.1f", cfg.animationName.c_str(), cfg.stateDuration, cfg.moveSpeed);
        node.subtitle = subBuf;

        // Master Input Pin
        node.inputPins.push_back({ pinId, "Enter", true });
        stateToInputPin[name] = pinId++;

        // Output Pins (Transitions)
        for (const auto& [eventTag, transition] : cfg.transitions)
        {
            node.outputPins.push_back({ pinId, eventTag + " -> " + transition.targetState, false });
            transitionToOutputPin[name + eventTag] = pinId++;
        }

        nodeIdToName[nodeId] = name;
        graph.nodes.push_back(node);
        nodeId++;
    }

    // Build Generic Links with dynamic chromatic pulse
    for (const auto& [name, cfg] : m_stateConfigs)
    {
        for (const auto& [eventTag, transition] : cfg.transitions)
        {
            if (stateToInputPin.find(transition.targetState) != stateToInputPin.end())
            {
                GenericGraphLink link;
                link.id = linkId++;
                link.startPinId = transitionToOutputPin[name + eventTag];
                link.endPinId = stateToInputPin[transition.targetState];
                link.isPulsing = (m_transitionPulseTimer > 0.0f && name == m_lastTransitionFrom && transition.targetState == m_lastTransitionTo);
                link.pulseTimer = m_transitionPulseTimer;
                graph.links.push_back(link);
            }
        }
    }

    int selectedNodeId = 0;
    m_nodeGraphView.Render(graph, &m_showNodeEditor, &selectedNodeId);

    if (selectedNodeId > 0 && nodeIdToName.find(selectedNodeId) != nodeIdToName.end())
    {
        m_selectedNodeName = nodeIdToName[selectedNodeId];
    }
}

void HEIN::CombatStateMachineComponent::RenderStateDetailsWindow()
{
    ImGui::SetNextWindowSize(ImVec2(500, 650), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Combat State Machine - State Editor", &m_showStateDetailsEditor))
    {
        ImGui::End();
        return;
    }

    if (m_stateConfigs.empty())
    {
        ImGui::TextDisabled("No states configured in this state machine.");
        ImGui::End();
        return;
    }

    bool configChanged = false;

    if (ImGui::BeginCombo("Editing State", m_selectedNodeName.empty() ? "<Select a State>" : m_selectedNodeName.c_str()))
    {
        for (auto& [name, cfg] : m_stateConfigs)
        {
            bool isSelected = (m_selectedNodeName == name);
            if (ImGui::Selectable(name.c_str(), isSelected))
            {
                m_selectedNodeName = name;
            }
            if (isSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    if (!m_selectedNodeName.empty() && m_stateConfigs.contains(m_selectedNodeName))
    {
        auto& cfg = m_stateConfigs[m_selectedNodeName];
        ImGui::PushID(m_selectedNodeName.c_str());

        bool isCurrent = (m_currentStateName == m_selectedNodeName);
        if (isCurrent)
        {
            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "[ACTIVE]");
            ImGui::SameLine();
        }

        if (ImGui::Button("Play State Live"))
        {
            ChangeState(m_selectedNodeName, 0.1f);
            ApplyPendingState();
        }

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
        if (ImGui::DragFloat3("Enter Impulse", &cfg.enterImpulse.x, 0.5f, -200.0f, 200.0f)) configChanged = true;

        if (ImGui::Checkbox("Is Looping", &cfg.isLooping)) configChanged = true;
        ImGui::SameLine();
        if (ImGui::Checkbox("Is Attack", &cfg.isAttack)) configChanged = true;
        ImGui::SameLine();
        if (ImGui::Checkbox("Is Block", &cfg.isBlock)) configChanged = true;
        ImGui::SameLine();
        if (ImGui::Checkbox("Is Guard Counter", &cfg.isGuardCounter)) configChanged = true;
        if (ImGui::Checkbox("Lock Direction", &cfg.lockMovementDirection)) configChanged = true;

        if (ImGui::DragFloatRange2("Invincibility Window", &cfg.invincibilityStart, &cfg.invincibilityEnd, 0.05f, 0.0f, 10.0f, "Start: %.2f", "End: %.2f"))
        {
            configChanged = true;
        }

        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "=== Combat State Role, Cost & Effects ===");
        const char* stateRoleItems[] = { "Normal", "Attack", "ComboFinisher", "Block", "Dodge", "Stun", "Jump", "GuardCounter", "Death" };
        int currentRoleIdx = static_cast<int>(cfg.stateTypeEnum);
        if (ImGui::Combo("Combat State Role", &currentRoleIdx, stateRoleItems, IM_ARRAYSIZE(stateRoleItems)))
        {
            cfg.stateTypeEnum = static_cast<CombatStateType>(currentRoleIdx);
            cfg.isAttack = cfg.IsAttackState();
            cfg.isBlock = cfg.IsBlockState();
            cfg.isGuardCounter = (cfg.stateTypeEnum == CombatStateType::GuardCounter);
            cfg.isDeath = cfg.IsDeathState();
            configChanged = true;
        }
        if (ImGui::DragFloat("Stamina Cost", &cfg.staminaCost, 0.5f, 0.0f, 200.0f, "%.1f (0 = Free)")) configChanged = true;
        if (ImGui::DragFloat("Stamina Regen Delay (-1 = Base Default)", &cfg.staminaRegenDelay, 0.05f, -1.0f, 5.0f, "%.2f s")) configChanged = true;

        if (cfg.isAttack)
        {
            if (ImGui::DragFloat("Stamina Damage (On Shield)", &cfg.staminaDamage, 1.0f, 0.0f, 200.0f, "%.0f")) configChanged = true;
            if (ImGui::SliderInt("Combo Step (-1=Auto, 0=Opener, 1=Mid, 2=Finisher)", &cfg.comboPermutationStep, -1, 2)) configChanged = true;

            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 0.65f, 0.2f, 1.0f), "(Wind-Up & Hitstop)");
            if (ImGui::DragFloat("Wind-Up Pause Start (s)", &cfg.windupPauseStart, 0.01f, 0.0f, 5.0f, "%.2f")) configChanged = true;
            if (ImGui::DragFloat("Wind-Up Hold Duration (s)", &cfg.windupPauseDuration, 0.01f, 0.0f, 2.0f, "%.2f")) configChanged = true;
            if (ImGui::SliderFloat("Wind-Up TimeScale (0=Freeze)", &cfg.windupTimeScale, 0.0f, 1.0f, "%.2f")) configChanged = true;
            if (ImGui::SliderFloat("Strike Release Speed", &cfg.strikeSpeedMultiplier, 0.5f, 2.5f, "%.2fx")) configChanged = true;
            if (ImGui::SliderInt("Hitstop Frames (60fps)", &cfg.hitstopFrames, 0, 20)) configChanged = true;
            if (ImGui::DragFloatRange2("Active Hitbox Window", &cfg.hitboxStart, &cfg.hitboxEnd, 0.02f, 0.0f, 5.0f, "Start: %.2f", "End: %.2f"))
            {
                configChanged = true;
            }
        }

        ImGui::Separator();
        if (ImGui::TreeNode("Transitions (Arrows / Links)"))
        {
            std::string transitionToRemove = "";
            for (auto& [eventTag, transition] : cfg.transitions)
            {
                ImGui::PushID(eventTag.c_str());
                ImGui::TextColored(ImVec4(1, 0.8f, 0, 1), "[ %s ] -> %s", eventTag.c_str(), transition.targetState.c_str());
                ImGui::SameLine();
                if (ImGui::Button("Delete"))
                {
                    transitionToRemove = eventTag;
                }
                ImGui::PopID();
            }
            if (!transitionToRemove.empty())
            {
                cfg.transitions.erase(transitionToRemove);
                configChanged = true;
            }
            ImGui::TreePop();
        }

        ImGui::PopID();
    }

    if (configChanged)
    {
        RebuildAllStates();
    }

    ImGui::End();
}

void HEIN::CombatStateMachineComponent::OnInspectorGUI(GameContext& /*gameContext*/)
{
    if (ImGui::CollapsingHeader("Combat State Machine Component", ImGuiTreeNodeFlags_DefaultOpen))
    {
        // TRIGGER FOR VISUAL GRAPH
        ImGui::Checkbox("Open Visual Combat Node View", &m_showNodeEditor);
        ImGui::SameLine();
        ImGui::Checkbox("Open Combat State Editor", &m_showStateDetailsEditor);

        // RUNTIME DEBUG INFO
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Active State: %s", m_currentStateName.c_str());
        if (auto* universalState = dynamic_cast<UniversalCombatState*>(m_currentState))
        {
            ImGui::SameLine();
            ImGui::TextDisabled("(Timer: %.2fs)", universalState->GetTimer());

            if (universalState->IsInWindupPause())
            {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.15f, 1.0f), "[WIND-UP PAUSE]");
            }
            if (universalState->IsWeaponHitboxActive())
            {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "[HITBOX ACTIVE]");
            }
        }
        if (m_owner)
        {
            ImGui::Text("Actor TimeScale: %.2f | Hitstop: %s (%.3fs)",
                m_owner->GetEffectiveTimeScale(),
                m_owner->IsHitstopped() ? "FROZEN" : "Active",
                m_owner->GetHitstopTimer());

            if (auto* pbb = m_owner->GetComponent<PlayerBlackBoard>())
            {
                if (ImGui::TreeNodeEx("Player Stamina Tuning", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    float stamPct = (pbb->maxBlockStamina > 0.0f) ? (pbb->currentBlockStamina / pbb->maxBlockStamina) : 0.0f;
                    char stamBuf[64];
                    sprintf_s(stamBuf, "%.1f / %.1f (%.0f%%)", pbb->currentBlockStamina, pbb->maxBlockStamina, stamPct * 100.0f);
                    ImGui::ProgressBar(std::clamp(stamPct, 0.0f, 1.0f), ImVec2(-1.0f, 18.0f), stamBuf);

                    if (ImGui::DragFloat("Stamina Recovery Rate (/s)", &pbb->staminaRecoveryRate, 0.5f, 0.0f, 300.0f, "%.1f pts/sec"))
                    {
                        pbb->blockRecoveryRate = pbb->staminaRecoveryRate;
                    }
                    ImGui::DragFloat("Blocking Recovery Rate (/s)", &pbb->blockHeldRecoveryRate, 0.5f, 0.0f, 150.0f, "%.1f pts/sec");
                    ImGui::DragFloat("Base Stamina Regen Delay (s)", &pbb->baseStaminaRegenDelay, 0.05f, 0.0f, 5.0f, "%.2fs");
                    ImGui::DragFloat("Exhaustion Penalty Delay (s)", &pbb->exhaustedRegenDelay, 0.05f, 0.0f, 5.0f, "%.2fs");
                    ImGui::DragFloat("Max Stamina", &pbb->maxBlockStamina, 1.0f, 10.0f, 1000.0f, "%.0f");
                    ImGui::TreePop();
                }
            }
        }
        ImGui::Separator();

        // ASSET MANAGEMENT
        char pathBuf[256] = { 0 };
        strncpy_s(pathBuf, m_configPath.c_str(), sizeof(pathBuf) - 1);
        if (ImGui::InputText("Combat Asset Path", pathBuf, sizeof(pathBuf)))
        {
            m_configPath = pathBuf;
        }
        if (!m_configPath.empty())
        {
            if (ImGui::Button("Reload from Asset"))
            {
                if (LoadFromConfigFile(m_configPath))
                {
                    RebuildAllStates();
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Save to Asset"))
            {
                SaveToConfigFile(m_configPath);
            }
        }
        ImGui::Separator();

        // SELECTED STATE EDITOR (Replaces the massive loop)
        bool configChanged = false;
        std::string stateToRemove = "";

        if (ImGui::BeginCombo("Editing State", m_selectedNodeName.empty() ? "<Select a State>" : m_selectedNodeName.c_str()))
        {
            for (auto& [name, cfg] : m_stateConfigs)
            {
                bool isSelected = (m_selectedNodeName == name);
                if (ImGui::Selectable(name.c_str(), isSelected))
                {
                    m_selectedNodeName = name;
                }
                if (isSelected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        if (!m_selectedNodeName.empty() && m_stateConfigs.contains(m_selectedNodeName))
        {
            auto& cfg = m_stateConfigs[m_selectedNodeName];
            ImGui::PushID(m_selectedNodeName.c_str());

            bool isCurrent = (m_currentStateName == m_selectedNodeName);
            if (isCurrent)
            {
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "[ACTIVE]");
                ImGui::SameLine();
            }

            if (ImGui::Button("Play State"))
            {
                ChangeState(m_selectedNodeName, 0.1f);
                ApplyPendingState();
            }

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
            if (ImGui::DragFloat3("Enter Impulse", &cfg.enterImpulse.x, 0.5f, -200.0f, 200.0f)) configChanged = true;

            if (ImGui::Checkbox("Is Looping", &cfg.isLooping)) configChanged = true;
            ImGui::SameLine();
            if (ImGui::Checkbox("Is Attack", &cfg.isAttack)) configChanged = true;
            ImGui::SameLine();
            if (ImGui::Checkbox("Is Block", &cfg.isBlock)) configChanged = true;
            ImGui::SameLine();
            if (ImGui::Checkbox("Is Guard Counter", &cfg.isGuardCounter)) configChanged = true;
            if (ImGui::Checkbox("Lock Direction", &cfg.lockMovementDirection)) configChanged = true;

            if (ImGui::DragFloatRange2("Invincibility Window", &cfg.invincibilityStart, &cfg.invincibilityEnd, 0.05f, 0.0f, 10.0f, "Start: %.2f", "End: %.2f"))
            {
                configChanged = true;
            }

            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "=== Combat State Role, Cost & Effects ===");
            const char* stateRoleItems[] = { "Normal", "Attack", "ComboFinisher", "Block", "Dodge", "Stun", "Jump", "GuardCounter", "Death" };
            int currentRoleIdx = static_cast<int>(cfg.stateTypeEnum);
            if (ImGui::Combo("Combat State Role", &currentRoleIdx, stateRoleItems, IM_ARRAYSIZE(stateRoleItems)))
            {
                cfg.stateTypeEnum = static_cast<CombatStateType>(currentRoleIdx);
                cfg.isAttack = cfg.IsAttackState();
                cfg.isBlock = cfg.IsBlockState();
                cfg.isGuardCounter = (cfg.stateTypeEnum == CombatStateType::GuardCounter);
                cfg.isDeath = cfg.IsDeathState();
                configChanged = true;
            }
            if (ImGui::DragFloat("Stamina Cost", &cfg.staminaCost, 0.5f, 0.0f, 200.0f, "%.1f (0 = Free)")) configChanged = true;
            if (ImGui::DragFloat("Stamina Regen Delay (-1 = Base Default)", &cfg.staminaRegenDelay, 0.05f, -1.0f, 5.0f, "%.2f s")) configChanged = true;

            if (cfg.isAttack)
            {
                if (ImGui::DragFloat("Stamina Damage (On Shield)", &cfg.staminaDamage, 1.0f, 0.0f, 200.0f, "%.0f")) configChanged = true;
                if (ImGui::SliderInt("Combo Step (-1=Auto, 0=Opener, 1=Mid, 2=Finisher)", &cfg.comboPermutationStep, -1, 2)) configChanged = true;

                ImGui::Separator();
                ImGui::TextColored(ImVec4(1.0f, 0.65f, 0.2f, 1.0f), "(Wind-Up & Hitstop)");
                if (ImGui::DragFloat("Wind-Up Pause Start (s)", &cfg.windupPauseStart, 0.01f, 0.0f, 5.0f, "%.2f")) configChanged = true;
                if (ImGui::DragFloat("Wind-Up Hold Duration (s)", &cfg.windupPauseDuration, 0.01f, 0.0f, 2.0f, "%.2f")) configChanged = true;
                if (ImGui::SliderFloat("Wind-Up TimeScale (0=Freeze)", &cfg.windupTimeScale, 0.0f, 1.0f, "%.2f")) configChanged = true;
                if (ImGui::SliderFloat("Strike Release Speed", &cfg.strikeSpeedMultiplier, 0.5f, 2.5f, "%.2fx")) configChanged = true;
                if (ImGui::SliderInt("Hitstop Frames (60fps)", &cfg.hitstopFrames, 0, 20)) configChanged = true;
                if (ImGui::DragFloatRange2("Active Hitbox Window", &cfg.hitboxStart, &cfg.hitboxEnd, 0.02f, 0.0f, 5.0f, "Start: %.2f", "End: %.2f"))
                {
                    configChanged = true;
                }
            }

            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "Modular State Action Effects (%zu active)", cfg.effects.size());

            if (ImGui::Button("+ Add Shockwave Effect"))
            {
                StateEffectConfig newWave;
                newWave.type = StateEffectType::Shockwave;
                newWave.triggerTime = cfg.hitboxStart;
                cfg.effects.push_back(newWave);
                configChanged = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("+ Add Camera Shake"))
            {
                StateEffectConfig newShake;
                newShake.type = StateEffectType::CameraShake;
                newShake.triggerTime = cfg.hitboxStart;
                cfg.effects.push_back(newShake);
                configChanged = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("+ Add Sound Cue"))
            {
                StateEffectConfig newSound;
                newSound.type = StateEffectType::SoundCue;
                newSound.triggerTime = (cfg.hitboxStart > 0.0f) ? cfg.hitboxStart : 0.1f;
                newSound.assetKey = "Slash1";
                if (m_owner)
                {
                    if (auto* sc = m_owner->GetComponent<SoundComponent>())
                    {
                        if (!sc->GetCues().empty())
                        {
                            newSound.assetKey = sc->GetCues().front().cueName;
                        }
                    }
                }
                cfg.effects.push_back(newSound);
                configChanged = true;
            }

            int effectToDelete = -1;
            for (int effIdx = 0; effIdx < static_cast<int>(cfg.effects.size()); ++effIdx)
            {
                auto& eff = cfg.effects[effIdx];
                ImGui::PushID(effIdx);
                ImGui::Separator();
                ImGui::Text("[%d] %s", effIdx + 1, StateEffectTypeToString(eff.type).c_str());
                ImGui::SameLine();
                if (ImGui::Button("Delete Effect"))
                {
                    effectToDelete = effIdx;
                }

                if (ImGui::DragFloat("Trigger Time (s)", &eff.triggerTime, 0.02f, 0.0f, 5.0f, "%.2f s")) configChanged = true;

                if (eff.type == StateEffectType::Shockwave)
                {
                    if (ImGui::DragFloat("Wave Radius", &eff.radius, 0.5f, 1.0f, 100.0f, "%.1f m")) configChanged = true;
                    if (ImGui::DragFloat("Wave Duration", &eff.duration, 0.05f, 0.1f, 5.0f, "%.2f s")) configChanged = true;
                    if (ImGui::DragFloat("Wave Damage", &eff.damage, 1.0f, 0.0f, 500.0f, "%.0f")) configChanged = true;
                    if (ImGui::DragFloat("Wave Knockback", &eff.knockbackForce, 1.0f, 0.0f, 100.0f, "%.1f")) configChanged = true;
                    if (ImGui::DragFloat("Forward Offset", &eff.forwardOffset, 0.5f, -20.0f, 50.0f, "%.1f m")) configChanged = true;
                    if (ImGui::Checkbox("Ground Only (Jump Evadable)", &eff.isGroundOnly)) configChanged = true;
                }
                else if (eff.type == StateEffectType::CameraShake)
                {
                    if (ImGui::SliderFloat("Shake Intensity", &eff.shakeIntensity, 0.0f, 2.0f, "%.2f")) configChanged = true;
                    if (ImGui::SliderFloat("Shake Duration", &eff.shakeDuration, 0.05f, 2.0f, "%.2f s")) configChanged = true;
                }
                else if (eff.type == StateEffectType::SoundCue)
                {
                    SoundComponent* soundComp = m_owner ? m_owner->GetComponent<SoundComponent>() : nullptr;
                    if (!soundComp)
                    {
                        ImGui::TextDisabled("No SoundComponent on this Actor yet.");
                        if (m_owner && ImGui::Button("+ Attach SoundComponent with Combat Preset"))
                        {
                            soundComp = m_owner->AddComponent<SoundComponent>();
                            soundComp->ApplyCombatCharacterPreset();
                            soundComp->Start();
                        }
                    }
                    else
                    {
                        const auto& cues = soundComp->GetCues();
                        if (!cues.empty())
                        {
                            if (ImGui::BeginCombo("SoundComponent Cue", eff.assetKey.empty() ? "<Select Cue>" : eff.assetKey.c_str()))
                            {
                                for (const auto& c : cues)
                                {
                                    bool isSel = (eff.assetKey == c.cueName);
                                    if (ImGui::Selectable(c.cueName.c_str(), isSel))
                                    {
                                        eff.assetKey = c.cueName;
                                        configChanged = true;
                                    }
                                    if (isSel) ImGui::SetItemDefaultFocus();
                                }
                                ImGui::EndCombo();
                            }
                        }
                    }

                    char cueBuf[128] = { 0 };
                    strncpy_s(cueBuf, eff.assetKey.c_str(), sizeof(cueBuf) - 1);
                    if (ImGui::InputText("Sound Cue Key", cueBuf, sizeof(cueBuf)))
                    {
                        eff.assetKey = cueBuf;
                        configChanged = true;
                    }
                    if (soundComp)
                    {
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Test Play"))
                        {
                            soundComp->PlayCueOrFallbackPath(eff.assetKey, AudioBus::CombatSFX, 1.0f);
                        }
                    }
                }
                ImGui::PopID();
            }

            if (effectToDelete >= 0 && effectToDelete < static_cast<int>(cfg.effects.size()))
            {
                cfg.effects.erase(cfg.effects.begin() + effectToDelete);
                configChanged = true;
            }
            ImGui::Separator();

            if (ImGui::TreeNode("State Collider Modifiers (Hurtbox Immunity / Root Resize)"))
            {
                std::vector<std::string> actorColliderTags;
                if (m_owner)
                {
                    for (auto* col : m_owner->GetComponents<ColliderComponent>())
                    {
                        if (!col) continue;
                        std::wstring wTag = col->GetColliderTag();
                        if (!wTag.empty())
                        {
                            actorColliderTags.emplace_back(wTag.begin(), wTag.end());
                        }
                    }
                }

                if (ImGui::Button("+ Add Collider Modifier"))
                {
                    StateColliderModifier newMod;
                    if (!actorColliderTags.empty()) newMod.colliderTag = actorColliderTags[0];
                    newMod.endTime = (cfg.stateDuration > 0.0f) ? cfg.stateDuration : 1.0f;
                    cfg.colliderModifiers.push_back(newMod);
                    configChanged = true;
                }

                int modToDelete = -1;
                for (int modIdx = 0; modIdx < static_cast<int>(cfg.colliderModifiers.size()); ++modIdx)
                {
                    auto& mod = cfg.colliderModifiers[modIdx];
                    ImGui::PushID(modIdx);
                    ImGui::Separator();

                    if (!actorColliderTags.empty())
                    {
                        int selectedIdx = 0;
                        for (int k = 0; k < static_cast<int>(actorColliderTags.size()); ++k)
                        {
                            if (actorColliderTags[k] == mod.colliderTag)
                            {
                                selectedIdx = k;
                                break;
                            }
                        }
                        if (ImGui::BeginCombo("Target Collider", mod.colliderTag.empty() ? "<Select>" : mod.colliderTag.c_str()))
                        {
                            for (int k = 0; k < static_cast<int>(actorColliderTags.size()); ++k)
                            {
                                bool isSelected = (selectedIdx == k);
                                if (ImGui::Selectable(actorColliderTags[k].c_str(), isSelected))
                                {
                                    mod.colliderTag = actorColliderTags[k];
                                    configChanged = true;
                                }
                                if (isSelected) ImGui::SetItemDefaultFocus();
                            }
                            ImGui::EndCombo();
                        }
                    }
                    else
                    {
                        char tagBuf[128] = { 0 };
                        strncpy_s(tagBuf, mod.colliderTag.c_str(), sizeof(tagBuf) - 1);
                        if (ImGui::InputText("Target Collider Tag", tagBuf, sizeof(tagBuf)))
                        {
                            mod.colliderTag = tagBuf;
                            configChanged = true;
                        }
                    }

                    ImGui::SameLine();
                    if (ImGui::SmallButton("Delete"))
                    {
                        modToDelete = modIdx;
                    }

                    if (ImGui::DragFloatRange2("Active Window (s)", &mod.startTime, &mod.endTime, 0.02f, 0.0f, 10.0f, "Start: %.2f", "End: %.2f")) configChanged = true;
                    if (ImGui::Checkbox("Collider Enabled (Uncheck = Immune)", &mod.enableCollider)) configChanged = true;
                    ImGui::SameLine();
                    if (ImGui::Checkbox("Follow Root Bone Y", &mod.followRootMotionY)) configChanged = true;
                    if (ImGui::DragFloat("Height Scale", &mod.heightScale, 0.02f, 0.1f, 5.0f, "%.2fx")) configChanged = true;
                    if (ImGui::DragFloat("Radius Scale", &mod.radiusScale, 0.02f, 0.1f, 5.0f, "%.2fx")) configChanged = true;
                    if (ImGui::DragFloat3("Offset Delta", &mod.offsetDelta.x, 0.5f, -200.0f, 200.0f)) configChanged = true;
                    ImGui::PopID();
                }

                if (modToDelete >= 0 && modToDelete < static_cast<int>(cfg.colliderModifiers.size()))
                {
                    cfg.colliderModifiers.erase(cfg.colliderModifiers.begin() + modToDelete);
                    configChanged = true;
                }
                ImGui::TreePop();
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

                    if (ImGui::Checkbox("Require Hit Confirm (Combos)", &transition.requireHit))
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
                        char comboAnimBuf[64];
                        strcpy_s(comboAnimBuf, cfg.comboAnimationNames[i].c_str());
                        if (ImGui::InputText("Anim Name", comboAnimBuf, sizeof(comboAnimBuf)))
                        {
                            cfg.comboAnimationNames[i] = comboAnimBuf;
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

            if (ImGui::Button("Delete This State", ImVec2(-1, 30)))
            {
                stateToRemove = m_selectedNodeName;
            }
            ImGui::PopID();
        }

        if (!stateToRemove.empty())
        {
            m_stateConfigs.erase(stateToRemove);
            m_selectedNodeName = "";
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
                m_selectedNodeName = newStateNameBuf;
                configChanged = true;
            }
        }

        if (configChanged)
        {
            RebuildAllStates();
        }
    }
}
#else
void HEIN::CombatStateMachineComponent::RenderNodeGraph() {}
void HEIN::CombatStateMachineComponent::RenderStateDetailsWindow() {}
void HEIN::CombatStateMachineComponent::OnInspectorGUI(GameContext& /*gameContext*/) {}
#endif
} // namespace HEIN
