#pragma once
#include "pch.h"

// Data Container
struct CameraData
{
	DirectX::SimpleMath::Vector3 position;
	DirectX::SimpleMath::Matrix viewMatrix;
	DirectX::SimpleMath::Matrix projMatrix;
	float fov;
};

// Strategy interface
class ICameraMode
{
public:

	~ICameraMode() = default;

	virtual void Update(CameraData& outData, float deltaTime) = 0;
	virtual void ProcessInput(float mouseX, float mouseY) = 0;
};
