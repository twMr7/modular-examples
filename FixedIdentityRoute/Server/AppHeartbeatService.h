#pragma once
#include <string>
#include <vector>
#include <Poco/Util/Application.h>
#include <Poco/Util/OptionSet.h>
#include <Poco/Event.h>
#include <Poco/NotificationQueue.h>

class AppHeartbeatService : public Poco::Util::Application
{
private:
	// for handling Ctrl+C and terminate request
	static Poco::Event _eventTerminated;
	static BOOL __stdcall ConsoleCtrlHandler(DWORD ctrlType);
	// for the help request by user
	bool _helpRequested{ false };
	void handleHelp(const std::string& name, const std::string& value);
	// for events handle by state machine
	static Poco::NotificationQueue _eventQueue;
	std::vector<std::string> getClientList();

protected:
	void initialize(Poco::Util::Application& self);
	void uninitialize();
	void defineOptions(Poco::Util::OptionSet& options);
	int main(const ArgVec& args);

public:
	AppHeartbeatService() {};
	bool helpRequested();
	static void terminate();
};

