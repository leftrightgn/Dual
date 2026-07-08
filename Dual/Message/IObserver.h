#pragma once
#include <Message/Message.h>

class IObserver
{
public:

	virtual void OnMessageAccepted(Message::MessageID messageID) = 0;
};
