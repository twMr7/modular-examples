#pragma once

#include "Poco/Util/Application.h"
#include "Poco/Util/OptionSet.h"
#include "Poco/Event.h"
#include "Poco/NotificationQueue.h"

class StateController : public Poco::Util::Application
{
private:
	static Poco::Event _eventTerminated;
	static Poco::NotificationQueue _eventQueue;
	bool _helpRequested{ false };

	void handleHelp(const std::string& name, const std::string& value);
	static BOOL __stdcall ConsoleCtrlHandler(DWORD ctrlType);

protected:
	void initialize(Poco::Util::Application& self);
	void uninitialize();
	void defineOptions(Poco::Util::OptionSet& options);
	int main(const ArgVec& args);

public:
	StateController() {};
	bool helpRequested();
	static void terminate();
};

