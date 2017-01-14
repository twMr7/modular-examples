#include <iostream>

#include "Poco/Util/Option.h"
#include "Poco/Util/HelpFormatter.h"
#include "Poco/ErrorHandler.h"
#include "Poco/AutoPtr.h"
#include "Poco/AsyncChannel.h"
#include "Poco/ConsoleChannel.h"
#include "Poco/TaskManager.h"

#include "AppWorker.h"
#include "MachineState.h"

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
Poco::Event AppWorker::_eventTerminated;
Poco::NotificationQueue AppWorker::_eventQueue;

void AppWorker::handleHelp(const std::string & name, const std::string & value)
{
	_helpRequested = true;
	// display help
	HelpFormatter helpFormatter(options());
	helpFormatter.setCommand(commandName());
	helpFormatter.setUsage("OPTIONS");
	helpFormatter.setHeader("A sample application simulates a basic state machine responses to I/O changes.");
	helpFormatter.format(std::cout);
	// stop further processing
	stopOptionsProcessing();
}

BOOL AppWorker::ConsoleCtrlHandler(DWORD ctrlType)
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

void AppWorker::initialize(Application & self)
{
	poco_information(logger(), config().getString("application.name", name()) + " initialize");
	// load default configuration file
	loadConfiguration();
	// all registered subsystems are initialized in ancestor's initialize procedure
	Application::initialize(self);
	// catch the termination request
	SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);
}

void AppWorker::uninitialize()
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

void AppWorker::defineOptions(Poco::Util::OptionSet & options)
{
	Application::defineOptions(options);

	options.addOption(
		Option("help", "h", "display help information on command line arguments")
		.required(false)
		.repeatable(false)
		.callback(OptionCallback<AppWorker>(this, &AppWorker::handleHelp)));
}

int AppWorker::main(const ArgVec & args)
{
	if (!_helpRequested)
	{
		// install the last line error catcher
		TaskErrorHandler newEH;
		Poco::ErrorHandler* pOldEH = Poco::ErrorHandler::set(&newEH);

		Poco::TaskManager taskManager;
		MachineState machineState(taskManager, _eventQueue);
		machineState.start();

		_eventTerminated.set();

		taskManager.cancelAll();
		taskManager.joinAll();

		Poco::ErrorHandler::set(pOldEH);
	}
	return Application::EXIT_OK;
}

bool AppWorker::helpRequested()
{
	return _helpRequested;
}

void AppWorker::terminate()
{
	_eventQueue.enqueueUrgentNotification(new Event_TerminateRequest);
}

Poco::NotificationQueue & AppWorker::eventQueue()
{
	return _eventQueue;
}
