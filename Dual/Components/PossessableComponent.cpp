#include "pch.h"
#include "PossessableComponent.h"

HEIN::PossessableComponent::PossessableComponent(Actor* owner)
	: HEIN::IComponent(owner)
{
}

nlohmann::json HEIN::PossessableComponent::Serialize()
{
	nlohmann::json data = IComponent::Serialize();
	data["Radius"] = m_possessRadius;
	data["IsOccupied"] = m_isOccupied;
	return data;
}

void HEIN::PossessableComponent::Deserialize(const nlohmann::json& data)
{
	IComponent::Deserialize(data);
	if (data.contains("Radius")) m_possessRadius = data["Radius"];
	if (data.contains("IsOccupied")) m_isOccupied = data["IsOccupied"];
}

void HEIN::PossessableComponent::OnInspectorGUI(GameContext& /*gameContext*/)
{
	if (ImGui::CollapsingHeader("PossessableComponent", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::DragFloat("Possess Radius", &m_possessRadius, 0.5f, 1.0f, 100.0f);
		ImGui::Checkbox("Is Occupied", &m_isOccupied);
	}
}
