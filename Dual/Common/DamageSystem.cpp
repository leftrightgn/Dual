#include "pch.h"
#include "DamageSystem.h"
#include <Components/DamageDealerComponent.h>
#include <Components/HealthComponent.h>
#include <Components/ColliderComponent/ColliderComponent.h>
#include <Components/CombatStateMachineComponent.h>
#include <Entities/ActorManager.h>
#include <Entities/Actor.h>

void HEIN::DamageSystem::HandlTriggerHit(const HEIN::TriggerEventPayLoad& payLoad, HEIN::ActorManager& actorManager)
{
	HEIN::Actor* actorA = payLoad.triggerA->GetOwner();
	HEIN::Actor* actorB = payLoad.triggerB->GetOwner();

	if (actorA == nullptr || actorB == nullptr) return;

	HEIN::DamageDealerComponent* dealerA = actorA->GetComponent<HEIN::DamageDealerComponent>();
	HEIN::HealthComponent* victimB = actorB->GetComponent<HEIN::HealthComponent>();

	if (dealerA != nullptr && victimB != nullptr)
	{
		if (actorA->GetOwnerID() != actorB->GetID())
		{
			bool isAttacking = true;
			HEIN::CombatStateMachineComponent* fsmA = actorA->GetComponent<HEIN::CombatStateMachineComponent>();
			if (!fsmA && actorA->GetOwnerID() != HEIN::INVALID_ACTOR_ID)
			{
				HEIN::Actor* ownerA = actorManager.GetActor(actorA->GetOwnerID());
				if (ownerA) fsmA = ownerA->GetComponent<HEIN::CombatStateMachineComponent>();
			}
			if (fsmA) isAttacking = fsmA->IsAttacking();

			bool isBlocking = false;
			HEIN::CombatStateMachineComponent* fsmB = actorB->GetComponent<HEIN::CombatStateMachineComponent>();
			if (!fsmB && actorB->GetOwnerID() != HEIN::INVALID_ACTOR_ID)
			{
				HEIN::Actor* ownerB = actorManager.GetActor(actorB->GetOwnerID());
				if (ownerB) fsmB = ownerB->GetComponent<HEIN::CombatStateMachineComponent>();
			}
			if (fsmB) isBlocking = fsmB->IsBlocking();

			if (isAttacking && !isBlocking)
			{
				victimB->ApplyDamage(dealerA->GetDamageAmount());
			}
		}
	}

	HEIN::DamageDealerComponent* dealerB = actorB->GetComponent<HEIN::DamageDealerComponent>();
	HEIN::HealthComponent* victimA = actorA->GetComponent<HEIN::HealthComponent>();

	if (dealerB != nullptr && victimA != nullptr)
	{
		if (actorB->GetOwnerID() != actorA->GetID())
		{
			bool isAttacking = true;
			HEIN::CombatStateMachineComponent* fsmB = actorB->GetComponent<HEIN::CombatStateMachineComponent>();
			if (!fsmB && actorB->GetOwnerID() != HEIN::INVALID_ACTOR_ID)
			{
				HEIN::Actor* ownerB = actorManager.GetActor(actorB->GetOwnerID());
				if (ownerB) fsmB = ownerB->GetComponent<HEIN::CombatStateMachineComponent>();
			}
			if (fsmB) isAttacking = fsmB->IsAttacking();

			bool isBlocking = false;
			HEIN::CombatStateMachineComponent* fsmA = actorA->GetComponent<HEIN::CombatStateMachineComponent>();
			if (!fsmA && actorA->GetOwnerID() != HEIN::INVALID_ACTOR_ID)
			{
				HEIN::Actor* ownerA = actorManager.GetActor(actorA->GetOwnerID());
				if (ownerA) fsmA = ownerA->GetComponent<HEIN::CombatStateMachineComponent>();
			}
			if (fsmA) isBlocking = fsmA->IsBlocking();

			if (isAttacking && !isBlocking)
			{
				victimA->ApplyDamage(dealerB->GetDamageAmount());
			}
		}
	}
}
