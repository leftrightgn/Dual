#pragma once
#include <Entities/Actor.h>
#include <Camera/CameraController.h>
#include <Components/SkinnedModelComponent.h>

namespace HEIN
{
	class SocketComponent;
	class ActorManager;
	// Data
	struct PlayerSpawnData
	{
		HEIN::ActorID playerID = HEIN::INVALID_ACTOR_ID;
		HEIN::SkinnedModelComponent* tpsModel = nullptr;
		//HEIN::SkinnedModelComponent* fpsModel = nullptr;
	};

	struct EnemySpawnData
	{
		HEIN::ActorID enemyID = HEIN::INVALID_ACTOR_ID;
		HEIN::SkinnedModelComponent* tpsModel = nullptr;
	};

	class ActorFactory
	{
	public:

		static PlayerSpawnData CreateKnight(
			ActorManager& actorManager,
			GameContext& gameContext,
			CameraController* cameraController,
			DirectX::SimpleMath::Vector3* targetCameraOut
		);

		static HEIN::ActorID CreateSword(
			ActorManager& actorManager,
			GameContext& gameContext,
			HEIN::ActorID wielderID
		);

		static HEIN::ActorID CreateStage(
			ActorManager& actorManager,
			GameContext& gameContext
		);

		static EnemySpawnData CreateEnemy(
			ActorManager& actorManager,
			GameContext& gameContext,
			HEIN::ActorID targetID
		);

	};

}
