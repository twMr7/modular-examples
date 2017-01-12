#pragma once

#include "Poco/Notification.h"

class Event_Sensor1Changed : public Poco::Notification
{
public:
	Event_Sensor1Changed(bool value) : _value(value) {}
	bool isON() const { return _value; }

private:
	bool _value;
};

class Event_Sensor2Changed : public Poco::Notification
{
public:
	Event_Sensor2Changed(bool value) : _value(value) {}
	bool isON() const { return _value; }

private:
	bool _value;
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
