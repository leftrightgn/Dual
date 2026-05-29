#pragma once
#include <SimpleMath.h>

struct GameContext;
namespace HEIN
{
	enum class CameraType;

	class InputManager
	{

	public:

		InputManager() = default;
		~InputManager() = default;

		static DirectX::SimpleMath::Vector3 GetMoveIntent(const GameContext& context);

		static bool IsAttacking(const GameContext& context);

		static bool WasCameraSwitchPressed(const GameContext& context, HEIN::CameraType& outType);
		static bool WasDebugMagnifyPressed(const GameContext& context);
		static bool WasDebugTogglePressed(const GameContext& context);
	};
}
