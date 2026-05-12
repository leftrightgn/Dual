#include "pch.h"
#include "DebugCameraMode.h"

HEIN::DebugCameraMode::DebugCameraMode(
	  float startDistance, 
	  DirectX::SimpleMath::Vector3 target
)
    : m_target(target)
    , m_pitch(PITCH)
    , m_yaw(YAW)
    , m_distance(startDistance)
    , m_mouseSensitivity(MOUSE_SENSITIVITY)
    , m_lastMouseX(LAST_MOUSEPOS)
    , m_lastMouseY(LAST_MOUSEPOS)
    , m_isDragging(false)
    , m_lastScrollValue(SCROLLVALUE)
    , m_needsScrollSync(true)
{
}

void HEIN::DebugCameraMode::OnEnter(CameraData& /*data*/)
{
    m_needsScrollSync = true;
}

void HEIN::DebugCameraMode::OnResume(CameraData& /*data*/)
{
    m_needsScrollSync = true;
}

void HEIN::DebugCameraMode::ProcessInput(const CameraInputState& input)
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

void HEIN::DebugCameraMode::Update(CameraData& outData, float /*deltaTime*/, ICameraController& /*controller*/)
{
    DirectX::SimpleMath::Matrix rotation = DirectX::SimpleMath::Matrix::CreateFromYawPitchRoll(m_yaw, m_pitch, 0.0f);

    DirectX::SimpleMath::Vector3 offset = rotation.Backward() * m_distance;

    outData.position = m_target + offset;
    outData.viewMatrix = DirectX::SimpleMath::Matrix::CreateLookAt(outData.position, m_target, DirectX::SimpleMath::Vector3::Up);
    outData.fov = DirectX::XMConvertToRadians(50.0f);

}