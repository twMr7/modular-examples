#pragma once
#include "Poco/Task.h"

class ServoMotionTask : public Poco::Task
{
public:
	ServoMotionTask();
	void runTask();
};

