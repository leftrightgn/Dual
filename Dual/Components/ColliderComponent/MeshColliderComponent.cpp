#include "pch.h"
#include "MeshColliderComponent.h"
#include <fstream>
#include <sstream>

HEIN::MeshColliderComponent::MeshColliderComponent(Actor* owner)
	: ColliderComponent(owner, ColliderShape::Mesh)
{
}

void HEIN::MeshColliderComponent::LoadFromObj(const wchar_t* filePath)
{
	std::ifstream file(filePath);
	if (!file.is_open()) return;

	std::vector<DirectX::SimpleMath::Vector3> vertices;
	std::string line;

	while (std::getline(file, line))
	{
		std::istringstream iss(line);
		std::string type;
		iss >> type;

		if (type == "v")
		{
			float x, y, z;
			iss >> x >> y >> z;
			vertices.push_back(DirectX::SimpleMath::Vector3(x, y, z));
		}
		else if (type == "f")
		{
			std::string v1, v2, v3;
			iss >> v1 >> v2 >> v3;

			int i1 = std::stoi(v1.substr(0, v1.find('/'))) - 1;
			int i2 = std::stoi(v2.substr(0, v2.find('/'))) - 1;
			int i3 = std::stoi(v3.substr(0, v3.find('/'))) - 1;

			Triangle tri;
			tri.v0 = vertices[i1];
			tri.v1 = vertices[i2];
			tri.v2 = vertices[i3];

			m_localTriangles.push_back(tri);
		}
	}
	m_worldTriangles.resize(m_localTriangles.size());
}

void HEIN::MeshColliderComponent::SyncColliderState()
{
	DirectX::SimpleMath::Matrix worldMatrix = GetCalculateWorldMatrix();

	for (size_t i = 0; i < m_localTriangles.size(); ++i)
	{
		m_worldTriangles[i].v0 = DirectX::SimpleMath::Vector3::Transform(m_localTriangles[i].v0, worldMatrix);
		m_worldTriangles[i].v1 = DirectX::SimpleMath::Vector3::Transform(m_localTriangles[i].v1, worldMatrix);
		m_worldTriangles[i].v2 = DirectX::SimpleMath::Vector3::Transform(m_localTriangles[i].v2, worldMatrix);
	}
}
