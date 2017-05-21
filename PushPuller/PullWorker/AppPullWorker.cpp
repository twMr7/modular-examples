#include <iostream>
#include <string>
#include <Poco/Util/Option.h>
#include <Poco/Util/HelpFormatter.h>
#include <Poco/ErrorHandler.h>
#include <Poco/AutoPtr.h>
#include <Poco/AsyncChannel.h>
#include <Poco/ConsoleChannel.h>
#include <Poco/TaskManager.h>
#include <Poco/Notification.h>
#include <Poco/NotificationQueue.h>
#include "Poco/Util/AbstractConfiguration.h"
#include "AppPullWorker.h"
#include "TaskPull.hpp"

using std::string;
using Poco::Util::Application;
using Poco::Util::Option;
using Poco::Util::OptionSet;
using Poco::Util::OptionCallback;
using Poco::Util::HelpFormatter;
using Poco::Util::AbstractConfiguration;
using Poco::TaskManager;
using Poco::Notification;

#define DEFAULT_PULLFROM_ADDRESS "tcp://127.0.0.1:6866"

class TaskErrorHandler : public Poco::ErrorHandler
{
public:
	void exception(const Poco::Exception& e)
	{
		std::cerr << "Unhandled task exception: " <<  e.displayText() << std::endl;
	}

	void exception(const std::exception& e)
	{
		std::cerr << "Unhandled task exception: " << e.what() << std::endl;
	}

	void exception()
	{
		std::cerr << "unHandled task exception: unknown exception" << std::endl;
	}
};

// static members initialize
Poco::Event AppPullWorker::_eventTerminated;
Poco::NotificationQueue AppPullWorker::_stateQueue;

BOOL AppPullWorker::ConsoleCtrlHandler(DWORD ctrlType)
{
	switch (ctrlType)
	{
	case CTRL_C_EVENT:
	case CTRL_CLOSE_EVENT:
	case CTRL_BREAK_EVENT:
		terminate();
		return _eventTerminated.tryWait(3000) ? TRUE : FALSE;
	default:
		return FALSE;
	}
}

void AppPullWorker::handleHelp(const string & name, const string & value)
{
	_helpRequested = true;
	// display help
	HelpFormatter helpFormatter(options());
	helpFormatter.setCommand(commandName());
	helpFormatter.setUsage("Options");
	helpFormatter.setHeader("Application module to handle digital I/O changes.");
	helpFormatter.format(std::cout);
	// stop further processing
	stopOptionsProcessing();
}

void AppPullWorker::initialize(Application & self)
{
	poco_information(logger(), config().getString("application.baseName", name()) + " initialize");
	// load default configuration file
	loadConfiguration();
	// all registered subsystems are initialized in ancestor's initialize procedure
	Application::initialize(self);
	// catch the termination request
	SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);
}

void AppPullWorker::uninitialize()
{
	poco_information(logger(), config().getString("application.baseName", name()) + " uninitialize");
	// ancestor uninitialization
	Application::uninitialize();
}

void AppPullWorker::defineOptions(Poco::Util::OptionSet & options)
{
	Application::defineOptions(options);

	options.addOption(
		Option("help", "h", "display help information on command line arguments")
		.required(false)
		.repeatable(false)
		.callback(OptionCallback<AppPullWorker>(this, &AppPullWorker::handleHelp)));
}

int AppPullWorker::main(const ArgVec & args)
{
	if (!_helpRequested)
	{

		// install the unhandled error catcher for threads
		TaskErrorHandler newEH;
		Poco::ErrorHandler* pOldEH = Poco::ErrorHandler::set(&newEH);

		string pullfrom = config().getString("application.pull.from", DEFAULT_PULLFROM_ADDRESS);
		TaskManager taskmanager;
		taskmanager.start(new TaskPull(pullfrom));

		for (;;)
		{
			Notification::Ptr pNotify(_stateQueue.waitDequeueNotification());
			if (pNotify)
			{
				// no terminating state, check the event here and exist right away
				if (pNotify.cast<Event_TerminateRequest>())
				{
					poco_information(logger(), "termination request -> exist state loop");
					break;
				}
			}
			else
				break;
		}

		_eventTerminated.set();

		taskmanager.cancelAll();

		// Note: Close the AsyncChannel before taskManager joinAll() get called.
		//       otherwise, default thread pool can be spin-locked on waiting to join. 
		Poco::AsyncChannel* pAsyncChannel = dynamic_cast<Poco::AsyncChannel*>(logger().getChannel());
		if (pAsyncChannel)
		{
			pAsyncChannel->close();
			Poco::AutoPtr<Poco::ConsoleChannel> pCC = new Poco::ConsoleChannel;
			logger().setChannel("", pCC);
		}

		taskmanager.joinAll();

		// put back the original error handler
		Poco::ErrorHandler::set(pOldEH);
	}

	return Application::EXIT_OK;
}

bool AppPullWorker::helpRequested()
{
	return _helpRequested;
}

void AppPullWorker::terminate()
{
	_stateQueue.enqueueUrgentNotification(new Event_TerminateRequest);
}
