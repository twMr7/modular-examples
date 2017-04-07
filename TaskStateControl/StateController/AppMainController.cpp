#include <iostream>
#include <Poco/Util/Option.h>
#include <Poco/Util/HelpFormatter.h>
#include <Poco/ErrorHandler.h>
#include <Poco/AutoPtr.h>
#include <Poco/AsyncChannel.h>
#include <Poco/ConsoleChannel.h>
#include "AppMainController.h"
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
Poco::Event AppMainController::_eventTerminated;
Poco::NotificationQueue AppMainController::_eventQueue;

void AppMainController::handleHelp(const std::string & name, const std::string & value)
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

BOOL AppMainController::ConsoleCtrlHandler(DWORD ctrlType)
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

void AppMainController::initialize(Application & self)
{
	poco_information(logger(), config().getString("application.baseName", name()) + " initialize");
	// load default configuration file
	loadConfiguration();
	// all registered subsystems are initialized in ancestor's initialize procedure
	Application::initialize(self);
	// catch the termination request
	SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);
}

void AppMainController::uninitialize()
{
	poco_information(logger(), config().getString("application.baseName", name()) + " uninitialize");
	
	// ancestor uninitialization
	Application::uninitialize();
}

void AppMainController::defineOptions(Poco::Util::OptionSet & options)
{
	Application::defineOptions(options);

	options.addOption(
		Option("help", "h", "display help information on command line arguments")
		.required(false)
		.repeatable(false)
		.callback(OptionCallback<AppMainController>(this, &AppMainController::handleHelp)));
}

int AppMainController::main(const ArgVec & args)
{
	if (!_helpRequested)
	{
		// install the last line error catcher
		TaskErrorHandler newEH;
		Poco::ErrorHandler* pOldEH = Poco::ErrorHandler::set(&newEH);

		// start state machine, loop until Event_TerminateRequest
		MachineState machineState(_eventQueue);
		machineState.start();
		// exit the state machine loop

		_eventTerminated.set();

		// Note: Close the AsyncChannel before taskManager joinAll() get called.
		//       otherwise, default thread pool can be spin-locked on waiting to join. 
		Poco::AsyncChannel* pAsyncChannel = dynamic_cast<Poco::AsyncChannel*>(logger().getChannel());
		if (pAsyncChannel)
		{
			pAsyncChannel->close();
			Poco::AutoPtr<Poco::ConsoleChannel> pCC = new Poco::ConsoleChannel;
			logger().setChannel("", pCC);
		}

		// put back the original error handler
		Poco::ErrorHandler::set(pOldEH);
	}
	return Application::EXIT_OK;
}

bool AppMainController::helpRequested()
{
	return _helpRequested;
}

void AppMainController::terminate()
{
	_eventQueue.enqueueUrgentNotification(new Event_TerminateRequest);
}
