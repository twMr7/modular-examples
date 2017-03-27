#pragma once
#include <Poco/Notification.h>

class Event_TerminateRequest : public Poco::Notification
{
public:
	Event_TerminateRequest() {}
};

class Event_ServerLinkUp : public Poco::Notification
{
public:
	Event_ServerLinkUp() {}
};
