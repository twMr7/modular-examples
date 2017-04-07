#pragma once
#include <Poco/Task.h>
#include <Poco/Logger.h>

class TaskServoMotion : public Poco::Task
{
private:
	Poco::Logger& _logger;
public:
	TaskServoMotion();
	void runTask();
};

