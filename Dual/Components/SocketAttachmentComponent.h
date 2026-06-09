#pragma once
#include <Components/IComponent.h>
#include <string>

namespace HEIN
{
	class SocketComponent;

	class SocketAttachmentComponent : public IComponent
	{
	private:

		HEIN::SocketComponent* m_targetSocket;
		std::wstring m_socketName;

	public:

		SocketAttachmentComponent(Actor* owner);

		void Initialize(SocketComponent* targetSocket, const std::wstring& socketName);

		void Start() override {}
		void Update(float deltaTime) override;
		void Draw(
			GameContext& gameContext,
			const DirectX::SimpleMath::Matrix& world,
			const DirectX::SimpleMath::Matrix& view,
			const DirectX::SimpleMath::Matrix& proj
			) override {}

	};

}
