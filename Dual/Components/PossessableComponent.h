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
		PossessableComponent(Actor* owner)
			: HEIN::IComponent(owner)
		{
		}

		void Update(float /*deltaTime*/) override {}

		float GetPossessRadius() const { return m_possessRadius; }
		void SetPossessRadius(float radius) { m_possessRadius = radius; }

		bool IsOccupied() const { return m_isOccupied; }
		void SetOccupied(bool occupied) { m_isOccupied = occupied; }

		std::string GetComponentName() const override { return "PossessableComponent"; }

		nlohmann::json Serialize() override
		{
			nlohmann::json data = IComponent::Serialize();
			data["Radius"] = m_possessRadius;
			data["IsOccupied"] = m_isOccupied;
			return data;
		}

		void Deserialize(const nlohmann::json& data) override
		{
			IComponent::Deserialize(data);
			if (data.contains("Radius")) m_possessRadius = data["Radius"];
			if (data.contains("IsOccupied")) m_isOccupied = data["IsOccupied"];
		}

		void OnInspectorGUI(GameContext& /*gameContext*/) override
		{
			if (ImGui::CollapsingHeader("PossessableComponent", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::DragFloat("Possess Radius", &m_possessRadius, 0.5f, 1.0f, 100.0f);
				ImGui::Checkbox("Is Occupied", &m_isOccupied);
			}
		}
	};
}
