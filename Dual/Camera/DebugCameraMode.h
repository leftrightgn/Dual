#pragma once
#include "ICameraMode.h"
#include <SimpleMath.h>

namespace HEIN
{
    class DebugCameraMode : public ICameraMode
    {
    private:

        static constexpr float PITCH = 0.0f;
        static constexpr float YAW = 0.0f;
        static constexpr float MOUSE_SENSITIVITY = 0.01f;
        static constexpr float LAST_MOUSEPOS = -1.0f;
        static constexpr float SCROLLVALUE = 0.0f;
        static constexpr float DEFAULT_STARTDIS = 5.0f;

    private:

        DirectX::SimpleMath::Vector3 m_target;
        float m_pitch;
        float m_yaw;
        float m_distance;
        float m_mouseSensitivity;
        float m_lastMouseX;
        float m_lastMouseY;
        bool m_isDragging;

        float m_lastScrollValue;
        bool m_needsScrollSync;

    public:

        DebugCameraMode(
            float startDistance = DEFAULT_STARTDIS,
            DirectX::SimpleMath::Vector3 target = DirectX::SimpleMath::Vector3::Zero
        );
            
        void OnEnter(CameraData& data) override;
       
        void OnResume(CameraData& data) override;
   
        void ProcessInput(const CameraInputState& input) override;

        void Update(CameraData& outData, float deltaTime, ICameraController& controller) override;
     
    };
}