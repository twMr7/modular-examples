#pragma once
#include "Poco/Task.h"
#include "Poco/Notification.h"

class ServoMotionTask : public Poco::Task
{
public:
	ServoMotionTask();
	void runTask();
};

class MotorFeedback : public Poco::Notification
{
public:
	MotorFeedback(int32_t pos) : _position(pos) {}
	int32_t Position() const { return _position; }

private:
	int32_t _position;
};
