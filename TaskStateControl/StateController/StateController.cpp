#include <iostream>

#include "Poco/Util/Option.h"
#include "Poco/Util/HelpFormatter.h"
#include "Poco/Process.h"
#include "Poco/ErrorHandler.h"
#include "Poco/AutoPtr.h"
#include "Poco/AsyncChannel.h"
#include "Poco/ConsoleChannel.h"
#include "Poco/Task.h"
#include "Poco/TaskManager.h"
#include "Poco/NObserver.h"

#include "StateController.h"
#include "DioPollingTask.h"
#include "ServoMotionTask.h"

using Poco::Util::Application;
using Poco::Util::Option;
using Poco::Util::OptionSet;
using Poco::Util::OptionCallback;
using Poco::Util::HelpFormatter;
using Poco::Logger;
using Poco::NamedEvent;
using Poco::ErrorHandler;
using Poco::AutoPtr;
using Poco::Task;
using Poco::TaskManager;
using Poco::NObserver;

// NOTE: for a real world application, a context class is required to complete the state pattern
class StateChangeHandler
{
public:
	StateChangeHandler(Poco::TaskManager& taskmgr) :
		_logger(Application::instance().logger()),
		_manager(taskmgr)
	{
	}

	void onSensor1Changed(const AutoPtr<Sensor1Changed>& pNotify)
	{
		std::string state = (pNotify->isON()) ? "ON" : "OFF";
		poco_information(_logger, "Sensor #1 is " + state + " -> Kick off motor task");
		// reacting to this change by kicking off another task
		_manager.addObserver(
			NObserver<StateChangeHandler, MotorFeedback>
			(*this, &StateChangeHandler::onMotorFeedback)
		);
		_manager.start(new ServoMotionTask);
	}

	void onSensor2Changed(const AutoPtr<Sensor2Changed>& pNotify)
	{
		std::string state = (pNotify->isON()) ? "ON" : "OFF";
		poco_information(_logger, "Sensor #2 is " + state + " -> stop motor task");
		TaskManager::TaskList taskList = _manager.taskList();
		for (auto& task : taskList)
		{
			if (task->name() == "ServoMotionTask")
			{
				task->cancel();
			}
		}
		_manager.removeObserver(
			NObserver<StateChangeHandler, MotorFeedback>
			(*this, &StateChangeHandler::onMotorFeedback)
		);
	}

	void onMotorFeedback(const AutoPtr<MotorFeedback>& pNotify)
	{
		poco_information(_logger, "Motor feedback position = " + std::to_string(pNotify->Position()));
	}

private:
	Logger& _logger;
	TaskManager& _manager;
};

class TaskErrorHandler : public ErrorHandler
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

Poco::NamedEvent StateController::_eventTerminate(Poco::ProcessImpl::terminationEventName(Poco::Process::id()));
Poco::Event StateController::_eventTerminated;

void StateController::handleHelp(const std::string & name, const std::string & value)
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

BOOL StateController::ConsoleCtrlHandler(DWORD ctrlType)
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

void StateController::initialize(Application & self)
{
	poco_information(logger(), config().getString("application.name", name()) + " initialize");
	// load default configuration file
	loadConfiguration();
	// all registered subsystems are initialized in ancestor's initialize procedure
	Application::initialize(self);
}

void StateController::uninitialize()
{
	poco_information(logger(), config().getString("application.name", name()) + " uninitialize");
	// to avoid unpredictable result cause by AsyncChannel, change the channel to ConsoleChannel explicitly
	if (dynamic_cast<Poco::AsyncChannel*>(logger().getChannel()))
	{
		AutoPtr<Poco::ConsoleChannel> pCC = new Poco::ConsoleChannel;
		logger().setChannel("", pCC);
	}

	// ancestor uninitialization
	Application::uninitialize();
}

void StateController::defineOptions(Poco::Util::OptionSet & options)
{
	Application::defineOptions(options);

	options.addOption(
		Option("help", "h", "display help information on command line arguments")
		.required(false)
		.repeatable(false)
		.callback(OptionCallback<StateController>(this, &StateController::handleHelp)));
}

int StateController::main(const ArgVec & args)
{
	if (!_helpRequested)
	{
		// install the last line error catcher
		TaskErrorHandler newEH;
		ErrorHandler* pOldEH = ErrorHandler::set(&newEH);

		TaskManager taskmgr;
		StateChangeHandler hStateChange(taskmgr);
		taskmgr.addObserver(
			NObserver<StateChangeHandler, Sensor1Changed>
			(hStateChange, &StateChangeHandler::onSensor1Changed)
		);
		taskmgr.addObserver(
			NObserver<StateChangeHandler, Sensor2Changed>
			(hStateChange, &StateChangeHandler::onSensor2Changed)
		);
		taskmgr.start(new DioPollingTask);

		waitForTerminationRequest();

		taskmgr.cancelAll();
		taskmgr.joinAll();

		ErrorHandler::set(pOldEH);
	}
	return Application::EXIT_OK;
}

void StateController::waitForTerminationRequest()
{
	SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);
	_eventTerminate.wait();
	_eventTerminated.set();
}

bool StateController::helpRequested()
{
	return _helpRequested;
}

void StateController::terminate()
{
	_eventTerminate.set();
}
