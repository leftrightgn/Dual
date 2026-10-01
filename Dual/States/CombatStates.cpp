#include "pch.h" 
#include "States/CombatStates.h"
#include "Components/CombatStateMachineComponent.h"
#include <BlackBoard/CombatBlackBoard.h>
#include "../../../External/Engine/Components/SkinnedModelComponent.h"
#include "../../../External/Engine/Components/TransformComponent.h"
#include "../../../External/Engine/Components/HealthComponent.h"
#include <cmath>

// ==============================================================================
// IDLE STATE
// ==============================================================================
HEIN::IdleState::IdleState(const HEIN::StateConfig& config) : m_config(config) {}

void HEIN::IdleState::OnEnter(Actor* owner, CombatStateMachineComponent* /*stateMachine*/, float blendDuration)
{
    HEIN::CombatBlackBoard* blackboard = owner->GetComponent<CombatBlackBoard>();
    if (blackboard) blackboard->currentStance = CombatStance::Idle;

    std::vector<HEIN::SkinnedModelComponent*> models = owner->GetComponents<SkinnedModelComponent>();
    for (HEIN::SkinnedModelComponent* model : models)
    {
        model->CrossfadeAnimation(m_config.animationName, blendDuration);
    }
}

void HEIN::IdleState::Update(Actor* owner, CombatStateMachineComponent* stateMachine, float /*deltaTime*/)
{
    HEIN::CombatBlackBoard* blackboard = owner->GetComponent<CombatBlackBoard>();
    if (!blackboard) return;

    blackboard->currentTurnSpeed = 12.0f;

    if (blackboard->moveIntent.LengthSquared() > 0.1f)
    {
        if (blackboard->isLockedOn && std::abs(blackboard->localMoveIntent.x) >= std::abs(blackboard->localMoveIntent.z))
        {
            auto& t = m_config.transitions["OnStrafe"];
            stateMachine->ChangeState(t.targetState, t.blendDuration);
        }
        else
        {
            auto& t = m_config.transitions["OnMove"];
            stateMachine->ChangeState(t.targetState, t.blendDuration);
        }
    }
}

bool HEIN::IdleState::HandleMessage(Actor* owner, CombatStateMachineComponent* stateMachine, Message::MessageID messageID)
{
    switch (messageID)
    {
    case Message::PLAYER_ACTION_ATTACK:
    {
        auto& t = m_config.transitions["OnAttack"];
        stateMachine->ChangeState(t.targetState, t.blendDuration);
        return true;
    }
    case Message::PLAYER_ACTION_DODGE:
    {
        auto& t = m_config.transitions["OnDodge"];
        stateMachine->ChangeState(t.targetState, t.blendDuration);
        return true;
    }
    case Message::PLAYER_ACTION_BLOCK:
    {
        auto& t = m_config.transitions["OnBlock"];
        stateMachine->ChangeState(t.targetState, t.blendDuration);
        return true;
    }
    }
    return false;
}

void HEIN::IdleState::OnExit(Actor* /*owner*/, CombatStateMachineComponent* /*stateMachine*/) {}

// ==============================================================================
// WALK STATE
// ==============================================================================
HEIN::WalkState::WalkState(const HEIN::StateConfig& config) : m_config(config) {}

void HEIN::WalkState::OnEnter(Actor* owner, CombatStateMachineComponent* /*stateMachine*/, float blendDuration)
{
    HEIN::CombatBlackBoard* blackboard = owner->GetComponent<CombatBlackBoard>();
    if (blackboard)
    {
        blackboard->currentStance = CombatStance::Walking;
        blackboard->currentSpeed = m_config.moveSpeed;
    }
    std::vector<HEIN::SkinnedModelComponent*> models = owner->GetComponents<SkinnedModelComponent>();
    for (HEIN::SkinnedModelComponent* model : models)
    {
        model->CrossfadeAnimation(m_config.animationName, blendDuration);
    }
}

void HEIN::WalkState::Update(Actor* owner, CombatStateMachineComponent* stateMachine, float /*deltaTime*/)
{
    HEIN::CombatBlackBoard* blackboard = owner->GetComponent<CombatBlackBoard>();
    if (!blackboard) return;
    blackboard->currentSpeed = m_config.moveSpeed;
    blackboard->currentTurnSpeed = 12.0f;

    if (blackboard->moveIntent.LengthSquared() <= 0.1f)
    {
        auto& t = m_config.transitions["OnStop"];
        stateMachine->ChangeState(t.targetState, t.blendDuration);
        return;
    }

    if (blackboard->isLockedOn && std::abs(blackboard->localMoveIntent.x) >= std::abs(blackboard->localMoveIntent.z))
    {
        auto& t = m_config.transitions["OnStrafe"];
        stateMachine->ChangeState(t.targetState, t.blendDuration);
        return;
    }
}

bool HEIN::WalkState::HandleMessage(Actor* owner, CombatStateMachineComponent* stateMachine, Message::MessageID messageID)
{
    switch (messageID)
    {
    case Message::PLAYER_ACTION_ATTACK:
    {
        auto& t = m_config.transitions["OnAttack"];
        stateMachine->ChangeState(t.targetState, t.blendDuration);
        return true;
    }
    case Message::PLAYER_ACTION_DODGE:
    {
        auto& t = m_config.transitions["OnDodge"];
        stateMachine->ChangeState(t.targetState, t.blendDuration);
        return true;
    }
    case Message::PLAYER_ACTION_BLOCK:
    {
        auto& t = m_config.transitions["OnBlock"];
        stateMachine->ChangeState(t.targetState, t.blendDuration);
        return true;
    }
    }
    return false;
}

void HEIN::WalkState::OnExit(Actor* /*owner*/, CombatStateMachineComponent* /*stateMachine*/) {}

// ==============================================================================
// ONE HAND ATTACK STATE
// ==============================================================================
HEIN::OneHandAttackState::OneHandAttackState(const StateConfig& config) : m_config(config) {}

int HEIN::OneHandAttackState::GetTotalStages(Actor* /*owner*/) const
{
    if (!m_config.comboAnimationNames.empty())
    {
        return static_cast<int>(m_config.comboAnimationNames.size());
    }
    if (!m_config.comboEndTimes.empty())
    {
        return static_cast<int>(m_config.comboEndTimes.size());
    }
    if (!m_config.animationName.empty())
    {
        return 1;
    }
    return 0;
}

const std::string* HEIN::OneHandAttackState::StageAnim(int stage) const
{
    if (stage >= 0 && stage < static_cast<int>(m_config.comboAnimationNames.size()))
    {
        return &m_config.comboAnimationNames[stage];
    }
    if (stage == 0 && !m_config.animationName.empty())
    {
        return &m_config.animationName;
    }
    // If comboAnimationNames is empty, fallback to primary animation
    if (m_config.comboAnimationNames.empty() && !m_config.animationName.empty())
    {
        return &m_config.animationName;
    }
    return nullptr;
}

float HEIN::OneHandAttackState::StageEndTime(int stage, Actor* owner) const
{
    if (stage >= 0 && stage < static_cast<int>(m_config.comboEndTimes.size()) && m_config.comboEndTimes[stage] > 0.0f)
    {
        return m_config.comboEndTimes[stage];
    }

    // Try auto-detecting duration from SkinnedModelComponent
    if (owner != nullptr)
    {
        const std::string* anim = StageAnim(stage);
        if (anim != nullptr && !anim->empty())
        {
            auto* model = owner->GetComponent<SkinnedModelComponent>();
            if (model != nullptr)
            {
                float duration = model->GetAnimationDuration(*anim);
                if (duration > 0.05f)
                {
                    return duration;
                }
            }
        }
    }

    // Built-in defaults for standard stages if available
    if (stage == 0) return STAGE_1_END_TIME;
    if (stage == 1) return STAGE_2_END_TIME;
    if (stage == 2) return STAGE_3_END_TIME;

    if (m_config.stateDuration > 0.0f)
    {
        return m_config.stateDuration;
    }

    return 1.2f;
}

float HEIN::OneHandAttackState::StageWindowStart(int stage, Actor* owner) const
{
    if (stage >= 0 && stage < static_cast<int>(m_config.comboWindowStarts.size()) && m_config.comboWindowStarts[stage] > 0.0f)
    {
        return m_config.comboWindowStarts[stage];
    }

    // Built-in defaults for standard stages if available
    if (stage == 0 && m_config.comboWindowStarts.empty()) return STAGE_1_WINDOW_START;
    if (stage == 1 && m_config.comboWindowStarts.size() <= 1) return STAGE_2_WINDOW_START;
    if (stage == 2 && m_config.comboWindowStarts.size() <= 2) return STAGE_3_WINDOW_START;

    // Dynamically calculate window start based on stage end time
    float endTime = StageEndTime(stage, owner);
    float windowStart = (endTime > 0.8f) ? 0.40f : (endTime * 0.40f);
    if (windowStart < 0.0f) windowStart = 0.0f;
    return windowStart;
}

float HEIN::OneHandAttackState::StageBlend(int stage, float fallback) const
{
    if (stage >= 0 && stage < static_cast<int>(m_config.comboBlendDurations.size()) && m_config.comboBlendDurations[stage] >= 0.0f)
    {
        return m_config.comboBlendDurations[stage];
    }
    return fallback;
}

void HEIN::OneHandAttackState::PlayStage(Actor* owner, int stage, float blendDuration)
{
    if (!owner) return;
    const std::string* anim = StageAnim(stage);
    if (anim == nullptr || anim->empty()) return;

    std::vector<HEIN::SkinnedModelComponent*> models = owner->GetComponents<SkinnedModelComponent>();
    for (HEIN::SkinnedModelComponent* model : models)
    {
        if (model)
        {
            model->CrossfadeAnimation(*anim, blendDuration, true);
        }
    }
}

void HEIN::OneHandAttackState::OnEnter(Actor* owner, CombatStateMachineComponent* /*stateMachine*/, float blendDuration)
{
    HEIN::CombatBlackBoard* blackboard = owner ? owner->GetComponent<CombatBlackBoard>() : nullptr;
    if (blackboard) blackboard->currentStance = CombatStance::OneHand;

    m_timer = 0.0f;
    m_comboStage = 0;
    m_comboQueued = false;

    float blend = StageBlend(0, blendDuration);
    PlayStage(owner, 0, blend);
}

void HEIN::OneHandAttackState::Update(Actor* owner, CombatStateMachineComponent* stateMachine, float deltaTime)
{
    if (!owner || !stateMachine) return;

    m_timer += deltaTime;
    HEIN::CombatBlackBoard* blackboard = owner->GetComponent<CombatBlackBoard>();

    int totalStages = GetTotalStages(owner);
    if (totalStages <= 0)
    {
        auto it = m_config.transitions.find("OnStop");
        std::string target = (it != m_config.transitions.end()) ? it->second.targetState : "Idle";
        float blend = (it != m_config.transitions.end()) ? it->second.blendDuration : 0.2f;
        stateMachine->ChangeState(target, blend);
        return;
    }

    if (blackboard)
    {
        float windUpTime = 0.3f;
        blackboard->currentTurnSpeed = (m_timer < windUpTime) ? 40.0f : 0.1f;
        blackboard->currentSpeed = m_config.moveSpeed;
    }

    float stageEnd = StageEndTime(m_comboStage, owner);

    // When the current animation stage finishes...
    if (m_timer >= stageEnd)
    {
        // Advance combo if queued AND there are more stages ahead
        if (m_comboQueued && m_comboStage < totalStages - 1)
        {
            m_comboStage++;
            m_timer = 0.0f;        // Reset timer for the next animation stage
            m_comboQueued = false; // Reset the queue flag

            float blend = StageBlend(m_comboStage, 0.1f);
            PlayStage(owner, m_comboStage, blend);
            return; // Exit early to remain in OneHandAttackState
        }

        // Exit attack state completely
        float exitBlend = (m_config.comboExitBlendDuration > 0.0f) ? m_config.comboExitBlendDuration : 0.3f;

        if (blackboard && blackboard->moveIntent.LengthSquared() > 0.01f)
        {
            if (blackboard->isLockedOn)
            {
                auto it = m_config.transitions.find("OnStrafe");
                if (it != m_config.transitions.end())
                {
                    stateMachine->ChangeState(it->second.targetState, it->second.blendDuration);
                    return;
                }
            }

            auto it = m_config.transitions.find("OnMove");
            if (it != m_config.transitions.end())
            {
                stateMachine->ChangeState(it->second.targetState, it->second.blendDuration);
                return;
            }
        }

        auto it = m_config.transitions.find("OnStop");
        if (it != m_config.transitions.end())
        {
            stateMachine->ChangeState(it->second.targetState, exitBlend);
        }
        else
        {
            stateMachine->ChangeState("Idle", exitBlend);
        }
    }
}

bool HEIN::OneHandAttackState::HandleMessage(Actor* owner, CombatStateMachineComponent* stateMachine, Message::MessageID messageID)
{
    if (messageID == Message::PLAYER_ACTION_ATTACK)
    {
        int totalStages = GetTotalStages(owner);
        // Only accept/queue combo if there is a next stage to advance to
        if (m_comboStage < totalStages - 1)
        {
            float windowStart = StageWindowStart(m_comboStage, owner);
            float stageEnd = StageEndTime(m_comboStage, owner);

            if (m_timer >= windowStart && m_timer < stageEnd)
            {
                m_comboQueued = true;
                return true;
            }
        }
        return false;
    }

    if (messageID == Message::PLAYER_ACTION_DODGE)
    {
        auto it = m_config.transitions.find("OnDodge");
        if (it != m_config.transitions.end())
        {
            stateMachine->ChangeState(it->second.targetState, it->second.blendDuration);
            return true;
        }
    }

    return false;
}

void HEIN::OneHandAttackState::OnExit(Actor* /*owner*/, CombatStateMachineComponent* /*stateMachine*/)
{
    m_comboQueued = false;
    m_comboStage = 0;
    m_timer = 0.0f;
}

// ==============================================================================
// DODGE STATE
// ==============================================================================
HEIN::DodgeState::DodgeState(const StateConfig& config) : m_config(config) {}

void HEIN::DodgeState::OnEnter(Actor* owner, CombatStateMachineComponent* stateMachine, float blendDuration)
{
    HEIN::CombatBlackBoard* blackboard = owner->GetComponent<CombatBlackBoard>();
    if (blackboard)
    {
        blackboard->currentStance = CombatStance::Dodging;
        blackboard->dodgeCooldownTimer = blackboard->maxDodgeCooldown;
        if (blackboard->moveIntent.LengthSquared() > 0.01f)
        {
            m_lockedDirection = blackboard->moveIntent;
        }
        else
        {
            HEIN::TransformComponent* trans = owner->GetComponent<HEIN::TransformComponent>();
            float currentMathematicalYaw = trans->GetRotationEuler().y;
            float trueVisualYaw = currentMathematicalYaw - DirectX::XM_PI;
            m_lockedDirection.x = sinf(trueVisualYaw);
            m_lockedDirection.y = 0.0f;
            m_lockedDirection.z = cosf(trueVisualYaw);
            m_lockedDirection.Normalize();
        }
    }
    std::vector<HEIN::SkinnedModelComponent*> models = owner->GetComponents<SkinnedModelComponent>();
    for (HEIN::SkinnedModelComponent* model : models)
    {
        model->CrossfadeAnimation(m_config.animationName, blendDuration);
    }
    m_timer = 0.0f;
}

void HEIN::DodgeState::Update(Actor* owner, CombatStateMachineComponent* stateMachine, float deltaTime)
{
    m_timer += deltaTime;
    HEIN::CombatBlackBoard* blackboard = owner->GetComponent<HEIN::CombatBlackBoard>();

    if (blackboard)
    {
        blackboard->currentTurnSpeed = 30.0f;
        blackboard->currentSpeed = m_config.moveSpeed;
        blackboard->moveIntent = m_lockedDirection;

        if (m_timer >= m_config.stateDuration)
        {
            if (blackboard->moveIntent.LengthSquared() > 0.1f)
            {
                if (blackboard->isLockedOn)
                {
                    auto& t = m_config.transitions["OnStrafe"];
                    stateMachine->ChangeState(t.targetState, t.blendDuration);
                }
                else
                {
                    auto& t = m_config.transitions["OnMove"];
                    stateMachine->ChangeState(t.targetState, t.blendDuration);
                }
            }
            else
            {
                auto& t = m_config.transitions["OnStop"];
                stateMachine->ChangeState(t.targetState, t.blendDuration);
            }
        }
    }
}

bool HEIN::DodgeState::HandleMessage(Actor* owner, CombatStateMachineComponent* stateMachine, Message::MessageID messageID)
{
    return false;
}

void HEIN::DodgeState::OnExit(Actor* owner, CombatStateMachineComponent* stateMachine) {}

// ==============================================================================
// STRAFE STATE
// ==============================================================================
HEIN::StrafeState::StrafeState(const StateConfig& config) : m_config(config), m_isRight(false) {}

void HEIN::StrafeState::OnEnter(Actor* owner, CombatStateMachineComponent* stateMachine, float blendDuration)
{
    HEIN::CombatBlackBoard* blackboard = owner->GetComponent<CombatBlackBoard>();
    if (blackboard)
    {
        blackboard->currentStance = CombatStance::Strafing;
        m_isRight = blackboard->localMoveIntent.x < 0.0f;
    }

    std::vector<HEIN::SkinnedModelComponent*> models = owner->GetComponents<SkinnedModelComponent>();
    for (HEIN::SkinnedModelComponent* model : models)
    {
        model->CrossfadeAnimation(m_isRight ? m_config.animationName : m_config.secondaryAnimationName, blendDuration);
    }
}

void HEIN::StrafeState::Update(Actor* owner, CombatStateMachineComponent* stateMachine, float deltaTime)
{
    HEIN::CombatBlackBoard* blackboard = owner->GetComponent<HEIN::CombatBlackBoard>();
    if (!blackboard) return;

    blackboard->currentSpeed = m_config.moveSpeed;

    if (!blackboard->isLockedOn || std::abs(blackboard->localMoveIntent.z) > std::abs(blackboard->localMoveIntent.x))
    {
        auto& t = m_config.transitions["OnMove"];
        stateMachine->ChangeState(t.targetState, t.blendDuration);
        return;
    }

    if (blackboard->moveIntent.LengthSquared() <= 0.1f)
    {
        auto& t = m_config.transitions["OnStop"];
        stateMachine->ChangeState(t.targetState, t.blendDuration);
        return;
    }

    bool isRight = blackboard->localMoveIntent.x < 0.0f;
    if (isRight != m_isRight)
    {
        m_isRight = isRight;
        std::vector<HEIN::SkinnedModelComponent*> models = owner->GetComponents<SkinnedModelComponent>();
        for (HEIN::SkinnedModelComponent* model : models)
        {
            // Mid-state directional shift uses a fast 0.2f blend
            model->CrossfadeAnimation(m_isRight ? m_config.animationName : m_config.secondaryAnimationName, 0.2f);
        }
    }
}

bool HEIN::StrafeState::HandleMessage(Actor* owner, CombatStateMachineComponent* stateMachine, Message::MessageID messageID)
{
    switch (messageID)
    {
    case Message::PLAYER_ACTION_ATTACK:
    {
        auto& t = m_config.transitions["OnAttack"];
        stateMachine->ChangeState(t.targetState, t.blendDuration);
        return true;
    }
    case Message::PLAYER_ACTION_DODGE:
    {
        auto& t = m_config.transitions["OnDodge"];
        stateMachine->ChangeState(t.targetState, t.blendDuration);
        return true;
    }
    case Message::PLAYER_ACTION_BLOCK:
    {
        auto& t = m_config.transitions["OnBlock"];
        stateMachine->ChangeState(t.targetState, t.blendDuration);
        return true;
    }
    }
    return false;
}

void HEIN::StrafeState::OnExit(Actor* owner, CombatStateMachineComponent* stateMachine) {}

// ==============================================================================
// BLOCK STATE
// ==============================================================================
HEIN::BlockState::BlockState(const StateConfig& config) : m_config(config) {}

void HEIN::BlockState::OnEnter(Actor* owner, CombatStateMachineComponent* stateMachine, float blendDuration)
{
    HEIN::CombatBlackBoard* blackboard = owner->GetComponent<CombatBlackBoard>();
    if (blackboard) blackboard->currentStance = CombatStance::Blocking;

    std::vector<HEIN::SkinnedModelComponent*> models = owner->GetComponents<SkinnedModelComponent>();
    for (HEIN::SkinnedModelComponent* model : models)
    {
        model->CrossfadeAnimation(m_config.animationName, blendDuration);
    }
}

void HEIN::BlockState::Update(Actor* owner, CombatStateMachineComponent* stateMachine, float deltaTime)
{
    HEIN::CombatBlackBoard* blackboard = owner->GetComponent<HEIN::CombatBlackBoard>();
    if (blackboard)
    {
        blackboard->currentBlockStamina -= deltaTime;
        blackboard->currentSpeed = m_config.moveSpeed;

        if (blackboard->currentBlockStamina <= 0.0f)
        {
            blackboard->isBlockBroken = true;

            if (blackboard->moveIntent.LengthSquared() > 0.1f)
            {
                if (blackboard->isLockedOn)
                {
                    auto& t = m_config.transitions["OnStrafe"];
                    stateMachine->ChangeState(t.targetState, t.blendDuration);
                }
                else
                {
                    auto& t = m_config.transitions["OnMove"];
                    stateMachine->ChangeState(t.targetState, t.blendDuration);
                }
            }
            else
            {
                auto& t = m_config.transitions["OnStop"];
                stateMachine->ChangeState(t.targetState, t.blendDuration);
            }
        }
    }
}

bool HEIN::BlockState::HandleMessage(Actor* owner, CombatStateMachineComponent* stateMachine, Message::MessageID messageID)
{
    if (messageID == Message::PLAYER_STOP_BLOCK)
    {
        HEIN::CombatBlackBoard* blackboard = owner->GetComponent<HEIN::CombatBlackBoard>();
        if (blackboard && blackboard->moveIntent.LengthSquared() > 0.1f)
        {
            if (blackboard->isLockedOn)
            {
                auto& t = m_config.transitions["OnStrafe"];
                stateMachine->ChangeState(t.targetState, t.blendDuration);
            }
            else
            {
                auto& t = m_config.transitions["OnMove"];
                stateMachine->ChangeState(t.targetState, t.blendDuration);
            }
        }
        else
        {
            auto& t = m_config.transitions["OnStop"];
            stateMachine->ChangeState(t.targetState, t.blendDuration);
        }
        return true;
    }

    if (messageID == Message::PLAYER_ACTION_DODGE)
    {
        auto& t = m_config.transitions["OnDodge"];
        stateMachine->ChangeState(t.targetState, t.blendDuration);
        return true;
    }
    return false;
}

void HEIN::BlockState::OnExit(Actor* owner, CombatStateMachineComponent* stateMachine) {}

// ==============================================================================
// UNIVERSAL DATA-DRIVEN COMBAT STATE
// ==============================================================================
HEIN::UniversalCombatState::UniversalCombatState(const StateConfig& config)
    : m_config(config)
{
}

void HEIN::UniversalCombatState::OnEnter(Actor* owner, CombatStateMachineComponent* /*stateMachine*/, float blendDuration)
{
    m_timer = 0.0f;
    m_hasQueuedTransition = false;
    m_stopDebounceTimer = 0.0f;

    auto* blackboard = owner->GetComponent<CombatBlackBoard>();
    if (blackboard)
    {
        if (m_config.isBlock)
        {
            blackboard->currentStance = CombatStance::Blocking;
        }
        else if (m_config.isAttack)
        {
            blackboard->currentStance = CombatStance::AttackRelese;
        }
        else if (m_config.lockMovementDirection || m_config.invincibilityEnd > 0.0f)
        {
            blackboard->currentStance = CombatStance::Dodging;
        }
        else if (m_config.stateName == "Strafe")
        {
            blackboard->currentStance = CombatStance::Strafing;
        }
        else if (m_config.moveSpeed > 0.0f)
        {
            blackboard->currentStance = CombatStance::Walking;
        }
        else
        {
            blackboard->currentStance = CombatStance::Idle;
        }

        blackboard->currentSpeed = m_config.moveSpeed;
        blackboard->currentTurnSpeed = m_config.turnSpeed;

        if (m_config.lockMovementDirection)
        {
            if (blackboard->moveIntent.LengthSquared() > 0.01f)
            {
                m_lockedDirection = blackboard->moveIntent;
            }
            else
            {
                auto* transform = owner->GetComponent<TransformComponent>();
                if (transform)
                {
                    m_lockedDirection = transform->GetForward();
                }
                else
                {
                    m_lockedDirection = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 1.0f);
                }
            }
            m_lockedDirection.y = 0.0f;
            if (m_lockedDirection.LengthSquared() > 0.001f) m_lockedDirection.Normalize();
            blackboard->moveIntent = m_lockedDirection;
        }
    }

    m_actualDuration = m_config.stateDuration;
    auto* model = owner->GetComponent<SkinnedModelComponent>();
    if (m_actualDuration <= 0.0f && model != nullptr && !m_config.animationName.empty())
    {
        float animDur = model->GetAnimationDuration(m_config.animationName);
        m_actualDuration = (animDur > 0.01f) ? animDur : 1.0f;
    }
    if (m_actualDuration <= 0.0f) m_actualDuration = 1.0f;

    if (model != nullptr && !m_config.animationName.empty())
    {
        model->CrossfadeAnimation(m_config.animationName, blendDuration, !m_config.isLooping);
    }
}

void HEIN::UniversalCombatState::Update(Actor* owner, CombatStateMachineComponent* stateMachine, float deltaTime)
{
    m_timer += deltaTime;

    auto* blackboard = owner->GetComponent<CombatBlackBoard>();
    if (blackboard)
    {
        blackboard->currentSpeed = m_config.moveSpeed;
        blackboard->currentTurnSpeed = m_config.turnSpeed;

        if (m_config.lockMovementDirection)
        {
            blackboard->moveIntent = m_lockedDirection;
        }
    }

    auto* health = owner->GetComponent<HealthComponent>();
    if (health)
    {
        bool isInvincible = (m_config.invincibilityEnd > 0.0f &&
                             m_timer >= m_config.invincibilityStart &&
                             m_timer <= m_config.invincibilityEnd);
        health->SetGameplayInvincible(isInvincible || m_config.isBlock);
    }

    if (m_config.isBlock && blackboard)
    {
        if (blackboard->currentBlockStamina <= 0.0f)
        {
            blackboard->isBlockBroken = true;
            auto itStop = m_config.transitions.find("OnStop");
            if (itStop != m_config.transitions.end())
            {
                stateMachine->ChangeState(itStop->second.targetState, itStop->second.blendDuration);
                return;
            }
        }
    }

    // Queued transition from input buffering
    if (m_hasQueuedTransition)
    {
        if (m_queuedTransition.canInterrupt || m_timer >= m_queuedTransition.windowStart)
        {
            stateMachine->ChangeState(m_queuedTransition.targetState, m_queuedTransition.blendDuration);
            m_hasQueuedTransition = false;
            return;
        }
    }

    // Movement cancellation for non-looping states (e.g. holding W cancels attack recovery window)
    if (!m_config.isLooping && blackboard && blackboard->moveIntent.LengthSquared() > 0.1f)
    {
        auto itMove = m_config.transitions.find("OnMove");
        if (itMove != m_config.transitions.end())
        {
            const auto& t = itMove->second;
            if (t.canInterrupt || (m_timer >= t.windowStart && m_timer <= t.windowEnd))
            {
                stateMachine->ChangeState(t.targetState, t.blendDuration);
                return;
            }
        }
    }

    // Automatic Exit Time / Timeout transitions for non-looping states (Attacks, Dodge)
    if (!m_config.isLooping && m_timer >= m_actualDuration)
    {
        // If moving when animation concludes, prefer OnMove over Idle
        if (blackboard && blackboard->moveIntent.LengthSquared() > 0.1f)
        {
            auto itMove = m_config.transitions.find("OnMove");
            if (itMove != m_config.transitions.end())
            {
                stateMachine->ChangeState(itMove->second.targetState, itMove->second.blendDuration);
                return;
            }
        }

        for (const auto& [key, t] : m_config.transitions)
        {
            if (t.hasExitTime || key == "OnFinished" || key == "OnStop" || t.triggerEvent == "OnFinished")
            {
                stateMachine->ChangeState(t.targetState, t.blendDuration);
                return;
            }
        }

        // Default fallback: return to Idle
        stateMachine->ChangeState("Idle", 0.2f);
        return;
    }

    // Directional auto-transitions for looping movement states
    if (m_config.isLooping && blackboard)
    {
        if (m_config.moveSpeed <= 0.001f) // Idle-like
        {
            if (blackboard->moveIntent.LengthSquared() > 0.1f)
            {
                if (blackboard->isLockedOn && std::abs(blackboard->localMoveIntent.x) >= std::abs(blackboard->localMoveIntent.z))
                {
                    auto it = m_config.transitions.find("OnStrafe");
                    if (it != m_config.transitions.end())
                    {
                        stateMachine->ChangeState(it->second.targetState, it->second.blendDuration);
                        return;
                    }
                }
                auto it = m_config.transitions.find("OnMove");
                if (it != m_config.transitions.end())
                {
                    stateMachine->ChangeState(it->second.targetState, it->second.blendDuration);
                    return;
                }
            }
        }
        else // Walk-like
        {
            if (blackboard->moveIntent.LengthSquared() <= 0.01f)
            {
                m_stopDebounceTimer += deltaTime;
                if (m_stopDebounceTimer >= 0.08f)
                {
                    auto it = m_config.transitions.find("OnStop");
                    if (it != m_config.transitions.end())
                    {
                        stateMachine->ChangeState(it->second.targetState, it->second.blendDuration);
                        return;
                    }
                }
            }
            else
            {
                m_stopDebounceTimer = 0.0f;
            }
        }
    }
}

void HEIN::UniversalCombatState::OnExit(Actor* owner, CombatStateMachineComponent* /*stateMachine*/)
{
    m_hasQueuedTransition = false;
    auto* health = owner->GetComponent<HealthComponent>();
    if (health)
    {
        health->SetGameplayInvincible(false);
    }
}

bool HEIN::UniversalCombatState::HandleMessage(Actor* owner, CombatStateMachineComponent* stateMachine, Message::MessageID messageID)
{
    std::string eventName = "";
    std::string fallbackEventName = "";

    auto* blackboard = owner->GetComponent<CombatBlackBoard>();

    switch (messageID)
    {
    case Message::PLAYER_ACTION_ATTACK:
        if (blackboard && blackboard->localMoveIntent.z > 0.5f)
        {
            eventName = "OnForwardAttack";
            fallbackEventName = "OnAttack";
        }
        else if (blackboard && blackboard->localMoveIntent.z < -0.5f)
        {
            eventName = "OnBackAttack";
            fallbackEventName = "OnAttack";
        }
        else
        {
            eventName = "OnAttack";
        }
        break;

    case Message::PLAYER_ACTION_DODGE:
        eventName = "OnDodge";
        break;

    case Message::PLAYER_ACTION_BLOCK:
        eventName = "OnBlock";
        break;

    case Message::PLAYER_STOP_BLOCK:
        eventName = "OnStopBlock";
        fallbackEventName = "OnStop";
        break;

    default:
        return false;
    }

    auto TryTransition = [&](const TransitionData& t) -> bool
    {
        // 1. Instant Cancel / Skip Animation:
        // If canInterrupt is true, OR windowStart <= 0.001f, OR within combo window:
        // Immediately skip current animation and chain directly to target state!
        if (t.canInterrupt || t.windowStart <= 0.001f || (m_timer >= t.windowStart && m_timer <= t.windowEnd))
        {
            stateMachine->ChangeState(t.targetState, t.blendDuration);
            m_hasQueuedTransition = false;
            return true;
        }

        // 2. Combo Buffering:
        // If pressed before windowStart, queue it so it never gets dropped and triggers the moment the window opens!
        if (m_timer < t.windowStart)
        {
            m_queuedTransition = t;
            m_hasQueuedTransition = true;
            return true;
        }
        return false;
    };

    // 1. Direct key match in transitions map
    auto it = m_config.transitions.find(eventName);
    if (it != m_config.transitions.end())
    {
        if (TryTransition(it->second)) return true;
    }

    // 2. Fallback key match (e.g. OnForwardAttack -> OnAttack)
    if (!fallbackEventName.empty())
    {
        auto itFb = m_config.transitions.find(fallbackEventName);
        if (itFb != m_config.transitions.end())
        {
            if (TryTransition(itFb->second)) return true;
        }
    }

    // 3. Match against triggerEvent field
    for (const auto& [key, t] : m_config.transitions)
    {
        if (t.triggerEvent == eventName || (!fallbackEventName.empty() && t.triggerEvent == fallbackEventName))
        {
            if (TryTransition(t)) return true;
        }
    }

    return false;
}

std::unique_ptr<HEIN::ICombatState> HEIN::CreateCombatStateFromConfig(const StateConfig& config)
{
    if (config.stateType == "OneHand" && !config.comboAnimationNames.empty())
    {
        return std::make_unique<HEIN::OneHandAttackState>(config);
    }
    return std::make_unique<HEIN::UniversalCombatState>(config);
}