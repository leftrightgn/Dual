#pragma once
#include "ICameraMode.h"
#include <SimpleMath.h>
#include <DirectXMath.h>

using namespace HEIN;

class DebugCameraMode : public ICameraMode
{
private:
	 
	DirectX::SimpleMath::Vector3 m_target;
	float m_yaw;
	float m_pitch;
	float m_distance;
	float m_mouseSensitivity;
	float m_lastMouseX;
	float m_lastMouseY;
	bool m_isDragging;

    float m_lastScrollValue;
    bool m_needsScrollSync;

public:

    DebugCameraMode(float startDistance = 5.0f, DirectX::SimpleMath::Vector3 target = DirectX::SimpleMath::Vector3::Zero)
        : m_target(target)
        , m_yaw(0.0f)
        , m_pitch(0.0f)
        , m_distance(startDistance)
        , m_mouseSensitivity(0.01f)
        , m_lastMouseX(-1.0f)
        , m_lastMouseY(-1.0f)
        , m_isDragging(false)
        , m_lastScrollValue(0.0f)
        , m_needsScrollSync(true)
	{
	}

    void OnEnter(CameraData& data) override
    {
        m_needsScrollSync = true;
    }
    void OnResume(CameraData& data) override
    {
        m_needsScrollSync = true;
    }

	void ProcessInput(const CameraInputState& input) override 
	{
        if (m_needsScrollSync)
        {
            m_lastScrollValue = input.scrollWheelDelta;
            m_needsScrollSync = false;

        }

        float currentScroll = input.scrollWheelDelta;
        float actualScrollDelta = currentScroll - m_lastScrollValue;

        m_lastScrollValue = currentScroll;

        if (actualScrollDelta != 0.0f)
        {
            m_distance -= (actualScrollDelta / 120.0f);
            m_distance = std::max(0.1f, m_distance); // Prevent zooming past the center point
        }

        // Only look around if the Left Mouse Button is held down
        if (input.isLeftMouseDown)
        {
            if (!m_isDragging)
            {
                m_lastMouseX = input.mouseX;
                m_lastMouseY = input.mouseY;
                m_isDragging = true;
            }
            else
            {
                float deltaX = input.mouseX - m_lastMouseX;
                float deltaY = input.mouseY - m_lastMouseY;

                m_yaw += -deltaX * m_mouseSensitivity;
                m_pitch += -deltaY * m_mouseSensitivity;

                // Update the last position for the next frame
                m_lastMouseX = input.mouseX;
                m_lastMouseY = input.mouseY;
            }
        }
        else
        {
            // Button was released, stop dragging
            m_isDragging = false;
        }

        // Clamp the pitch so it doesn't flip upside down
        const float pitchLimit = DirectX::XM_PIDIV2 - 0.01f;
        m_pitch = std::clamp(m_pitch, -pitchLimit, pitchLimit);

       
	}

	void Update(CameraData& outData, float deltaTime, ICameraController& controller) override
	{
		DirectX::SimpleMath::Matrix rotation = DirectX::SimpleMath::Matrix::CreateFromYawPitchRoll(m_yaw, m_pitch, 0.0f);

		DirectX::SimpleMath::Vector3 offset = rotation.Backward() * m_distance;

		outData.position = m_target + offset;
		outData.viewMatrix = DirectX::SimpleMath::Matrix::CreateLookAt(outData.position, m_target, DirectX::SimpleMath::Vector3::Up);
		outData.fov = DirectX::XM_PIDIV4;

	}

	
};
