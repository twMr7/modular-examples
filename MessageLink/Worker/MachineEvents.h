#pragma once
#include <Poco/Notification.h>

class Event_StartMotor : public Poco::Notification
{
private:
	int32_t _speed;
public:
	Event_StartMotor(int32_t speed) : _speed(speed) {}
	int32_t Speed() const { return _speed; }
};

class Event_StopMotor : public Poco::Notification
{
public:
	Event_StopMotor() {}
};

class Event_MotorFeedback : public Poco::Notification
{
public:
	Event_MotorFeedback(int32_t pos) : _position(pos) {}
	int32_t Position() const { return _position; }

private:
	int32_t _position;
};

class Event_TerminateRequest : public Poco::Notification
{
public:
	Event_TerminateRequest() {}
};
