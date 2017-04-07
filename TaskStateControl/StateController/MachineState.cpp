#include <Poco/Util/Application.h>
#include <Poco/NObserver.h>
#include <Poco/Thread.h>
#include "MachineState.h"
#include "TaskDioPolling.h"
#include "TaskServoMotion.h"

using Poco::Util::Application;
using Poco::Logger;
using Poco::Thread;
using Poco::TaskManager;
using Poco::AutoPtr;
using Poco::Notification;
using Poco::NotificationQueue;
using Poco::NObserver;

MachineState::MachineState(NotificationQueue & queue)
	: _currentState(new IdleState)
	, _nextState(StateType::StayAsWere)
	, _logger(Logger::get("MachineState"))
	, _queue(queue)
{
}

MachineState::~MachineState()
{
	if (_taskmanager.count() > 0)
		_taskmanager.cancelAll();

	// Note: this destructor is called on exit Application.main() but before Application.uninitialize()
	//       do the best to join all other threads in default thread pool before taskManager.joinAll().
	_taskmanager.joinAll();
}

Logger & MachineState::logger() const
{
	return _logger;
}

TaskManager & MachineState::taskmanager()
{
	return _taskmanager;
}

void MachineState::start()
{
	poco_information(_logger, Poco::format("state machine started Tid=%lu", Thread::currentTid()));
	_taskmanager.addObserver(NObserver<MachineState, Event_Sensor1Changed>(*this, &MachineState::onSensor1Changed));
	_taskmanager.addObserver(NObserver<MachineState, Event_Sensor2Changed>(*this, &MachineState::onSensor2Changed));
	_taskmanager.start(new TaskDioPolling);

	for (;;)
	{
		Notification::Ptr pNotify(_queue.waitDequeueNotification());
		if (pNotify)
		{
			// no terminating state, check the event here and exist right away
			if (pNotify.cast<Event_TerminateRequest>())
			{
				poco_information(_logger, "termination request -> exist state loop");
				break;
			}

			// handle normal operating events
			if ((_nextState = _currentState->handleEvent(*this, pNotify)) != StateType::StayAsWere)
				transitState();
		}
		else
			break;
	}

	_taskmanager.removeObserver(NObserver<MachineState, Event_Sensor1Changed>(*this, &MachineState::onSensor1Changed));
	_taskmanager.removeObserver(NObserver<MachineState, Event_Sensor2Changed>(*this, &MachineState::onSensor2Changed));
	_taskmanager.removeObserver(NObserver<MachineState, Event_MotorFeedback>(*this, &MachineState::onMotorFeedback));
	_taskmanager.cancelAll();
}

void MachineState::startServoMotion()
{
	_taskmanager.addObserver(NObserver<MachineState, Event_MotorFeedback>(*this, &MachineState::onMotorFeedback));
	_taskmanager.start(new TaskServoMotion);
}

void MachineState::stopServoMotion()
{
	TaskManager::TaskList taskList = _taskmanager.taskList();
	for (auto& task : taskList)
	{
		if (task->name() == "TaskServoMotion")
		{
			task->cancel();
		}
	}
	_taskmanager.removeObserver(NObserver<MachineState, Event_MotorFeedback>(*this, &MachineState::onMotorFeedback));
}

void MachineState::transitState()
{
	if (_nextState == StateType::StayAsWere)
		return;

	// initialze next state
	switch (_nextState)
	{
		case StateType::MotorMoving:
			_currentState = std::unique_ptr<MotorMovingState>(new MotorMovingState);
			break;

		case StateType::Idle:
		default:
			_currentState = std::unique_ptr<IdleState>(new IdleState);
			break;
	}

	_currentState->enter(*this);
}

/**********************************************************************************
 * Notification "Events" from TaskManager
 * dispatch these events to MachineState by direct forwarding
 **********************************************************************************/
void MachineState::onSensor1Changed(const AutoPtr<Event_Sensor1Changed> & pNotify)
{
	poco_information(_logger, Poco::format("onSensor1Changed Tid=%lu", Thread::currentTid()));
	_queue.enqueueNotification(pNotify);
}

void MachineState::onSensor2Changed(const AutoPtr<Event_Sensor2Changed> & pNotify)
{
	poco_information(_logger, Poco::format("onSensor2Changed Tid=%lu", Thread::currentTid()));
	_queue.enqueueNotification(pNotify);
}

void MachineState::onMotorFeedback(const AutoPtr<Event_MotorFeedback> & pNotify)
{
	poco_information(_logger, Poco::format("onMotorFeedback Tid=%lu", Thread::currentTid()));
	poco_information(_logger, "Motor feedback position = " + std::to_string(pNotify->Position()));
}

/**********************************************************************************
 * State Patterns for MachineState
 **********************************************************************************/
StateType IdleState::handleEvent(MachineState & machine, const AutoPtr<Notification> & pNotify)
{
	auto pOnSensor1Changed = pNotify.cast<Event_Sensor1Changed>();
	if (pOnSensor1Changed && pOnSensor1Changed->isON())
		return StateType::MotorMoving;
	else
		return StateType::StayAsWere;
}

void IdleState::enter(MachineState & machine)
{
	poco_information(machine.logger(), "stop motor task -> IdleState");
	machine.stopServoMotion();
}

StateType MotorMovingState::handleEvent(MachineState & machine, const AutoPtr<Notification> & pNotify)
{
	auto pOnSensor2Changed = pNotify.cast<Event_Sensor2Changed>();
	if (pOnSensor2Changed && pOnSensor2Changed->isON())
		return StateType::Idle;
	else
		return StateType::StayAsWere;
}

void MotorMovingState::enter(MachineState & machine)
{
	poco_information(machine.logger(), "kick off motor task -> MotorMovingState");
	machine.startServoMotion();
}
