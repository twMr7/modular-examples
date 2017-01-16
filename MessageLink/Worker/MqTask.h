#pragma once
#include "Poco/Task.h"
#include "Poco/Logger.h"

class MqTask : public Poco::Task
{
private:
	Poco::Logger& _logger;

public:
	MqTask();
	void runTask();
};

