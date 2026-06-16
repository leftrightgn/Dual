#pragma once
#include <Entities/Actor.h>
#include <Camera/CameraController.h>
#include <Components/SkinnedModelComponent.h>

namespace HEIN
{
	class SocketComponent;
	// Data
	struct PlayerSpawnData
	{
		std::unique_ptr<HEIN::Actor> playerActor;
		HEIN::SkinnedModelComponent* tpsModel = nullptr;
		HEIN::SkinnedModelComponent* fpsModel = nullptr;
	};

	struct EnemySpawnData
	{
		std::unique_ptr<HEIN::Actor> enemyActor;
		HEIN::SkinnedModelComponent* tpsModel = nullptr;
	};

	class ActorFactory
	{
	public:

		static PlayerSpawnData CreateKnight(
			GameContext& gameContext,
			CameraController* cameraController,
			DirectX::SimpleMath::Vector3* targetCameraOut
		);

		static std::unique_ptr<HEIN::Actor> CreateSword(
			GameContext& gameContext,
			HEIN::SocketComponent* targetPlayerSocket
		);

		static std::unique_ptr<HEIN::Actor> CreateStage(
			GameContext& gameContext
		);

		static EnemySpawnData CreateEnemy(
			GameContext& gameContext
		);

	};

}
