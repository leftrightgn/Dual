#include "pch.h"
#include "InputManager.h"
#include "Camera/CameraController.h"
#include "Framework/GameContext.h"

namespace HEIN
{
	void InputManager::Update(const GameContext& context)
	{
		if (context.mouseState.positionMode == DirectX::Mouse::MODE_RELATIVE)
		{
			m_deltaX = context.mouseState.x;
			m_deltaY = context.mouseState.y;
		}
		else
		{
			m_deltaX = context.mouseState.x - m_lastMouseX;
			m_deltaY = context.mouseState.y - m_lastMouseY;

		}
		
		m_lastMouseX = context.mouseState.x;
		m_lastMouseY = context.mouseState.y;

		
	}
	std::pair<int, int> InputManager::GetMouseDelta()
	{
		return std::make_pair(m_deltaX, m_deltaY);
	}
	
	bool InputManager::IsDebugDrugHeld(const GameContext& context)
	{
		return context.mouseState.leftButton;
	}
	DirectX::SimpleMath::Vector3 InputManager::GetMoveIntent(const GameContext& context)
	{
		DirectX::SimpleMath::Vector3 intent = DirectX::SimpleMath::Vector3::Zero;

		if (context.keyboardState.W) intent.z += 1.0f;
		if (context.keyboardState.S) intent.z -= 1.0f;
		if (context.keyboardState.A) intent.x += 1.0f;
		if (context.keyboardState.D) intent.x -= 1.0f;

		return intent;
	}
	bool InputManager::IsAttacking(const GameContext& context)
	{
		return context.mouseState.leftButton;
	}
	
	bool InputManager::WasCameraSwitchPressed(const GameContext& context, HEIN::CameraType& outType)
	{
		if (context.keyboardTracker.pressed.T)
		{
			outType = HEIN::CameraType::ThirdPerson;
			return true;
		}
		if (context.keyboardTracker.pressed.P)
		{
			outType = HEIN::CameraType::FirstPerson;
			return true;
		}
		if (context.keyboardTracker.pressed.E)
		{
			outType = HEIN::CameraType::Spring;
			return true;
		}
		return false;
	}
	bool InputManager::WasDebugMagnifyPressed(const GameContext& context)
	{
		return context.keyboardTracker.pressed.F2;
	}
	bool InputManager::WasDebugTogglePressed(const GameContext& context)
	{
		return context.keyboardTracker.pressed.F3;
	}
}

