#pragma once
#include "ICameraMode.h"
#include <utility>
#include <functional>
#include <unordered_map>
#include <vector>
#include <memory>
#include <optional>
#include <cmath>

namespace HEIN
{
	class CameraController final : public ICameraController
	{
	private:
		enum class Command { None, Switch, Push, Pop };

		CameraData m_data;

		using CameraFactory = std::function<std::unique_ptr<ICameraMode>()>;
		std::unordered_map<CameraType, CameraFactory> m_factories;
		std::vector<std::unique_ptr<ICameraMode>> m_cameraStack;

		Command m_nextCommand = Command::None;
		std::optional<CameraType> m_nextCameraKey = std::nullopt;

		CameraData m_previousCameraData;
		bool m_isBlending = false;
		float m_blendDuration = 1.0f;
		float m_blendTimer = 0.0f;

	public:

		
		void RegisterCamera(CameraType key, CameraFactory factory)
		{
			m_factories[key] = factory;
		}

		// Sets the initial camera instantly
		void SetFirstCamera(CameraType key)
		{
			std::unique_ptr<ICameraMode> firstCam = m_factories[key]();
			m_cameraStack.push_back(std::move(firstCam));
			m_cameraStack.back()->OnEnter(m_data);
		}

		void Update(float deltaTime)
		{
			if (!m_cameraStack.empty())
			{
				m_cameraStack.back()->Update(m_data, deltaTime, *this);
			}

			if (m_isBlending)
			{
				m_blendTimer += deltaTime;
				float t = m_blendTimer / m_blendDuration;

				if (t >= 1.0f)
				{
					m_isBlending = false;
				}
				else
				{
					m_data.position = DirectX::SimpleMath::Vector3::Lerp(m_previousCameraData.position, m_data.position, t);

					m_data.fov = std::lerp(m_previousCameraData.fov, m_data.fov, t);

					DirectX::SimpleMath::Quaternion oldRot = DirectX::SimpleMath::Quaternion::CreateFromRotationMatrix(m_previousCameraData.viewMatrix.Transpose());
					DirectX::SimpleMath::Quaternion newRot = DirectX::SimpleMath::Quaternion::CreateFromRotationMatrix(m_data.viewMatrix.());

					DirectX::SimpleMath::Quaternion blendRot = DirectX::SimpleMath::Quaternion::Slerp(oldRot, newRot, t);

					DirectX::SimpleMath::Matrix camWorld = DirectX::SimpleMath::Matrix::CreateFromQuaternion(blendRot);
					camWorld.Translation(m_data.position);
					m_data.viewMatrix = camWorld.Invert();
				}
			}
			ApplyRequest();
		}

		void ProcessInput(const CameraInputState& input)
		{
			if (!m_cameraStack.empty())
			{
				m_cameraStack.back()->ProcessInput(input);
			}
		}

		// Getters for GameScene.cpp
		DirectX::SimpleMath::Matrix GetView() const { return m_data.viewMatrix; }
		DirectX::SimpleMath::Vector3 GetPosition() const { return m_data.position; }
		float GetFov() const { return m_data.fov; }

		bool LocksPlayerRotation() const
		{
			if (m_cameraStack.empty()) return false;
			return m_cameraStack.back()->LocksPlayerRotation();
		}

		void RequestSwitch(CameraType type) override
		{
			m_nextCommand = Command::Switch;
			m_nextCameraKey = type;
		}

		void RequestPush(CameraType type) override
		{
			m_nextCommand = Command::Push;
			m_nextCameraKey = type;
		}

		void RequestPop(CameraType /*type*/) override
		{
			m_nextCommand = Command::Pop;
		}

	private:

		void ApplyRequest()
		{
			if (m_nextCommand == Command::None) return;

			m_previousCameraData = m_data;
			m_isBlending = true;
			m_blendTimer = 0.0f;

			if (m_nextCommand == Command::Switch)
			{
				if (!m_cameraStack.empty()) m_cameraStack.back()->OnExit(m_data);

				std::unique_ptr<ICameraMode> newCam = m_factories[*m_nextCameraKey]();
				m_cameraStack.back() = std::move(newCam);

				m_cameraStack.back()->OnEnter(m_data);
				UpdateMouseMode();
			}
			else if (m_nextCommand == Command::Push)
			{
				if (!m_cameraStack.empty()) m_cameraStack.back()->OnSuspend(m_data);

			
				std::unique_ptr<ICameraMode> newCam = m_factories[*m_nextCameraKey]();
				m_cameraStack.push_back(std::move(newCam));

				m_cameraStack.back()->OnEnter(m_data);
				UpdateMouseMode();
			}
			else if (m_nextCommand == Command::Pop)
			{
				m_cameraStack.back()->OnExit(m_data);
				m_cameraStack.pop_back();
				if (!m_cameraStack.empty()) m_cameraStack.back()->OnResume(m_data);
				UpdateMouseMode();
			}

			m_nextCommand = Command::None;
			m_nextCameraKey = std::nullopt;
		}

		void UpdateMouseMode()
		{
			if (m_cameraStack.back()->RequiresRelativeMouse())
				DirectX::Mouse::Get().SetMode(DirectX::Mouse::MODE_RELATIVE);
			else
				DirectX::Mouse::Get().SetMode(DirectX::Mouse::MODE_ABSOLUTE);
		}
	};
}