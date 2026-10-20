#pragma once
#include <cstdint>

namespace GameConstants
{
    namespace Camera
    {
        constexpr float DefaultFovDegrees = 45.0f;
        constexpr float NearClipPlane = 0.01f;
        constexpr float FarClipPlane = 5000.0f;
        constexpr float TargetHeadVerticalOffset = 15.0f;
        constexpr float MaxValidHeadDistanceSq = 10000.0f; // Square distance threshold (100.0f * 100.0f)
    }

    namespace Shadow
    {
        constexpr uint32_t DefaultMapWidth = 2048;
        constexpr uint32_t DefaultMapHeight = 2048;
        constexpr float GlobalLightShadowRange = 500.0f;
    }

    namespace Ghost
    {
        constexpr float SpawnVerticalOffset = 3.5f;
        constexpr float DefaultPossessRadius = 25.0f;
        constexpr float PhysicsMass = 80.0f;
        constexpr float ColliderRadius = 3.0f;
        constexpr float ColliderHeight = 12.0f;
        constexpr float ColliderVerticalOffset = 90.0f;
    }

    namespace Combat
    {
        constexpr float DefaultKnockbackForce = 25.0f;
        constexpr float DefaultStaminaDamage = 40.0f;
        constexpr float HeavyAttackStaminaThreshold = 60.0f;
        constexpr float StunStateTransitionDuration = 0.05f;
        constexpr float GuardBreakThreshold = 0.0f;
        constexpr float GuardPercentDivisor = 100.0f;
        constexpr float HitFlashDuration = 0.15f;
    }

    namespace BossAI
    {
        // Distance Thresholds
        constexpr float CloseGuardBreakDistance = 30.0f;
        constexpr float HeavyAttackDistance = 32.0f;
        constexpr float ComboAttackDistance = 28.0f;
        constexpr float ChaseArrivalDistance = 25.0f;
        constexpr float StrafeDistance = 35.0f;
        constexpr float TetherMaxDistance = 500.0f;
        constexpr float HeavyLungeDistanceThreshold = 22.0f;
        constexpr float ComboStepDistanceThreshold = 20.0f;

        // Speeds
        constexpr float Phase1ChaseSpeed = 95.0f;
        constexpr float Phase2ChaseSpeed = 130.0f;
        constexpr float HeavyLungeSpeed = 35.0f;
        constexpr float ComboStepSpeed = 30.0f;
        constexpr float HeavyTurnSpeed = 5.0f;
        constexpr float ComboTurnSpeed = 8.0f;

        // Timers & Cooldowns
        constexpr float Phase2HealthThreshold = 0.50f;
        constexpr float ReactiveDodgeCooldown = 1.25f;
        constexpr float DodgeActionDuration = 1.20f;
        constexpr float P1GuardBreakCooldown = 0.80f;
        constexpr float P2GuardBreakCooldown = 0.50f;
        constexpr float P2HeavyCooldown = 2.00f;
        constexpr float P2ComboCooldown = 0.40f;
        constexpr float HeavyAttackDuration = 3.20f;
        constexpr float ComboAttackDuration = 6.00f;
        constexpr float EarlySwingMissTimeout = 1.25f;
        constexpr float AttackStateMinCheckTime = 0.40f;
        constexpr float HeavyTrackRatio = 0.60f;
        constexpr float ComboTrackTime = 0.45f;
    }

    namespace Weapons
    {
        constexpr float DefaultSwordDamage = 5.0f;
        constexpr float DefaultSwordKnockback = 15.0f;
        constexpr float BossAxeDamage = 15.0f;
        constexpr float BossAxeKnockback = 35.0f;
        constexpr float DefaultSwordScale = 0.02f;
    }

    namespace Shockwave
    {
        constexpr int GroundRingSegments = 36;
        constexpr int LongitudeArchCount = 8;
        constexpr float DomeHeightQuarter = 0.25f;
        constexpr float DomeHeightHalf = 0.50f;
        constexpr float DomeHeightThreeQuarters = 0.75f;
    }
}
