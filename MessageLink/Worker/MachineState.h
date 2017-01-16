#pragma once

#include <memory>
#include "Poco/TaskManager.h"
#include "Poco/Logger.h"
#include "Poco/AutoPtr.h"
#include "Poco/NotificationQueue.h"
#include "Poco/DynamicAny.h"

#include "MachineEvents.h"

enum class StateType : int8_t
{
	// concrete machine states
	Idle,
	MotorMoving,
	// special type of states, no class is defined for these
	StayAsWere
};

// forward declaration for State class
class State;
typedef std::map<std::string, Poco::DynamicAny> StateInfo;

// the machine context of the example
// MachineState manages all events and states of this machine
class MachineState
{
private:
	std::unique_ptr<State> _currentState;
	StateInfo _nextStateInfo;
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
	Poco::Logger& logger() const;
	Poco::TaskManager& taskmanager() const;

	// event observers
	void onStartMotor(const Poco::AutoPtr<Event_StartMotor>& pNotify);
	void onStopMotor(const Poco::AutoPtr<Event_StopMotor>& pNotify);
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
	virtual StateInfo handleEvent(MachineState& machine, const Poco::AutoPtr<Poco::Notification>& pNotify) = 0;
	virtual void enter(MachineState& machine) = 0;
};

// concrete state classes corresponding to StateType
class IdleState : public State
{
public:
	IdleState() : State(StateType::Idle) {}
	StateInfo handleEvent(MachineState& machine, const Poco::AutoPtr<Poco::Notification>& pNotify);
	void enter(MachineState& machine);
};

class MotorMovingState : public State
{
private:
	int32_t _speed;
public:
	MotorMovingState(int32_t speed) : State(StateType::MotorMoving), _speed(speed) {}
	StateInfo handleEvent(MachineState& machine, const Poco::AutoPtr<Poco::Notification>& pNotify);
	void enter(MachineState& machine);
};

