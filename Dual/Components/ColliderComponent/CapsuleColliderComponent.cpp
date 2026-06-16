#include "pch.h"
#include "CapsuleColliderComponent.h"
#include <DirectXColors.h>

HEIN::CapsuleColliderComponent::CapsuleColliderComponent(Actor* owner)
	: ColliderComponent(owner, ColliderShape::Capsule)
	, m_radius(0.5f)
	, m_height(1.0f)
{
}

void HEIN::CapsuleColliderComponent::Initialize(float radius, float height)
{
	m_radius = radius;
	m_height = height;
}

void HEIN::CapsuleColliderComponent::Update(float deltaTime)
{
}

void HEIN::CapsuleColliderComponent::Draw(GameContext& gameContext, const DirectX::SimpleMath::Matrix& world, const DirectX::SimpleMath::Matrix& view, const DirectX::SimpleMath::Matrix& proj)
{
	if (gameContext.debugCollisionRenderer == nullptr) return;

	DirectX::SimpleMath::Matrix worldMatrix = CalculateWorldMatrix();
	
	// Get Center of the capsule
	DirectX::SimpleMath::Vector3 center = worldMatrix.Translation();

	// Get the Up Direction Row2 of the Matrix
	DirectX::SimpleMath::Vector3 upDir(worldMatrix._21, worldMatrix._22, worldMatrix._23);
	upDir.Normalize();

	// Calculate the Point A & B
	// Move up by half of the Height
	DirectX::SimpleMath::Vector3 topSphereCenter = center + (upDir * (m_height * 0.5f));
	
	// Move down by half of the Height
	DirectX::SimpleMath::Vector3 bottomSphereCenter = center - (upDir * (m_height * 0.5f));

	DirectX::SimpleMath::Color debugColor = DirectX::SimpleMath::Color(DirectX::Colors::Red);
	if (m_isCollidingThisFrame)
	{
		debugColor = DirectX::Colors::Yellow;
	}
	// Get the Right Direction Row1 of the Matrix
	DirectX::SimpleMath::Vector3 rightDir(worldMatrix._11, worldMatrix._12, worldMatrix._13);
	rightDir.Normalize();

	// Get the Forward Direction Row3 of the Matrix
	DirectX::SimpleMath::Vector3 forwardDir(worldMatrix._31, worldMatrix._32, worldMatrix._33);
	forwardDir.Normalize();
	
	gameContext.debugCollisionRenderer->QueueLine(topSphereCenter + (rightDir * m_radius), bottomSphereCenter + (rightDir * m_radius), debugColor);
	gameContext.debugCollisionRenderer->QueueLine(topSphereCenter - (rightDir * m_radius), bottomSphereCenter - (rightDir * m_radius), debugColor);
	gameContext.debugCollisionRenderer->QueueLine(topSphereCenter + (forwardDir * m_radius), bottomSphereCenter + (forwardDir * m_radius), debugColor);
	gameContext.debugCollisionRenderer->QueueLine(topSphereCenter - (forwardDir * m_radius), bottomSphereCenter - (forwardDir * m_radius), debugColor);

	// DRAW THE DOMES AND RINGS USING SINE WAVES ---
	const int segments = 16;

	// Starting points for the arcs
	DirectX::SimpleMath::Vector3 prevTopRight = topSphereCenter + (rightDir * m_radius);
	DirectX::SimpleMath::Vector3 prevTopForward = topSphereCenter + (forwardDir * m_radius);

	DirectX::SimpleMath::Vector3 prevBottomRight = bottomSphereCenter + (rightDir * m_radius);
	DirectX::SimpleMath::Vector3 prevBottomForward = bottomSphereCenter + (forwardDir * m_radius);

	DirectX::SimpleMath::Vector3 prevTopRing = topSphereCenter + (rightDir * m_radius);
	DirectX::SimpleMath::Vector3 prevBottomRing = bottomSphereCenter + (rightDir * m_radius);

	for (int i = 1; i <= segments; i++)
	{
		// Calculate the angles 
		float arcAngle = (DirectX::XM_PI) * ((float)i / segments); // 0 to 180 degrees (for the over-the-top arcs)
		float ringAngle = (DirectX::XM_2PI) * ((float)i / segments); // 0 to 360 degrees (for the flat rings)

		float cosArc = cos(arcAngle);
		float sinArc = sin(arcAngle);

		float cosRing = cos(ringAngle);
		float sinRing = sin(ringAngle);

		// --- TOP DOME (Uses +upDir) ---
		DirectX::SimpleMath::Vector3 nextTopRight = topSphereCenter + (rightDir * cosArc * m_radius) + (upDir * sinArc * m_radius);
		gameContext.debugCollisionRenderer->QueueLine(prevTopRight, nextTopRight, debugColor);
		prevTopRight = nextTopRight;

		DirectX::SimpleMath::Vector3 nextTopForward = topSphereCenter + (forwardDir * cosArc * m_radius) + (upDir * sinArc * m_radius);
		gameContext.debugCollisionRenderer->QueueLine(prevTopForward, nextTopForward, debugColor);
		prevTopForward = nextTopForward;

		// --- BOTTOM BOWL (Uses -upDir) ---
		DirectX::SimpleMath::Vector3 nextBottomRight = bottomSphereCenter + (rightDir * cosArc * m_radius) - (upDir * sinArc * m_radius);
		gameContext.debugCollisionRenderer->QueueLine(prevBottomRight, nextBottomRight, debugColor);
		prevBottomRight = nextBottomRight;

		DirectX::SimpleMath::Vector3 nextBottomForward = bottomSphereCenter + (forwardDir * cosArc * m_radius) - (upDir * sinArc * m_radius);
		gameContext.debugCollisionRenderer->QueueLine(prevBottomForward, nextBottomForward, debugColor);
		prevBottomForward = nextBottomForward;

		// --- HORIZONTAL RINGS (The "Seams" connecting the domes to the cylinder) ---
		DirectX::SimpleMath::Vector3 nextTopRing = topSphereCenter + (rightDir * cosRing * m_radius) + (forwardDir * sinRing * m_radius);
		gameContext.debugCollisionRenderer->QueueLine(prevTopRing, nextTopRing, debugColor);
		prevTopRing = nextTopRing;

		DirectX::SimpleMath::Vector3 nextBottomRing = bottomSphereCenter + (rightDir * cosRing * m_radius) + (forwardDir * sinRing * m_radius);
		gameContext.debugCollisionRenderer->QueueLine(prevBottomRing, nextBottomRing, debugColor);
		prevBottomRing = nextBottomRing;

		float ringSpacing = 2.0f;

		int numExtraRings = static_cast<int>(m_height / ringSpacing);

		if (numExtraRings > 0)
		{
			// Calculate the exact distance between rings
			float stepSize = m_height / (numExtraRings + 1);

			for (int r = 1; r <= numExtraRings; r++)
			{
				DirectX::SimpleMath::Vector3 ringCenter =
					bottomSphereCenter + (upDir * (stepSize * r));

				DirectX::SimpleMath::Vector3 prevRingPoint =
					ringCenter + (rightDir * m_radius);

				for (int i = 1; i <= segments; i++)
				{
					float ringAngle = (DirectX::XM_2PI) * ((float)i / segments);

					float cosRing = cos(ringAngle);
					float sinRing = sin(ringAngle);

					DirectX::SimpleMath::Vector3 nextRingPoint = ringCenter +
						(rightDir * cosRing * m_radius) +
						(forwardDir * sinRing * m_radius);

					gameContext.debugCollisionRenderer->QueueLine(prevRingPoint, nextRingPoint, debugColor);
					prevRingPoint = nextRingPoint;
				}
			}
		}
	}
}
