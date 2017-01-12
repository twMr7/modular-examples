#include "Poco/Util/Application.h"
#include "Poco/NObserver.h"
#include "MachineState.h"
#include "DioPollingTask.h"
#include "ServoMotionTask.h"

using Poco::Util::Application;
using Poco::Logger;
using Poco::TaskManager;
using Poco::AutoPtr;
using Poco::Notification;
using Poco::NotificationQueue;
using Poco::NObserver;

MachineState::MachineState(TaskManager & taskmgr, NotificationQueue & queue)
	: _currentState(new IdleState)
	, _nextState(StateType::StayAsWere)
	, _logger(Logger::get("MachineState"))
	, _taskmanager(taskmgr)
	, _queue(queue)
{
}

void MachineState::start()
{
	_taskmanager.addObserver(NObserver<MachineState, Event_Sensor1Changed>(*this, &MachineState::onSensor1Changed));
	_taskmanager.addObserver(NObserver<MachineState, Event_Sensor2Changed>(*this, &MachineState::onSensor2Changed));
	_taskmanager.start(new DioPollingTask);

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
}

void MachineState::handleEvent(const AutoPtr<Notification>& pNotify)
{
	_currentState->handleEvent(*this, pNotify);
}

void MachineState::setNext(StateType type)
{
	_nextState = type;
}

Logger & MachineState::logger() const
{
	return _logger;
}

TaskManager & MachineState::taskmanager() const
{
	return _taskmanager;
}

void MachineState::transitState()
{
	if (_nextState == StateType::StayAsWere)
		return;

	// finalize current state
	delete _currentState;

	// initialze next state
	switch (_nextState)
	{
		case StateType::MotorMoving:
			_currentState = new MotorMovingState;
			break;

		case StateType::Idle:
		default:
			_currentState = new IdleState;
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
	_queue.enqueueNotification(pNotify);
}

void MachineState::onSensor2Changed(const AutoPtr<Event_Sensor2Changed> & pNotify)
{
	_queue.enqueueNotification(pNotify);
}

void MachineState::onMotorFeedback(const AutoPtr<Event_MotorFeedback> & pNotify)
{
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
	TaskManager::TaskList taskList = machine.taskmanager().taskList();
	for (auto& task : taskList)
	{
		if (task->name() == "ServoMotionTask")
		{
			task->cancel();
		}
	}
	machine.taskmanager().removeObserver(NObserver<MachineState, Event_MotorFeedback>(machine, &MachineState::onMotorFeedback));
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
	machine.taskmanager().addObserver(NObserver<MachineState, Event_MotorFeedback>(machine, &MachineState::onMotorFeedback));
	machine.taskmanager().start(new ServoMotionTask);
}
