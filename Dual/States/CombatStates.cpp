#include "pch.h" 
#include "States/CombatStates.h"
#include "Components/CombatStateMachineComponent.h"
#include <BlackBoard/CombatBlackBoard.h>
#include "../../../External/Engine/Components/SkinnedModelComponent.h"
#include "../../../External/Engine/Components/TransformComponent.h"
#include "../../../External/Engine/Components/HealthComponent.h"
#include <cmath>


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
        bool isBlockState = (m_config.isBlock || m_config.stateName == "Block" || m_config.stateType == "Block");
        bool isDodgeState = (m_config.lockMovementDirection || m_config.invincibilityEnd > 0.0f || m_config.stateName == "Dodge" || m_config.stateType == "Dodge");

        if (isBlockState)
        {
            blackboard->currentStance = CombatStance::Blocking;
        }
        else if (m_config.isAttack)
        {
            blackboard->currentStance = CombatStance::AttackRelese;
        }
        else if (isDodgeState)
        {
            blackboard->currentStance = CombatStance::Dodging;
            blackboard->dodgeCooldownTimer = blackboard->maxDodgeCooldown;
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

        if (m_config.lockMovementDirection || isDodgeState)
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
    bool isDodge = (m_config.lockMovementDirection || m_config.invincibilityEnd > 0.0f || m_config.stateName == "Dodge" || m_config.stateType == "Dodge");

    if (blackboard)
    {
        blackboard->currentSpeed = m_config.moveSpeed;
        blackboard->currentTurnSpeed = m_config.turnSpeed;

        if (m_config.lockMovementDirection || isDodge)
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
        health->SetGameplayInvincible(isInvincible);
    }

    bool isBlockState = (m_config.isBlock || m_config.stateName == "Block" || m_config.stateType == "Block");
    if (isBlockState && blackboard)
    {
        if (blackboard->currentBlockStamina <= 0.0f || blackboard->isBlockBroken)
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
    // Dodge states must NOT be movement-cancelled; transitions with hasExitTime must wait for exit time
    if (!m_config.isLooping && !isDodge && blackboard && blackboard->moveIntent.LengthSquared() > 0.1f)
    {
        auto itMove = m_config.transitions.find("OnMove");
        if (itMove != m_config.transitions.end())
        {
            const auto& t = itMove->second;
            if (!t.hasExitTime && (t.canInterrupt || (m_timer >= t.windowStart && m_timer <= t.windowEnd)))
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
        if (blackboard && (blackboard->localMoveIntent.LengthSquared() > 0.1f || (!isDodge && blackboard->moveIntent.LengthSquared() > 0.1f)))
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
