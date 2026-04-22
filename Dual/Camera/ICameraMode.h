#pragma once
#include "pch.h"

// Data Container
struct CameraData
{
	DirectX::SimpleMath::Vector3 position = {};
	DirectX::SimpleMath::Matrix viewMatrix = {};
	DirectX::SimpleMath::Matrix projMatrix = {};
	float fov = {};
};

// Input Data Container
struct CameraInputState
{
	float mouseX = 0.0f;
	float mouseY = 0.0f;
	float scrollWheelDelta = 0.0f;
};
// Strategy interface
class ICameraMode
{
public:

	virtual ~ICameraMode() = default;

	virtual void Update(CameraData& outData, float deltaTime) = 0;
	virtual void ProcessInput(const CameraInputState& input) = 0;
};
