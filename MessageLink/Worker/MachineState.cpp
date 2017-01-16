#include "Poco/Util/Application.h"
#include "Poco/NObserver.h"
#include "MachineState.h"
#include "DioPollingTask.h"
#include "ServoMotionTask.h"
#include "MqTask.h"

using Poco::Util::Application;
using Poco::Logger;
using Poco::TaskManager;
using Poco::AutoPtr;
using Poco::Notification;
using Poco::NotificationQueue;
using Poco::NObserver;

MachineState::MachineState(TaskManager & taskmgr, NotificationQueue & queue)
	: _currentState(new IdleState)
	, _nextStateInfo{ {"type", (int8_t)StateType::StayAsWere} }
	, _logger(Logger::get("MachineState"))
	, _taskmanager(taskmgr)
	, _queue(queue)
{
}

void MachineState::start()
{
	_taskmanager.addObserver(NObserver<MachineState, Event_StartMotor>(*this, &MachineState::onStartMotor));
	_taskmanager.addObserver(NObserver<MachineState, Event_StopMotor>(*this, &MachineState::onStopMotor));
	_taskmanager.start(new MqTask);
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
			_nextStateInfo = _currentState->handleEvent(*this, pNotify);
			if (_nextStateInfo["type"].convert<int8_t>() != ((int8_t)StateType::StayAsWere))
				transitState();
		}
		else
			break;
	}
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
	StateType nextState = (StateType)_nextStateInfo["type"].convert<int8_t>();
	if (nextState == StateType::StayAsWere)
		return;

	// initialze next state
	switch (nextState)
	{
		case StateType::MotorMoving:
		{
			int32_t speed = _nextStateInfo["speed"];
			_currentState = std::unique_ptr<MotorMovingState>(new MotorMovingState(speed));
			break;
		}

		case StateType::Idle:
		default:
		{
			_currentState = std::unique_ptr<IdleState>(new IdleState);
			break;
		}
	}

	_currentState->enter(*this);
}

/**********************************************************************************
 * Notification "Events" from TaskManager
 * dispatch these events to MachineState by direct forwarding
 **********************************************************************************/
void MachineState::onStartMotor(const Poco::AutoPtr<Event_StartMotor>& pNotify)
{
	_queue.enqueueNotification(pNotify);
}

void MachineState::onStopMotor(const Poco::AutoPtr<Event_StopMotor>& pNotify)
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
StateInfo IdleState::handleEvent(MachineState & machine, const AutoPtr<Notification> & pNotify)
{
	auto pOnStartMotor = pNotify.cast<Event_StartMotor>();
	StateInfo stanfo;
	if (pOnStartMotor)
	{
		stanfo["type"] = (int8_t)StateType::MotorMoving;
		stanfo["speed"] = pOnStartMotor->Speed();
	}
	else
	{
		stanfo["type"] = (int8_t)StateType::StayAsWere;
	}
	return stanfo;
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

StateInfo MotorMovingState::handleEvent(MachineState & machine, const AutoPtr<Notification> & pNotify)
{
	auto pOnStopMotor = pNotify.cast<Event_StopMotor>();
	StateInfo stanfo;
	if (pOnStopMotor)
		stanfo["type"] = (int8_t)StateType::Idle;
	else
		stanfo["type"] = (int8_t)StateType::StayAsWere;
	return stanfo;
}

void MotorMovingState::enter(MachineState & machine)
{
	poco_information(machine.logger(), "kick off motor task -> MotorMovingState");
	machine.taskmanager().addObserver(NObserver<MachineState, Event_MotorFeedback>(machine, &MachineState::onMotorFeedback));
	machine.taskmanager().start(new ServoMotionTask(_speed));
}
