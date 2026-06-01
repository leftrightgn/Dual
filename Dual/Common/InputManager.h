#pragma once
#include <SimpleMath.h>

struct GameContext;
namespace HEIN
{
	enum class CameraType;

	class InputManager
	{
	private:

		inline static int m_lastMouseX = 0;
		inline static int m_lastMouseY = 0;

		inline static int m_deltaX = 0;
		inline static int m_deltaY = 0;
	public:

		InputManager() = default;
		~InputManager() = default;

		static void Update(const GameContext& context);

		static std::pair<int, int> GetMouseDelta();

		static bool IsDebugDrugHeld(const GameContext& context);

		static DirectX::SimpleMath::Vector3 GetMoveIntent(const GameContext& context);

		static bool IsAttacking(const GameContext& context);

		static bool WasCameraSwitchPressed(const GameContext& context, HEIN::CameraType& outType);
		static bool WasDebugMagnifyPressed(const GameContext& context);
		static bool WasDebugTogglePressed(const GameContext& context);


	};
}
