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
		m_skinBones = DirectX::ModelBone::MakeArray(m_model->bones.size());
		m_targetBones = DirectX::ModelBone::MakeArray(m_model->bones.size());

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
		if (!m_model) return;

		if (m_isBlending && m_currentAnimation != nullptr &&
			m_targetAnimation != nullptr)
		{
			m_blendTimer += deltaTime;
			float blendFactor = m_blendTimer / m_blendDuration;

			if (blendFactor >= 1.0f)
			{
				m_currentAnimation = m_targetAnimation;
				m_targetAnimation = nullptr;
				m_isBlending = false;

				m_currentAnimation->Update(deltaTime);
				m_currentAnimation->Apply(*m_model, m_model->bones.size(), m_drawBones.get());
			}
			else
			{
				m_currentAnimation->Update(deltaTime);
				m_targetAnimation->Update(deltaTime);

				m_currentAnimation->Apply(*m_model, m_model->bones.size(), m_drawBones.get());
				m_targetAnimation->Apply(*m_model, m_model->bones.size(), m_targetBones.get());

				for (size_t i = 0; i < m_model->bones.size(); ++i)
				{

					// 1. LOW-LEVEL TRANSLATION
					DirectX::SimpleMath::Vector3 posA = m_drawBones[i].r[3];
					DirectX::SimpleMath::Vector3 posB = m_targetBones[i].r[3];

					// 2. ROTATION
					DirectX::SimpleMath::Quaternion rotA = DirectX::SimpleMath::Quaternion::CreateFromRotationMatrix(m_drawBones[i]);
					DirectX::SimpleMath::Quaternion rotB = DirectX::SimpleMath::Quaternion::CreateFromRotationMatrix(m_targetBones[i]);

					// 3. FAST SCALE EXTRACTION (Extract the length of the X, Y, and Z axis rows)
					DirectX::SimpleMath::Vector3 scaleA(
						DirectX::SimpleMath::Vector3(m_drawBones[i].r[0]).Length(),
						DirectX::SimpleMath::Vector3(m_drawBones[i].r[1]).Length(),
						DirectX::SimpleMath::Vector3(m_drawBones[i].r[2]).Length()
					);
					DirectX::SimpleMath::Vector3 scaleB(
						DirectX::SimpleMath::Vector3(m_targetBones[i].r[0]).Length(),
						DirectX::SimpleMath::Vector3(m_targetBones[i].r[1]).Length(),
						DirectX::SimpleMath::Vector3(m_targetBones[i].r[2]).Length()
					);

					// 4. BLEND EVERYTHING
					DirectX::SimpleMath::Vector3 blendedPos = DirectX::SimpleMath::Vector3::Lerp(posA, posB, blendFactor);
					DirectX::SimpleMath::Quaternion blendedRot = DirectX::SimpleMath::Quaternion::Slerp(rotA, rotB, blendFactor);
					DirectX::SimpleMath::Vector3 blendedScale = DirectX::SimpleMath::Vector3::Lerp(scaleA, scaleB, blendFactor);

					// 5. REBUILD
					m_drawBones[i] = DirectX::SimpleMath::Matrix::CreateScale(blendedScale) * DirectX::SimpleMath::Matrix::CreateFromQuaternion(blendedRot) * DirectX::SimpleMath::Matrix::CreateTranslation(blendedPos);
				}

			}
		}
		else if (m_currentAnimation != nullptr)
		{
			m_currentAnimation->Update(deltaTime);
			m_currentAnimation->Apply(*m_model, m_model->bones.size(), m_drawBones.get());
		}
		if (m_currentAnimation != nullptr)
		{
			for (size_t i = 0; i < m_model->bones.size(); i++)
			{
				m_skinBones[i] = m_drawBones[i];
			}
			m_currentAnimation->ApplySkinMatrix(*m_model, m_model->bones.size(), m_skinBones.get());
		}
	}

	void SkinnedModelComponent::Draw(GameContext& gameContext, const DirectX::SimpleMath::Matrix& world, const DirectX::SimpleMath::Matrix& view, const DirectX::SimpleMath::Matrix& proj)
	{
		if (!m_isVisible) return;

		if (!m_model) return;

		ID3D11DeviceContext* context = gameContext.deviceResources.GetD3DDeviceContext();
		DirectX::DX11::CommonStates& states = gameContext.commonStates;

		m_model->DrawSkinned(context, states, m_model->bones.size(), m_skinBones.get(), world, view, proj);
	}

	DirectX::SimpleMath::Vector3 SkinnedModelComponent::GetBoneWorldPosition(
		const wchar_t* boneName, 
		const DirectX::SimpleMath::Matrix& actorWorldMatrix)
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

	DirectX::SimpleMath::Vector3 SkinnedModelComponent::GetBoneWorldPosition(
		const int boneNum, 
		const DirectX::SimpleMath::Matrix& actorWorldMatrix
	)
	{
		if (!m_model) return DirectX::SimpleMath::Vector3::Zero;

		DirectX::SimpleMath::Matrix boneMatrix = m_drawBones[boneNum];
		DirectX::SimpleMath::Matrix finalWorldMatrix = boneMatrix * actorWorldMatrix;

		return finalWorldMatrix.Translation();
	}

	DirectX::SimpleMath::Matrix SkinnedModelComponent::GetBoneWorldMatrix(
		const wchar_t* boneName,
		const DirectX::SimpleMath::Matrix& actorWorldMatrix
	)
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

	DirectX::SimpleMath::Matrix SkinnedModelComponent::GetBoneWorldMatrix(const int boneNum, const DirectX::SimpleMath::Matrix& actorWorldMatrix)
	{
		if (!m_model) return DirectX::SimpleMath::Matrix::Identity;

		if (boneNum < 0 || static_cast<size_t>(boneNum) >= m_model->bones.size())
		{
			return DirectX::SimpleMath::Matrix::Identity;
		}
		DirectX::SimpleMath::Matrix boneMatrix = m_drawBones[boneNum];
		DirectX::SimpleMath::Matrix finalWorldMatrix = boneMatrix * actorWorldMatrix;

		return finalWorldMatrix;
	}

	int SkinnedModelComponent::GetBoneIndex(const std::wstring boneName)
	{
		if (!m_model) return -1;
		for (size_t i = 0; i < m_model->bones.size(); i++)
		{
			if (m_model->bones[i].name.find(boneName) != std::wstring::npos)
			{
				return static_cast<int>(i);
			}
		}
		return -1;
	}

	void SkinnedModelComponent::LoadAnimation(const std::string& name, const wchar_t* animPath)
	{
		if (!m_model) return;

		std::unique_ptr<DX::AnimationSDKMESH> newAnim = std::make_unique<DX::AnimationSDKMESH>();
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

	void SkinnedModelComponent::CrossfadeAnimation(const std::string& name, float duration)
	{
		std::unordered_map<std::string, std::unique_ptr<DX::AnimationSDKMESH>>::iterator it =
			m_animations.find(name);

		if (it != m_animations.end())
		{
			if (m_currentAnimation == nullptr)
			{
				m_currentAnimation = it->second.get();
				return;
			}

			if (m_isBlending && m_targetAnimation == it->second.get()) return;
			
			if (m_currentAnimation == it->second.get())
			{
				if (m_isBlending)
				{
					m_isBlending = false;
					m_targetAnimation = nullptr;
				}
				return;
			}

			m_targetAnimation = it->second.get();
			m_blendDuration = duration;
			m_blendTimer = 0.0f;
			m_isBlending = true;
		}
	}

}