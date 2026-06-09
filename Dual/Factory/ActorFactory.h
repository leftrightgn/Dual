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
		HEIN::SkinnedModelComponent* tpsModel;
		HEIN::SkinnedModelComponent* fpsModel;
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

	};

}
