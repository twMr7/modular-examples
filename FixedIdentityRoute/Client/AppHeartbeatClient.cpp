#include <iostream>
#include <string>
#include <Poco/Util/Option.h>
#include <Poco/Util/HelpFormatter.h>
#include <Poco/ErrorHandler.h>
#include <Poco/AutoPtr.h>
#include <Poco/AsyncChannel.h>
#include <Poco/ConsoleChannel.h>
#include <Poco/TaskManager.h>

#include "AppHeartbeatClient.h"
#include "ClientState.h"

using std::string;
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
Poco::Event AppHeartbeatClient::_eventTerminated;
Poco::NotificationQueue AppHeartbeatClient::_eventQueue;

void AppHeartbeatClient::handleHelp(const std::string & name, const std::string & value)
{
	_helpRequested = true;
	// display help
	HelpFormatter helpFormatter(options());
	helpFormatter.setCommand(commandName());
	helpFormatter.setUsage("Options");
	helpFormatter.setHeader("A simple client reacts to heartbeat message from server.\nAvailable Options:");
	helpFormatter.format(std::cout);
	// stop further processing
	stopOptionsProcessing();
}

BOOL AppHeartbeatClient::ConsoleCtrlHandler(DWORD ctrlType)
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

void AppHeartbeatClient::initialize(Application & self)
{
	poco_information(logger(), config().getString("application.baseName", name()) + " initialize");
	// load default configuration file
	loadConfiguration();
	// all registered subsystems are initialized in ancestor's initialize procedure
	Application::initialize(self);
	// catch the termination request
	SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);
}

void AppHeartbeatClient::uninitialize()
{
	poco_information(logger(), config().getString("application.baseName", name()) + " uninitialize");
	// ancestor uninitialization
	Application::uninitialize();
}

void AppHeartbeatClient::defineOptions(Poco::Util::OptionSet & options)
{
	Application::defineOptions(options);

	options.addOption(
		Option("help", "h", "display help information on command line arguments")
		.required(false)
		.repeatable(false)
		.callback(OptionCallback<AppHeartbeatClient>(this, &AppHeartbeatClient::handleHelp)));

	// This option assign argument directly to configuration "application.identity".
	options.addOption(
		Option("id", "i", "assign an alternative identity to this client instead of application.name")
		.required(false)
		.repeatable(false)
		.argument("name")
		.binding("application.identity"));
}

int AppHeartbeatClient::main(const ArgVec & args)
{
	if (!_helpRequested)
	{
		// install the unhandled error catcher for threads
		TaskErrorHandler newEH;
		Poco::ErrorHandler* pOldEH = Poco::ErrorHandler::set(&newEH);

		Poco::TaskManager taskManager;
		string id = config().getString("application.identity", name());
		ClientState clientState(id, taskManager, _eventQueue);
		clientState.start();

		_eventTerminated.set();

		taskManager.cancelAll();

		// Note: Close the AsyncChannel before taskManager joinAll() get called.
		//       otherwise, default thread pool can be spin-locked on waiting to join. 
		Poco::AsyncChannel* pAsyncChannel = dynamic_cast<Poco::AsyncChannel*>(logger().getChannel());
		if (pAsyncChannel)
		{
			pAsyncChannel->close();
			Poco::AutoPtr<Poco::ConsoleChannel> pCC = new Poco::ConsoleChannel;
			logger().setChannel("", pCC);
		}

		taskManager.joinAll();

		Poco::ErrorHandler::set(pOldEH);
	}
	return Application::EXIT_OK;
}

bool AppHeartbeatClient::helpRequested()
{
	return _helpRequested;
}

void AppHeartbeatClient::terminate()
{
	_eventQueue.enqueueUrgentNotification(new Event_TerminateRequest);
}
