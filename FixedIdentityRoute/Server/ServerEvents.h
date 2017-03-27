#pragma once
#include <Poco/Notification.h>

class Event_TerminateRequest : public Poco::Notification
{
public:
	Event_TerminateRequest() {}
};

class Event_ClientLinkUp : public Poco::Notification
{
public:
	Event_ClientLinkUp(std::string id) : _id(id) {}
	std::string identity() const { return _id; }

private:
	std::string _id;
};

class Event_ClientLinkDown : public Poco::Notification
{
public:
	Event_ClientLinkDown(std::string id) : _id(id) {}
	std::string identity() const { return _id; }

private:
	std::string _id;
};
