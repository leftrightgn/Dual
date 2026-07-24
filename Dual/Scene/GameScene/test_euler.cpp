#include <iostream>
#include <cmath>
#include <DirectXMath.h>
#include <SimpleMath.h>

using namespace DirectX;
using namespace DirectX::SimpleMath;

Vector3 GetRotationEuler(const Quaternion& m_rotation)
{
     Vector3 euler;
     float sinp = 2.0f * (m_rotation.w * m_rotation.x - m_rotation.y * m_rotation.z);
     if (std::abs(sinp) >= 1.0f) euler.x = std::copysign(XM_PIDIV2, sinp);
     else euler.x = std::asin(sinp);

     float siny_cosp = 2.0f * (m_rotation.w * m_rotation.y + m_rotation.z * m_rotation.x);
     float cosy_cosp = 1.0f - 2.0f * (m_rotation.x * m_rotation.x + m_rotation.y * m_rotation.y);
     euler.y = std::atan2(siny_cosp, cosy_cosp);

     float sinr_cosp = 2.0f * (m_rotation.w * m_rotation.z + m_rotation.x * m_rotation.y);
     float cosr_cosp = 1.0f - 2.0f * (m_rotation.y * m_rotation.y + m_rotation.z * m_rotation.z);
     euler.z = std::atan2(sinr_cosp, cosr_cosp);
     return euler;
}

int main()
{
    float pitch = 0.5f, yaw = 1.0f, roll = -0.5f;
    Quaternion q = Quaternion::CreateFromYawPitchRoll(yaw, pitch, roll);
    Vector3 euler = GetRotationEuler(q);
    std::cout << "Original: pitch=" << pitch << " yaw=" << yaw << " roll=" << roll << "\n";
    std::cout << "Extracted: pitch=" << euler.x << " yaw=" << euler.y << " roll=" << euler.z << "\n";
    return 0;
}
