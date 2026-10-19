#pragma once
#include "../../External/Engine/Components/IComponent.h"
#include "../../External/Engine/Entities/Actor.h"
#include "../../External/Engine/Framework/GameContext.h"
#include <SimpleMath.h>
#include <DirectXColors.h>
#include <vector>
#include <unordered_set>
#include <string>

namespace HEIN
{
	class ActorManager;

	/**
	 * @brief Target filtering strategy for shockwave impacts.
	 */
	enum class ShockwaveTargetFilter
	{
		OpponentsOnly,  ///< Targets the opposing faction (Boss targets Player, Player targets Enemy, Neutral targets both)
		PlayerOnly,     ///< Targets only ActorType::Player
		EnemyOnly,      ///< Targets only ActorType::Enemy
		AllCharacters   ///< Targets both Player and Enemy (e.g. explosive barrels, stage hazards)
	};

	/**
	 * @brief Instance data for an active expanding ground or aerial shockwave.
	 */
	struct ShockwaveInstance
	{
		DirectX::SimpleMath::Vector3 center = DirectX::SimpleMath::Vector3::Zero;
		float currentRadius = 1.0f;
		float maxRadius = 24.0f;
		float lifetime = 0.0f;
		float maxLifetime = 1.1f;
		float damage = 45.0f;
		float knockbackForce = 35.0f;
		float waveThickness = 3.5f;       ///< Width of active expanding wave-front band (hollow ring)
		bool isGroundOnly = true;         ///< True: Elden Ring ground wave (jump evades, grounded block = guard break)
		ShockwaveTargetFilter targetFilter = ShockwaveTargetFilter::OpponentsOnly;
		DirectX::XMVECTORF32 primaryColor = DirectX::Colors::Gold;
		DirectX::XMVECTORF32 secondaryColor = DirectX::Colors::OrangeRed;

		std::unordered_set<ActorID> hitActors; ///< Ensures each target is hit/evaded at most once per shockwave
		bool isFinished = false;
	};

	/**
	 * @class ShockwaveEmitterComponent
	 * @brief Generic, reusable ECS component that allows any Actor (Boss, Player, Barrel, Trap)
	 *        to emit expanding ground shockwaves or explosive blastwaves.
	 * 
	 * DESIGN PHILOSOPHY:
	 * - Fully reusable across all entity types without bespoke hardcoded classes.
	 * - Elden Ring Jump Evade: Ground shockwaves travel along the terrain as expanding hollow rings.
	 *    Airborne/jumping players evade cleanly with zero damage.
	 * - Option A Instant Guard Break: Ground shockwaves cannot be blocked with a shield—grounded
	 *    blocking immediately breaks guard, exhausts stamina, and triggers hit stun.
	 * - Self-contained ECS rendering: Visualizes expanding wireframe dome & rings in GameScene pass.
	 */
	class ShockwaveEmitterComponent : public IComponent
	{
	private:
		ActorManager* m_actorManager = nullptr;
		std::vector<ShockwaveInstance> m_shockwaves;
		ShockwaveTargetFilter m_defaultFilter = ShockwaveTargetFilter::OpponentsOnly;
		bool m_drawDebugWireframe = false;

	public:
		ShockwaveEmitterComponent(Actor* owner, ActorManager* actorManager = nullptr);
		virtual ~ShockwaveEmitterComponent() = default;

		void SetActorManager(ActorManager* actorManager) { m_actorManager = actorManager; }
		ActorManager* GetActorManager() const { return m_actorManager; }

		void SetDefaultFilter(ShockwaveTargetFilter filter) { m_defaultFilter = filter; }
		ShockwaveTargetFilter GetDefaultFilter() const { return m_defaultFilter; }

		bool IsDebugWireframeEnabled() const { return m_drawDebugWireframe; }
		void SetDebugWireframeEnabled(bool enabled) { m_drawDebugWireframe = enabled; }

		/**
		 * @brief Emits a shockwave at an explicit world position.
		 */
		void EmitShockwave(
			const DirectX::SimpleMath::Vector3& worldCenter,
			float maxRadius = 24.0f,
			float duration = 1.1f,
			float damage = 45.0f,
			float knockbackForce = 35.0f,
			bool isGroundOnly = true,
			ShockwaveTargetFilter filter = ShockwaveTargetFilter::OpponentsOnly,
			DirectX::XMVECTORF32 primaryColor = DirectX::Colors::Gold,
			DirectX::XMVECTORF32 secondaryColor = DirectX::Colors::OrangeRed
		);

		/**
		 * @brief Emits a shockwave at an offset relative to the owner actor's orientation.
		 * Forward is +Z, Right is +X, Up is +Y.
		 */
		void EmitShockwaveFromOffset(
			const DirectX::SimpleMath::Vector3& localOffset,
			float maxRadius = 24.0f,
			float duration = 1.1f,
			float damage = 45.0f,
			float knockbackForce = 35.0f,
			bool isGroundOnly = true,
			ShockwaveTargetFilter filter = ShockwaveTargetFilter::OpponentsOnly,
			DirectX::XMVECTORF32 primaryColor = DirectX::Colors::Gold,
			DirectX::XMVECTORF32 secondaryColor = DirectX::Colors::OrangeRed
		);

		void Clear();
		bool HasActiveShockwaves() const { return !m_shockwaves.empty(); }
		const std::vector<ShockwaveInstance>& GetActiveShockwaves() const { return m_shockwaves; }

		void Update(float deltaTime) override;

		void Draw(
			GameContext& gameContext,
			const DirectX::SimpleMath::Matrix& world,
			const DirectX::SimpleMath::Matrix& view,
			const DirectX::SimpleMath::Matrix& proj
		) override;

		std::string GetComponentName() const override { return "ShockwaveEmitterComponent"; }

		void OnInspectorGUI(GameContext& gameContext) override;

	private:
		bool IsTargetValid(Actor* target, ShockwaveTargetFilter filter) const;
		void EvaluateShockwaveHit(ShockwaveInstance& wave, Actor* target, float deltaTime);
		void DrawShockwaveWireframe(GameContext& gameContext, const ShockwaveInstance& wave);
	};
}
