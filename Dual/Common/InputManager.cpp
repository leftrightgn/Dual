#include "pch.h"
#include "InputManager.h"
#include "Camera/CameraController.h"
#include "Framework/GameContext.h"

namespace HEIN
{
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

