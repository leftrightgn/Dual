#pragma once

namespace HEIN
{
	class DebugCollisionRenderer
	{
	private:
		static const uint32_t DISPLAY_COLLISION_MAX = 100;

		uint32_t m_collisionMax;

		bool m_modelActive;

		bool m_lineActive;

		struct Sphere
		{
			DirectX::SimpleMath::Vector3 center;

			float radius;

			DirectX::SimpleMath::Color lineColor;

			constexpr Sphere(
				const DirectX::SimpleMath::Vector3& center,
				float radius,
				DirectX::SimpleMath::Color lineColor) noexcept
				: center(center), radius(radius), lineColor(lineColor) {
			}
		};

		struct Box
		{
			DirectX::SimpleMath::Vector3 center;

			DirectX::SimpleMath::Vector3 extents;

			DirectX::SimpleMath::Quaternion rotate;

			DirectX::SimpleMath::Color lineColor;

			constexpr Box(
				const DirectX::SimpleMath::Vector3& center,
				const DirectX::SimpleMath::Vector3& extents,
				const DirectX::SimpleMath::Quaternion& rotate,
				DirectX::SimpleMath::Color lineColor)noexcept
				: center(center), extents(extents), rotate(rotate), lineColor(lineColor) {
			}
		};

		struct Mesh
		{
			const std::vector<DirectX::VertexPosition>& vertexes;
			
			const std::vector<uint16_t>& indexes;

			DirectX::SimpleMath::Vector3 position;

			DirectX::SimpleMath::Quaternion rotate;

			DirectX::SimpleMath::Color lineColor;

			constexpr Mesh(
				const std::vector<DirectX::VertexPosition>& vertexes,
				const std::vector<uint16_t>& indexes,
				const DirectX::SimpleMath::Vector3& position,
				const DirectX::SimpleMath::Quaternion& rotate,
				DirectX::SimpleMath::Color lineColor) noexcept
				: vertexes(vertexes), indexes(indexes), position(position), rotate(rotate), lineColor(lineColor) {}
		};

		struct LineSegment
		{
			DirectX::SimpleMath::Vector3 a;
			DirectX::SimpleMath::Vector3 b;
			DirectX::SimpleMath::Color lineColor;

			constexpr LineSegment(
				const DirectX::SimpleMath::Vector3& a,
				const DirectX::SimpleMath::Vector3& b,
				DirectX::SimpleMath::Color lineColor) noexcept
				: a(a), b(b), lineColor(lineColor) {}
		};

		std::vector<Sphere> m_spheres;

		std::vector<Box> m_boxes;

		std::vector<Mesh> m_meshes;

		std::vector<LineSegment> m_lineSegments;
		
	    std::unique_ptr<DirectX::GeometricPrimitive> m_modelSphere;

		std::unique_ptr<DirectX::GeometricPrimitive> m_modelBox;

		std::unique_ptr<DirectX::NormalMapEffect> m_modelEffect;

		Microsoft::WRL::ComPtr<ID3D11InputLayout> m_modelInputLayout;

		std::unique_ptr<DirectX::BasicEffect> m_meshEffect;

		Microsoft::WRL::ComPtr<ID3D11InputLayout> m_meshInputLayout;

		std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPosition>> m_meshBatch;

		Microsoft::WRL::ComPtr<ID3D11Buffer> m_instancedVB;

		std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>> m_primitiveBatch;

		std::unique_ptr<DirectX::BasicEffect> m_lineEffect;

		Microsoft::WRL::ComPtr<ID3D11InputLayout> m_lineInputLayout;

    private:

		void DrawSolidShapes(
			ID3D11DeviceContext* context,
			const DirectX::CommonStates& states,
			const DirectX::SimpleMath::Matrix& view,
			const DirectX::SimpleMath::Matrix& proj,
			DirectX::FXMVECTOR color
		);

		void DrawWireFrames(
			ID3D11DeviceContext* context,
			const DirectX::CommonStates& states,
			const DirectX::SimpleMath::Matrix& view,
			const DirectX::SimpleMath::Matrix& proj,
			DirectX::FXMVECTOR color
		);

    public:

		DebugCollisionRenderer(
			ID3D11Device* device,
			ID3D11DeviceContext* context,
			bool modelActive = true,
			bool lineActive = true,
			uint32_t collisionMax = DISPLAY_COLLISION_MAX
		);

		void RenderAndFlush(
			ID3D11DeviceContext* context,
			const DirectX::CommonStates& states,
			const DirectX::SimpleMath::Matrix& view,
			const DirectX::SimpleMath::Matrix& proj,
			DirectX::FXMVECTOR baseColor = DirectX::Colors::White,
			DirectX::FXMVECTOR lineColor = DirectX::XMVECTORF32{ 0.0f, 0.0f, 0.0f, 0.0f },
			float alpha = 0.5f
		);

		void QueueSphere(
			DirectX::BoundingSphere sphere,
			DirectX::FXMVECTOR lineColor = DirectX::XMVECTORF32{ 0.0f, 0.0f, 0.0f, 0.0f }
		)
		{
			DirectX::XMFLOAT3 center = sphere.Center;
			m_spheres.push_back(Sphere(center, sphere.Radius, lineColor));
		}

		void QueueAABB(
			DirectX::BoundingBox box,
			DirectX::FXMVECTOR lineColor = DirectX::XMVECTORF32{ 0.0f, 0.0f, 0.0f, 0.0f }
		)
		{
			m_boxes.push_back(Box(box.Center, box.Extents, DirectX::SimpleMath::Quaternion(), lineColor));
		}

		void QueueOBB(
			DirectX::BoundingOrientedBox obb,
			DirectX::FXMVECTOR lineColor = DirectX::XMVECTORF32{ 0.0f, 0.0f, 0.0f, 0.0f }
		)
		{
			m_boxes.push_back(Box(obb.Center, obb.Extents, DirectX::SimpleMath::Quaternion(obb.Orientation), lineColor));
		}

		void QueueMesh(
			const std::vector<DirectX::VertexPosition>& vertexes,
			const std::vector<uint16_t>& indexes,
			DirectX::SimpleMath::Vector3 position,
			DirectX::SimpleMath::Quaternion rotate,
			DirectX::FXMVECTOR lineColor = DirectX::XMVECTORF32{ 0.0f, 0.0f, 0.0f, 0.0f }
		)
		{
			m_meshes.push_back(Mesh(vertexes, indexes, position, rotate, lineColor));
		}

		void QueueLine(
			DirectX::SimpleMath::Vector3 a,
			DirectX::SimpleMath::Vector3 b,
			DirectX::FXMVECTOR lineColor = DirectX::XMVECTORF32{ 0.0f, 0.0f, 0.0f, 0.0f }
		)
		{
			m_lineSegments.push_back(LineSegment(a, b, lineColor));
		}

		void SetSolidRenderingEnable(bool active) { m_modelActive = active; }

		void SetWireFrameRenderingEnable(bool active) { m_lineActive = active; }
	};

}