#pragma once
#include "Poco/Task.h"

class ServoMotionTask : public Poco::Task
{
private:
	int32_t _speed;
public:
	ServoMotionTask(int32_t speed);
	void runTask();
};

