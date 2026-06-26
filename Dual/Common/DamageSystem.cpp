#include "pch.h"
#include "DamageSystem.h"
#include <Components/DamageDealerComponent.h>
#include <Components/HealthComponent.h>
#include <Components/ColliderComponent/ColliderComponent.h>
#include <Entities/Actor.h>

void HEIN::DamageSystem::HandlTriggerHit(const HEIN::TriggerEventPayLoad& payLoad)
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
			victimB->ApplyDamage(dealerA->GetDamageAmount());
		}
	}

	HEIN::DamageDealerComponent* dealerB = actorB->GetComponent<HEIN::DamageDealerComponent>();
	HEIN::HealthComponent* victimA = actorA->GetComponent<HEIN::HealthComponent>();

	if (dealerB != nullptr && victimA != nullptr)
	{
		if (actorB->GetOwnerID() != actorA->GetID())
		{
			victimA->ApplyDamage(dealerB->GetDamageAmount());
		}
	}
}
