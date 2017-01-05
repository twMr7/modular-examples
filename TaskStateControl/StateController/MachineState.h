#pragma once

#include "Poco/TaskManager.h"
#include "Poco/Logger.h"
#include "Poco/AutoPtr.h"
#include "Poco/NotificationQueue.h"

#include "MachineEvents.h"

enum class StateType
{
	// concrete machine states
	Idle,
	MotorMoving,
	// special type of states, no class is defined for these
	StayAsWere
};

// forward declaration for State class
class State;

// the machine context of the example
// MachineState manages all events and states of this machine
class MachineState
{
private:
	State* _currentState;
	StateType _nextState;
	Poco::Logger& _logger;
	Poco::TaskManager& _taskmanager;
	Poco::NotificationQueue& _queue;

protected:
	void transitState();

public:
	MachineState(Poco::TaskManager& taskmgr, Poco::NotificationQueue& queue);

	// start looping and wait for events
	void start();
	// event handler and state transition
	void handleEvent(const Poco::AutoPtr<Poco::Notification>& pNotify);
	void setNext(StateType type);
	Poco::Logger& logger() const;
	Poco::TaskManager& taskmanager() const;

	// event observers
	void onSensor1Changed(const Poco::AutoPtr<Event_Sensor1Changed>& pNotify);
	void onSensor2Changed(const Poco::AutoPtr<Event_Sensor2Changed>& pNotify);
	void onMotorFeedback(const Poco::AutoPtr<Event_MotorFeedback>& pNotify);
};

// abstract base class for all the states defined for this machine
class State
{
private:
	StateType _type;

public:
	State(StateType type) : _type(type) {}
	StateType type() const { return _type; }
	// return the next state to be transitiioned
	virtual StateType handleEvent(MachineState& machine, const Poco::AutoPtr<Poco::Notification>& pNotify) = 0;
	virtual void enter(MachineState& machine) = 0;
};

// concrete state classes corresponding to StateType
class IdleState : public State
{
public:
	IdleState() : State(StateType::Idle) {}
	StateType handleEvent(MachineState& machine, const Poco::AutoPtr<Poco::Notification>& pNotify);
	void enter(MachineState& machine);
};

class MotorMovingState : public State
{
public:
	MotorMovingState() : State(StateType::MotorMoving) {}
	StateType handleEvent(MachineState& machine, const Poco::AutoPtr<Poco::Notification>& pNotify);
	void enter(MachineState& machine);
};
