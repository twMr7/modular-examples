#pragma once
#include <string>
#include <Poco/Task.h>
#include <Poco/Logger.h>

class TaskHeartbeat : public Poco::Task
{
private:
	Poco::Logger& _logger;
	std::string _identity;

public:
	TaskHeartbeat(std::string& id);
	void runTask();
};

