#include "pch.h"
#include "SkinnedModelComponent.h"
#include "Entities/Actor.h"


namespace HEIN
{
	SkinnedModelComponent::SkinnedModelComponent(Actor* owner)
		: IComponent(owner)
	{
	}

	void SkinnedModelComponent::Initialize(GameContext& gameContext, const wchar_t* modelPath, const wchar_t* textureDir)
	{
		ID3D11Device* device = gameContext.deviceResources.GetD3DDevice();

		m_fxFactory = std::make_unique<DirectX::EffectFactory>(device);
		static_cast<DirectX::EffectFactory*>(m_fxFactory.get())->SetDirectory(textureDir);

		m_model = DirectX::Model::CreateFromSDKMESH(
			device,
			modelPath,
			*m_fxFactory,
			static_cast<DirectX::ModelLoaderFlags>
			(
				DirectX::ModelLoader_Clockwise |
				DirectX::ModelLoader_IncludeBones
		    )
		);

		m_drawBones = DirectX::ModelBone::MakeArray(m_model->bones.size());

		// bone name checker
		/*OutputDebugStringW(L"--- BONE LIST START ---\n");
		for (const auto& bone : m_model->bones)
		{
			OutputDebugStringW(bone.name.c_str());
			OutputDebugStringW(L"\n");
		}
		OutputDebugStringW(L"--- BONE LIST END ---\n");*/



	}

	void SkinnedModelComponent::Update(float deltaTime)
	{
		if (m_currentAnimation && m_model)
		{
			m_currentAnimation->Update(deltaTime);
			m_currentAnimation->Apply(*m_model, m_model->bones.size(), m_drawBones.get());
		}
	}

	void SkinnedModelComponent::Draw(GameContext& gameContext, const DirectX::SimpleMath::Matrix& world, const DirectX::SimpleMath::Matrix& view, const DirectX::SimpleMath::Matrix& proj)
	{
		if (!m_isVisible) return;

		if (!m_model) return;

		ID3D11DeviceContext* context = gameContext.deviceResources.GetD3DDeviceContext();
		DirectX::DX11::CommonStates& states = gameContext.commonStates;

		m_model->DrawSkinned(context, states, m_model->bones.size(), m_drawBones.get(), world, view, proj);
	}

	DirectX::SimpleMath::Vector3 SkinnedModelComponent::GetBoneWorldPosition(const wchar_t* boneName, const DirectX::SimpleMath::Matrix& actorWorldMatrix)
	{
		if (!m_model) return DirectX::SimpleMath::Vector3::Zero;

		for (size_t i = 0; i < m_model->bones.size(); i++)
		{
			if (m_model->bones[i].name.find(boneName) != std::wstring::npos)
			{
				DirectX::SimpleMath::Matrix boneMatrix = m_drawBones[i];
				DirectX::SimpleMath::Matrix finalWorldMatrix = boneMatrix * actorWorldMatrix;

				return finalWorldMatrix.Translation();
			}
		}

		return DirectX::SimpleMath::Vector3::Zero;
	}

	DirectX::SimpleMath::Vector3 SkinnedModelComponent::GetBoneWorldPosition(const int boneNum, const DirectX::SimpleMath::Matrix& actorWorldMatrix)
	{
		DirectX::SimpleMath::Matrix boneMatrix = m_drawBones[boneNum];
		DirectX::SimpleMath::Matrix finalWorldMatrix = boneMatrix * actorWorldMatrix;

		return finalWorldMatrix.Translation();
	}

	DirectX::SimpleMath::Matrix SkinnedModelComponent::GetBoneWorldMatrix(const wchar_t* boneName, const DirectX::SimpleMath::Matrix& actorWorldMatrix)
	{
		if (!m_model) return DirectX::SimpleMath::Matrix::Identity;

		for (size_t i = 0; i < m_model->bones.size(); i++)
		{
			if (m_model->bones[i].name.find(boneName) != std::wstring::npos)
			{
				DirectX::SimpleMath::Matrix boneMatrix = m_drawBones[i];
				DirectX::SimpleMath::Matrix finalWorldMatrix = boneMatrix * actorWorldMatrix;

				return finalWorldMatrix;
			}
		}

		return DirectX::SimpleMath::Matrix::Identity;
	}

	void SkinnedModelComponent::LoadAnimation(const std::string& name, const wchar_t* animPath)
	{
		if (!m_model) return;

		auto newAnim = std::make_unique<DX::AnimationSDKMESH>();
		DX::ThrowIfFailed(newAnim->Load(animPath));
		newAnim->Bind(*m_model);

		m_animations[name] = std::move(newAnim);

		if (m_currentAnimation == nullptr)
		{
			m_currentAnimation = m_animations[name].get();
		}

	}

	void SkinnedModelComponent::ChangeAnimation(const std::string& name)
	{
		auto it = m_animations.find(name);
		if (it != m_animations.end())
		{
			m_currentAnimation = it->second.get();
		}
	}

}