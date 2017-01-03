#pragma once
#include "Poco/Util/Application.h"
#include "Poco/Util/OptionSet.h"
#include "Poco/Event.h"
#include "Poco/NamedEvent.h"

class StateController : public Poco::Util::Application
{
private:
	static Poco::NamedEvent _eventTerminate;
	static Poco::Event _eventTerminated;
	bool _helpRequested{ false };

	void handleHelp(const std::string& name, const std::string& value);
	static BOOL __stdcall ConsoleCtrlHandler(DWORD ctrlType);

protected:
	void initialize(Poco::Util::Application& self);
	void uninitialize();
	void defineOptions(Poco::Util::OptionSet& options);
	int main(const ArgVec& args);
	void waitForTerminationRequest();

public:
	StateController() {};
	bool helpRequested();
	static void terminate();
};

