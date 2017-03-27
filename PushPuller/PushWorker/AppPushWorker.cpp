/*
	File:
		AppPushWorker.cpp
	Description:
		Implementation of the application life cycle and facilities
	Copyright:
		Orisol Asia Ltd.
*/
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
#include "AppPushWorker.h"
#include "TaskPush.hpp"

using std::string;
using Poco::Util::Application;
using Poco::Util::Option;
using Poco::Util::OptionSet;
using Poco::Util::OptionCallback;
using Poco::Util::HelpFormatter;
using Poco::Util::AbstractConfiguration;
using Poco::TaskManager;
using Poco::Notification;

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
Poco::Event AppPushWorker::_eventTerminated;
Poco::NotificationQueue AppPushWorker::_stateQueue;

BOOL AppPushWorker::ConsoleCtrlHandler(DWORD ctrlType)
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

void AppPushWorker::handleHelp(const string & name, const string & value)
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

void AppPushWorker::initialize(Application & self)
{
	poco_information(logger(), config().getString("application.basename", name()) + " initialize");
	// load default configuration file
	loadConfiguration();
	// all registered subsystems are initialized in ancestor's initialize procedure
	Application::initialize(self);
	// catch the termination request
	SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);
}

void AppPushWorker::uninitialize()
{
	poco_information(logger(), config().getString("application.basename", name()) + " uninitialize");
	// to avoid unpredictable result cause by AsyncChannel, change the channel to ConsoleChannel explicitly
	// NOTE: if this cause the undesired console to pop out, modify the ConsoleChannel to NullChannel instead.
	if (dynamic_cast<Poco::AsyncChannel*>(logger().getChannel()))
	{
		Poco::AutoPtr<Poco::ConsoleChannel> pCC = new Poco::ConsoleChannel;
		logger().setChannel("", pCC);
	}

	// ancestor uninitialization
	Application::uninitialize();
}

void AppPushWorker::defineOptions(Poco::Util::OptionSet & options)
{
	Application::defineOptions(options);

	options.addOption(
		Option("help", "h", "display help information on command line arguments")
		.required(false)
		.repeatable(false)
		.callback(OptionCallback<AppPushWorker>(this, &AppPushWorker::handleHelp)));
}

void AppPushWorker::printProperties(const std::string & base)
{
	AbstractConfiguration::Keys keys;
	config().keys(base, keys);
	if (keys.empty())
	{
		if (config().hasProperty(base))
		{
			std::string msg;
			msg.append(base);
			msg.append(" = ");
			msg.append(config().getString(base));
			logger().information(msg);
		}
	}
	else
	{
		for (AbstractConfiguration::Keys::const_iterator it = keys.begin(); it != keys.end(); ++it)
		{
			std::string fullKey = base;
			if (!fullKey.empty()) fullKey += '.';
			fullKey.append(*it);
			printProperties(fullKey);
		}
	}
}

int AppPushWorker::main(const ArgVec & args)
{
	if (!_helpRequested)
	{

		// install the unhandled error catcher for threads
		TaskErrorHandler newEH;
		Poco::ErrorHandler* pOldEH = Poco::ErrorHandler::set(&newEH);

		printProperties("");

		TaskManager taskmanager;
		taskmanager.start(new TaskPush);

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

		// put back the original error handler
		Poco::ErrorHandler::set(pOldEH);
	}

	return Application::EXIT_OK;
}

bool AppPushWorker::helpRequested()
{
	return _helpRequested;
}

void AppPushWorker::terminate()
{
	_stateQueue.enqueueUrgentNotification(new Event_TerminateRequest);
}
