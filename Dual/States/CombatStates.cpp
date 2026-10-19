#include "pch.h" 
#include "States/CombatStates.h"
#include "Components/ShockwaveEmitterComponent.h"
#include "Components/CombatStateMachineComponent.h"
#include <BlackBoard/CombatBlackBoard.h>
#include "../../../External/Engine/Components/SkinnedModelComponent.h"
#include "../../../External/Engine/Components/TransformComponent.h"
#include "../../../External/Engine/Components/HealthComponent.h"
#include "../../../External/Engine/Message/Messenger.h"
#include <cmath>


// ==============================================================================
// UNIVERSAL DATA-DRIVEN COMBAT STATE
// ==============================================================================
HEIN::UniversalCombatState::UniversalCombatState(const StateConfig* config)
    : m_config(config)
{
}

HEIN::UniversalCombatState::UniversalCombatState(const StateConfig& config)
    : m_config(&config)
{
}

void HEIN::UniversalCombatState::OnEnter(Actor* owner, CombatStateMachineComponent* /*stateMachine*/, float blendDuration)
{
    if (!m_config) return;

    m_timer = 0.0f;
    m_windupHoldTimer = 0.0f;
    m_hasTriggeredWindupPause = false;
    m_isInWindupPause = false;
    m_isWeaponHitboxActive = false;
    m_hasSpawnedShockwave = false;
    m_hasQueuedTransition = false;
    m_stopDebounceTimer = 0.0f;

    if (owner)
    {
        owner->SetLocalTimeScale(1.0f);
        if (m_config->isAttack)
        {
            // One Attack = One Time Damage: clear the weapon's hit registry at the start of every new swing
            Messenger::GetInstance()->Notify(owner->GetID(), Message::RESET_WEAPON_SWING);
        }
    }

    auto* blackboard = owner->GetComponent<CombatBlackBoard>();
    auto* playerBB = owner->GetComponent<PlayerBlackBoard>();
    auto* bossBB = owner->GetComponent<BossBlackBoard>();
    if (blackboard)
    {
        // Reset active parry timer and anti-combo consecutive hits counter
        if (playerBB)
        {
            playerBB->blockActiveTimer = 0.0f;
            if (m_config->isAttack && !m_config->isGuardCounter && !m_config->isBlock)
            {
                playerBB->deflectStacks = 0;
            }
        }
        if (m_config->isAttack || m_config->stateName == "Idle" || m_config->moveSpeed <= 0.001f)
        {
            blackboard->consecutiveHitsTaken = 0;
        }

        for (const auto& effect : m_config->effects)
        {
            effect.hasTriggered = false;
        }

        bool isBlockState = m_config->IsBlockState();
        bool isDodgeState = m_config->IsDodgeState();
        bool isJumpState = m_config->IsJumpState();
        bool isStunState = m_config->IsStunState();

        blackboard->isAttacking = m_config->isAttack;
        blackboard->isJumping = isJumpState;

        if (isBlockState)
        {
            blackboard->currentStance = CombatStance::Blocking;
            if (bossBB) bossBB->comboIndex = 0;
        }
        else if (isStunState)
        {
            blackboard->currentStance = CombatStance::Staggered;
            if (bossBB) bossBB->comboIndex = 0;
        }
        else if (m_config->isAttack)
        {
            blackboard->currentStance = (m_config->windupPauseDuration > 0.0f)
                ? CombatStance::AttackWindup
                : CombatStance::AttackRelese;
            blackboard->lastAttackHit = false;

            // Data-driven stamina consumption for Player (Boss never consumes stamina)
            if (playerBB)
            {
                float cost = (m_config->staminaCost > 0.0f)
                    ? m_config->staminaCost
                    : ((m_config->stateTypeEnum == CombatStateType::ComboFinisher)
                        ? playerBB->comboFinisherStaminaCost
                        : playerBB->lightAttackStaminaCost);
                playerBB->currentBlockStamina -= cost;
                if (playerBB->currentBlockStamina < -20.0f) playerBB->currentBlockStamina = -20.0f;
                playerBB->staminaRegenDelayTimer = (playerBB->currentBlockStamina <= 0.0f) ? playerBB->exhaustedRegenDelay : playerBB->attackRegenDelay;
            }

            // Sync Boss 3-hit permutation index (123, 132, 213, 231, 312, 321)
            if (bossBB && (m_config->comboPermutationStep >= 0 || m_config->stateName.find("Combo") == 0))
            {
                if (bossBB->activeComboSequence.empty())
                {
                    bossBB->RollNextBossComboPattern();
                }
                if (m_config->comboPermutationStep >= 0)
                {
                    bossBB->comboIndex = m_config->comboPermutationStep;
                }
                else
                {
                    for (size_t i = 0; i < bossBB->activeComboSequence.size(); ++i)
                    {
                        if (bossBB->activeComboSequence[i] == m_config->stateName)
                        {
                            bossBB->comboIndex = static_cast<int>(i);
                            break;
                        }
                    }
                }
            }
        }
        else if (isJumpState)
        {
            blackboard->currentStance = CombatStance::Jumping;
            if (bossBB) bossBB->comboIndex = 0;
        }
        else if (isDodgeState)
        {
            blackboard->currentStance = CombatStance::Dodging;
            blackboard->dodgeCooldownTimer = blackboard->maxDodgeCooldown;
            if (bossBB) bossBB->comboIndex = 0;

            if (playerBB)
            {
                playerBB->currentBlockStamina -= playerBB->dodgeStaminaCost;
                if (playerBB->currentBlockStamina < -30.0f) playerBB->currentBlockStamina = -30.0f;
                playerBB->staminaRegenDelayTimer = (playerBB->currentBlockStamina <= 0.0f) ? playerBB->exhaustedRegenDelay : playerBB->dodgeRegenDelay;
            }
        }
        else if (m_config->stateName == "Strafe")
        {
            blackboard->currentStance = CombatStance::Strafing;
            if (bossBB) bossBB->comboIndex = 0;
        }
        else if (m_config->moveSpeed > 0.0f)
        {
            blackboard->currentStance = CombatStance::Walking;
            if (bossBB) bossBB->comboIndex = 0;
        }
        else
        {
            blackboard->currentStance = CombatStance::Idle;
            if (bossBB) bossBB->comboIndex = 0;
        }

        blackboard->currentSpeed = m_config->moveSpeed;
        blackboard->currentTurnSpeed = m_config->turnSpeed;

        if (m_config->lockMovementDirection || isDodgeState)
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

    m_actualDuration = m_config->stateDuration;
    auto* model = owner->GetComponent<SkinnedModelComponent>();
    if (m_actualDuration <= 0.0f && model != nullptr && !m_config->animationName.empty())
    {
        float animDur = model->GetAnimationDuration(m_config->animationName);
        m_actualDuration = (animDur > 0.01f) ? animDur : 1.0f;
    }
    if (m_actualDuration <= 0.0f) m_actualDuration = 1.0f;

    if (model != nullptr && !m_config->animationName.empty())
    {
        model->CrossfadeAnimation(m_config->animationName, blendDuration, !m_config->isLooping);
    }
}

void HEIN::UniversalCombatState::Update(Actor* owner, CombatStateMachineComponent* stateMachine, float deltaTime)
{
    if (!m_config) return;

    // During Hitstop (Hit Freeze), hold the state timeline and wind-up pause timer completely still
    if (owner && owner->IsHitstopped())
    {
        return;
    }

    m_timer += deltaTime;

    auto* blackboard = owner->GetComponent<CombatBlackBoard>();
    auto* playerBB = owner->GetComponent<PlayerBlackBoard>();
    auto* bossBB = owner->GetComponent<BossBlackBoard>();
    bool isDodge = (m_config->lockMovementDirection || m_config->invincibilityEnd > 0.0f || m_config->stateName == "Dodge" || m_config->stateType == "Dodge");

    // Souls-like Anticipation Frame Interpolation Pause & Active Hitbox Window
    if (m_config->isAttack && owner != nullptr)
    {
        if (m_config->windupPauseDuration > 0.0f)
        {
            if (!m_hasTriggeredWindupPause && m_timer >= m_config->windupPauseStart)
            {
                m_hasTriggeredWindupPause = true;
                m_isInWindupPause = true;
                m_windupHoldTimer = 0.0f;
                owner->SetLocalTimeScale(m_config->windupTimeScale);
                if (blackboard)
                {
                    blackboard->currentStance = CombatStance::AttackWindup;
                }
            }

            if (m_isInWindupPause)
            {
                float rawDt = (owner->GetUnscaledDeltaTime() > 0.0f) ? owner->GetUnscaledDeltaTime() : 0.016f;
                m_windupHoldTimer += rawDt;
                owner->SetLocalTimeScale(m_config->windupTimeScale);

                if (m_windupHoldTimer >= m_config->windupPauseDuration)
                {
                    m_isInWindupPause = false;
                    float releaseScale = (m_config->strikeSpeedMultiplier > 0.05f) ? m_config->strikeSpeedMultiplier : 1.0f;
                    owner->SetLocalTimeScale(releaseScale);
                    if (blackboard)
                    {
                        blackboard->currentStance = CombatStance::AttackRelese;
                    }
                }
            }
            else if (m_hasTriggeredWindupPause && m_timer >= m_actualDuration * 0.78f)
            {
                owner->SetLocalTimeScale(1.0f);
                if (blackboard)
                {
                    blackboard->currentStance = CombatStance::AttackRecovery;
                }
            }
        }

        // Active Weapon Hitbox Window: off during wind-up & anticipation pause, active during strike release
        float activeStart = (m_config->hitboxStart > 0.0f)
            ? m_config->hitboxStart
            : ((m_config->windupPauseDuration > 0.0f) ? m_config->windupPauseStart : 0.0f);
        float activeEnd = (m_config->hitboxEnd > 0.0f)
            ? m_config->hitboxEnd
            : (m_actualDuration * 0.85f);

        m_isWeaponHitboxActive = (!m_isInWindupPause) &&
                                 (m_config->windupPauseDuration <= 0.0f || m_hasTriggeredWindupPause) &&
                                 (m_timer >= activeStart && m_timer <= activeEnd);

        // Execute modular state action effects (Shockwaves, Camera Shakes, VFX, Sounds)
        for (const auto& effect : m_config->effects)
        {
            if (m_timer >= effect.triggerTime && !effect.hasTriggered)
            {
                effect.hasTriggered = true;
                if (effect.type == StateEffectType::Shockwave)
                {
                    if (auto* emitter = owner->GetComponent<ShockwaveEmitterComponent>())
                    {
                        emitter->EmitShockwaveFromOffset(
                            DirectX::SimpleMath::Vector3(0.0f, 0.0f, effect.forwardOffset),
                            effect.radius,
                            effect.duration,
                            effect.damage,
                            effect.knockbackForce,
                            effect.isGroundOnly
                        );
                    }
                }
            }
        }

        // Backward compatibility fallback for single shockwave property if effects array is empty
        if (m_config->effects.empty() && m_config->emitShockwave && m_timer >= activeStart && !m_hasSpawnedShockwave)
        {
            m_hasSpawnedShockwave = true;
            if (auto* emitter = owner->GetComponent<ShockwaveEmitterComponent>())
            {
                emitter->EmitShockwaveFromOffset(
                    DirectX::SimpleMath::Vector3(0.0f, 0.0f, m_config->shockwaveForwardOffset),
                    m_config->shockwaveRadius,
                    m_config->shockwaveDuration,
                    m_config->shockwaveDamage,
                    m_config->shockwaveKnockback,
                    m_config->shockwaveIsGroundOnly
                );
            }
        }
    }
    else
    {
        m_isWeaponHitboxActive = false;
    }

    if (blackboard)
    {
        blackboard->currentSpeed = m_config->moveSpeed;
        blackboard->currentTurnSpeed = m_config->turnSpeed;

        if (m_config->lockMovementDirection || isDodge)
        {
            blackboard->moveIntent = m_lockedDirection;
        }
    }

    auto* health = owner->GetComponent<HealthComponent>();
    if (health)
    {
        bool isInvincible = (m_config->invincibilityEnd > 0.0f &&
                             m_timer >= m_config->invincibilityStart &&
                             m_timer <= m_config->invincibilityEnd);
        health->SetGameplayInvincible(isInvincible);
    }

    bool isBlockState = m_config->IsBlockState();
    if (isBlockState && playerBB)
    {
        playerBB->blockActiveTimer += deltaTime;

        if (playerBB->isBlockBroken)
        {
            auto itStop = m_config->transitions.find("OnStop");
            if (itStop != m_config->transitions.end())
            {
                stateMachine->ChangeState(itStop->second.targetState, itStop->second.blendDuration);
                return;
            }
        }
    }

    // Boss 6-Permutation 3-Hit Combo Continuation:
    // Only carry on to the 2nd and 3rd combo swings when the current attack lands (lastAttackHit == true)!
    // If the attack misses, let this swing finish and return to Idle so the Boss immediately chases the player.
    if (bossBB && bossBB->lastAttackHit && m_config->isAttack && m_config->stateName.find("Combo") == 0 && !m_isInWindupPause)
    {
        if (bossBB->comboIndex + 1 < static_cast<int>(bossBB->activeComboSequence.size()))
        {
            float comboChainTime = m_actualDuration * 0.78f;
            if (m_timer >= comboChainTime)
            {
                stateMachine->ChangeState(bossBB->activeComboSequence[bossBB->comboIndex + 1], 0.18f);
                m_hasQueuedTransition = false;
                return;
            }
        }
    }

    // Queued transition from input buffering
    if (m_hasQueuedTransition)
    {
        bool hitValid = (!m_queuedTransition.requireHit) || (blackboard && blackboard->lastAttackHit);
        float triggerTime = m_queuedTransition.hasExitTime ? m_actualDuration : m_queuedTransition.windowStart;
        if (hitValid && (m_queuedTransition.canInterrupt || m_timer >= triggerTime))
        {
            stateMachine->ChangeState(m_queuedTransition.targetState, m_queuedTransition.blendDuration);
            m_hasQueuedTransition = false;
            return;
        }
    }

    // Movement cancellation for non-looping states (e.g. holding W cancels attack recovery window)
    // Dodge and Stun states must NOT be movement-cancelled; transitions with hasExitTime must wait for exit time
    bool isStun = m_config->IsStunState();
    if (!m_config->isLooping && !isDodge && !isStun && blackboard && blackboard->moveIntent.LengthSquared() > 0.1f)
    {
        auto itMove = m_config->transitions.find("OnMove");
        if (itMove != m_config->transitions.end())
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
    if (!m_config->isLooping && m_timer >= m_actualDuration)
    {
        // Boss 3-hit permutation fallback at clip end ONLY if the current attack landed (lastAttackHit == true)
        if (bossBB && bossBB->lastAttackHit && m_config->stateName.find("Combo") == 0 && !bossBB->activeComboSequence.empty())
        {
            if (bossBB->comboIndex + 1 < static_cast<int>(bossBB->activeComboSequence.size()))
            {
                stateMachine->ChangeState(bossBB->activeComboSequence[bossBB->comboIndex + 1], 0.18f);
                return;
            }
        }

        // Check if a combo hit-confirmed transition should trigger
        bool hitConfirmed = (blackboard != nullptr && blackboard->lastAttackHit);
        if (hitConfirmed)
        {
            for (const auto& [key, t] : m_config->transitions)
            {
                if (t.requireHit && (t.hasExitTime || key == "OnNextCombo" || t.triggerEvent == "OnNextCombo"))
                {
                    if (bossBB && m_config->stateName.find("Combo") == 0)
                    {
                        continue; // Handled by 3-hit permutation logic above
                    }
                    stateMachine->ChangeState(t.targetState, t.blendDuration);
                    return;
                }
            }
        }

        // If moving when animation concludes, prefer OnMove over Idle
        if (blackboard && (blackboard->localMoveIntent.LengthSquared() > 0.1f || (!isDodge && blackboard->moveIntent.LengthSquared() > 0.1f)))
        {
            auto itMove = m_config->transitions.find("OnMove");
            if (itMove != m_config->transitions.end() && !itMove->second.requireHit)
            {
                stateMachine->ChangeState(itMove->second.targetState, itMove->second.blendDuration);
                return;
            }
        }

        for (const auto& [key, t] : m_config->transitions)
        {
            bool isAutoExit = t.triggerEvent.empty() || key == "Exit" || key == "OnFinished" || key == "OnStop";
            if (!t.requireHit && isAutoExit && (t.hasExitTime || key == "OnFinished" || key == "OnStop" || key == "Exit"))
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
    if (m_config->isLooping && blackboard)
    {
        if (m_config->moveSpeed <= 0.001f) // Idle-like
        {
            if (blackboard->moveIntent.LengthSquared() > 0.1f)
            {
                if (blackboard->isLockedOn && std::abs(blackboard->localMoveIntent.x) >= std::abs(blackboard->localMoveIntent.z))
                {
                    auto it = m_config->transitions.find("OnStrafe");
                    if (it != m_config->transitions.end())
                    {
                        stateMachine->ChangeState(it->second.targetState, it->second.blendDuration);
                        return;
                    }
                }
                auto it = m_config->transitions.find("OnMove");
                if (it != m_config->transitions.end())
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
                    auto it = m_config->transitions.find("OnStop");
                    if (it != m_config->transitions.end())
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
    m_isInWindupPause = false;
    m_isWeaponHitboxActive = false;
    m_hasSpawnedShockwave = false;

    if (owner)
    {
        owner->SetLocalTimeScale(1.0f);
    }

    auto* health = owner->GetComponent<HealthComponent>();
    if (health)
    {
        health->SetGameplayInvincible(false);
    }
    auto* blackboard = owner->GetComponent<CombatBlackBoard>();
    if (blackboard)
    {
        blackboard->isAttacking = false;
        blackboard->isJumping = false;
    }

    if (m_config && m_config->IsBlockState())
    {
        if (owner)
        {
            auto* playerBB = owner->GetComponent<PlayerBlackBoard>();
            if (playerBB)
            {
                playerBB->parryLockoutTimer = 0.35f;
            }
        }
    }
}

bool HEIN::UniversalCombatState::HandleMessage(Actor* owner, CombatStateMachineComponent* stateMachine, Message::MessageID messageID)
{
    if (!m_config) return false;

    std::string eventName = "";
    std::string fallbackEventName = "";

    auto* blackboard = owner->GetComponent<CombatBlackBoard>();
    auto* bossBB = owner->GetComponent<BossBlackBoard>();

    switch (messageID)
    {
    case Message::PLAYER_ACTION_ATTACK:
    {
        auto* playerBB = owner ? owner->GetComponent<PlayerBlackBoard>() : nullptr;
        if (playerBB && playerBB->guardCounterWindowTimer > 0.0f)
        {
            eventName = "OnGuardCounter";
            fallbackEventName = "OnAttack";
            playerBB->guardCounterWindowTimer = 0.0f;
        }
        else if (blackboard && blackboard->localMoveIntent.z > 0.5f)
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
            fallbackEventName = "OnNextCombo";
        }
        break;
    }

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

    case Message::ENEMY_ACTION_HEAVY_ATTACK:
        eventName = "OnHeavyAttack";
        fallbackEventName = "OnAttack";
        break;

    case Message::ATTACK_HIT_CONFIRMED:
        if (blackboard) blackboard->lastAttackHit = true;
        eventName = "OnHit";
        fallbackEventName = "OnNextCombo";
        break;

    case Message::HIT_STUN:
        eventName = "OnStun";
        fallbackEventName = "OnHit";
        break;

    case Message::PLAYER_ACTION_JUMP:
        eventName = "OnJump";
        break;

    default:
        return false;
    }

    auto TryTransition = [&](const TransitionData& rawT) -> bool
    {
        TransitionData t = rawT;

        // Dynamic Boss 6-Permutation Combo Routing (123, 132, 213, 231, 312, 321)
        if (bossBB && t.targetState.find("Combo") == 0)
        {
            if (!m_config->isAttack)
            {
                // Entering combo opener (Step 0) from Idle / Walk / Dodge
                if (bossBB->activeComboSequence.empty() || bossBB->comboIndex > 0)
                {
                    bossBB->RollNextBossComboPattern();
                }
                t.targetState = bossBB->activeComboSequence[0];
            }
            else if (m_config->stateName.find("Combo") == 0)
            {
                // Chaining from Step 0 -> Step 1 or Step 1 -> Step 2
                if (bossBB->activeComboSequence.empty())
                {
                    bossBB->RollNextBossComboPattern();
                }
                if (bossBB->comboIndex + 1 < static_cast<int>(bossBB->activeComboSequence.size()))
                {
                    t.targetState = bossBB->activeComboSequence[bossBB->comboIndex + 1];
                    t.canInterrupt = false;
                    t.requireHit = true;
                    t.windowStart = m_actualDuration * 0.75f;
                }
                else
                {
                    // Currently on the 3rd (final) hit of the permutation; do not chain to a 4th hit
                    return false;
                }
            }
        }

        if (t.requireHit && (!blackboard || !blackboard->lastAttackHit))
        {
            return false;
        }

        // If the transition has exit time and cannot interrupt early:
        // Do NOT cut the animation off mid-state! Buffer it so it fires when the state completes.
        float triggerTime = t.hasExitTime ? m_actualDuration : t.windowStart;
        if (!t.canInterrupt && m_timer < triggerTime)
        {
            m_queuedTransition = t;
            m_hasQueuedTransition = true;
            return true;
        }

        // Within valid combo window or allowed to interrupt immediately:
        if (t.canInterrupt || (m_timer >= triggerTime && m_timer <= t.windowEnd))
        {
            stateMachine->ChangeState(t.targetState, t.blendDuration);
            m_hasQueuedTransition = false;
            return true;
        }

        // If pressed before triggerTime, queue it to fire when the window opens:
        if (m_timer < triggerTime)
        {
            m_queuedTransition = t;
            m_hasQueuedTransition = true;
            return true;
        }
        return false;
    };

    // Direct key match in transitions map
    auto it = m_config->transitions.find(eventName);
    if (it != m_config->transitions.end())
    {
        if (TryTransition(it->second)) return true;
    }

    // Fallback key match (e.g. OnForwardAttack -> OnAttack, or OnAttack -> OnNextCombo)
    if (!fallbackEventName.empty())
    {
        auto itFb = m_config->transitions.find(fallbackEventName);
        if (itFb != m_config->transitions.end())
        {
            if (TryTransition(itFb->second)) return true;
        }
    }

    // Match against triggerEvent field
    for (const auto& [key, t] : m_config->transitions)
    {
        if (t.triggerEvent == eventName || (!fallbackEventName.empty() && t.triggerEvent == fallbackEventName))
        {
            if (TryTransition(t)) return true;
        }
    }

    return false;
}
