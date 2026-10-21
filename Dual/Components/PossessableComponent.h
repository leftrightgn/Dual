#pragma once
#include "../../External/Engine/Components/IComponent.h"
#include "../../External/Engine/ImGui/imgui.h"

namespace HEIN
{
	class Actor;

	/**
	 * @class PossessableComponent
	 * @brief Identifies an entity body as a vessel that can be possessed by the player ghost.
	 */
	class PossessableComponent : public HEIN::IComponent
	{
	private:
		float m_possessRadius = 25.0f;
		bool m_isOccupied = false;

	public:
		PossessableComponent(Actor* owner);

		void Update(float /*deltaTime*/) override {}

		float GetPossessRadius() const { return m_possessRadius; }
		void SetPossessRadius(float radius) { m_possessRadius = radius; }

		bool IsOccupied() const { return m_isOccupied; }
		void SetOccupied(bool occupied) { m_isOccupied = occupied; }

		std::string GetComponentName() const override { return "PossessableComponent"; }

		nlohmann::json Serialize() override;
		

		void Deserialize(const nlohmann::json& data) override;
		
		void OnInspectorGUI(GameContext& gameContext) override;
		
	};
}
