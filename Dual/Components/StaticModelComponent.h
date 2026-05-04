#pragma once
#include "Components/IComponent.h"
#include "GameContext.h"
#include "pch.h"

namespace HEIN
{
	class StaticModelComponent : public IComponent
	{
	private:

		std::unique_ptr<DirectX::Model> m_model;
		std::unique_ptr<DirectX::EffectFactory> m_fxFactory;
		bool m_isVisible = true;
    public:
        StaticModelComponent(Actor* owner) : IComponent(owner) {}

        void Initialize(GameContext& gameContext, const wchar_t* modelPath, const wchar_t* textureDir)
        {
            ID3D11Device* device = gameContext.deviceResources.GetD3DDevice();

            m_fxFactory = std::make_unique<DirectX::EffectFactory>(device);
            static_cast<DirectX::EffectFactory*>(m_fxFactory.get())->SetDirectory(textureDir);

            m_model = DirectX::Model::CreateFromSDKMESH(device, modelPath, *m_fxFactory);
        }

        void Update(float /*deltaTime*/) override {}

        void Draw(GameContext& gameContext, const DirectX::SimpleMath::Matrix& world, const DirectX::SimpleMath::Matrix& view, const DirectX::SimpleMath::Matrix& proj)
        {
            if (!m_isVisible || !m_model) return;

            ID3D11DeviceContext* context = gameContext.deviceResources.GetD3DDeviceContext();
            DirectX::DX11::CommonStates& states = gameContext.commonStates;

           
            m_model->Draw(context, states, world, view, proj);
        }

        void SetVisible(bool visible) { m_isVisible = visible; }
        bool IsVisible() const { return m_isVisible; }
	};
}
