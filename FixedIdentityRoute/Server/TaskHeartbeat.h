#pragma once
#include <unordered_map>
#include <vector>
#include <Poco/Task.h>
#include <Poco/Logger.h>

typedef std::unordered_map<std::string, int8_t> ClientHeartbeatState;

class TaskHeartbeat : public Poco::Task
{
private:
	Poco::Logger& _logger;
	std::vector<std::string> _clientid;
	ClientHeartbeatState _clientHeart;

public:
	TaskHeartbeat(std::vector<std::string>& clientlist);
	void runTask();
};

