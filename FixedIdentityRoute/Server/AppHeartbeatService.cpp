#include <iostream>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <Poco/Util/Option.h>
#include <Poco/Util/HelpFormatter.h>
#include <Poco/ErrorHandler.h>
#include <Poco/AutoPtr.h>
#include <Poco/AsyncChannel.h>
#include <Poco/ConsoleChannel.h>
#include <Poco/TaskManager.h>

#include "AppHeartbeatService.h"
#include "ServerState.h"

using std::string;
using std::vector;
using std::stringstream;
using std::istream_iterator;
using Poco::Util::Application;
using Poco::Util::Option;
using Poco::Util::OptionSet;
using Poco::Util::OptionCallback;
using Poco::Util::HelpFormatter;

class TaskErrorHandler : public Poco::ErrorHandler
{
public:
	void exception(const Poco::Exception& exp)
	{
		std::cerr << "Unhandled task exception: " <<  exp.displayText() << std::endl;
	}

	void exception(const std::exception& exp)
	{
		std::cerr << "Unhandled task exception: " << exp.what() << std::endl;
	}

	void exception()
	{
		std::cerr << "unHandled task exception: unknown exception" << std::endl;
	}
};

// static members initialize
Poco::Event AppHeartbeatService::_eventTerminated;
Poco::NotificationQueue AppHeartbeatService::_eventQueue;

void AppHeartbeatService::handleHelp(const std::string & name, const std::string & value)
{
	_helpRequested = true;
	// display help
	HelpFormatter helpFormatter(options());
	helpFormatter.setCommand(commandName());
	helpFormatter.setUsage("Options");
	helpFormatter.setHeader("A simple service to send heartbeat message through fixed identity route.\nAvailable Options:");
	helpFormatter.format(std::cout);
	// stop further processing
	stopOptionsProcessing();
}

vector<string> AppHeartbeatService::getClientList()
{
	stringstream list(config().getString("application.clients", ""));
	vector<string> listClient;
	std::move(istream_iterator<string>(list), istream_iterator<string>(), back_inserter(listClient));
	return listClient;
	// another way to do the same
	//return vector<string>(istream_iterator<string>(list), istream_iterator<string>());
}

BOOL AppHeartbeatService::ConsoleCtrlHandler(DWORD ctrlType)
{
	switch (ctrlType)
	{
	case CTRL_C_EVENT:
	case CTRL_CLOSE_EVENT:
	case CTRL_BREAK_EVENT:
		terminate();
		return _eventTerminated.tryWait(10000) ? TRUE : FALSE;
	default:
		return FALSE;
	}
}

void AppHeartbeatService::initialize(Application & self)
{
	poco_information(logger(), config().getString("application.name", name()) + " initialize");
	// load default configuration file
	loadConfiguration();
	// all registered subsystems are initialized in ancestor's initialize procedure
	Application::initialize(self);
	// catch the termination request
	SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);
}

void AppHeartbeatService::uninitialize()
{
	poco_information(logger(), config().getString("application.name", name()) + " uninitialize");
	// to avoid unpredictable result cause by AsyncChannel, change the channel to ConsoleChannel explicitly
	if (dynamic_cast<Poco::AsyncChannel*>(logger().getChannel()))
	{
		Poco::AutoPtr<Poco::ConsoleChannel> pCC = new Poco::ConsoleChannel;
		logger().setChannel("", pCC);
	}

	// ancestor uninitialization
	Application::uninitialize();
}

void AppHeartbeatService::defineOptions(Poco::Util::OptionSet & options)
{
	Application::defineOptions(options);

	options.addOption(
		Option("help", "h", "display help information on command line arguments")
		.required(false)
		.repeatable(false)
		.callback(OptionCallback<AppHeartbeatService>(this, &AppHeartbeatService::handleHelp)));
}

int AppHeartbeatService::main(const ArgVec & args)
{
	if (!_helpRequested)
	{
		// install the unhandled error catcher for threads
		TaskErrorHandler newEH;
		Poco::ErrorHandler* pOldEH = Poco::ErrorHandler::set(&newEH);

		Poco::TaskManager taskManager;
		ServerState serverState(taskManager, _eventQueue, getClientList());
		serverState.start();

		_eventTerminated.set();

		taskManager.cancelAll();
		taskManager.joinAll();

		Poco::ErrorHandler::set(pOldEH);
	}
	return Application::EXIT_OK;
}

bool AppHeartbeatService::helpRequested()
{
	return _helpRequested;
}

void AppHeartbeatService::terminate()
{
	_eventQueue.enqueueUrgentNotification(new Event_TerminateRequest);
}
