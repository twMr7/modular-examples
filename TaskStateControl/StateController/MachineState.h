#pragma once
#include <memory>
#include <Poco/TaskManager.h>
#include <Poco/Logger.h>
#include <Poco/AutoPtr.h>
#include <Poco/NotificationQueue.h>

enum class StateType
{
	// concrete machine states
	Idle,
	MotorMoving,
	// special type of states, no class is defined for these
	StayAsWere
};

class Event_Sensor1Changed : public Poco::Notification
{
public:
	Event_Sensor1Changed(bool value) : _value(value) {}
	bool isON() const { return _value; }

private:
	bool _value;
};

class Event_Sensor2Changed : public Poco::Notification
{
public:
	Event_Sensor2Changed(bool value) : _value(value) {}
	bool isON() const { return _value; }

private:
	bool _value;
};

class Event_MotorFeedback : public Poco::Notification
{
public:
	Event_MotorFeedback(int32_t pos) : _position(pos) {}
	int32_t Position() const { return _position; }

private:
	int32_t _position;
};

class Event_TerminateRequest : public Poco::Notification
{
public:
	Event_TerminateRequest() {}
};

// forward declaration for State class
class State;

// the machine context of the example
// MachineState manages all events and states of this machine
class MachineState
{
private:
	std::unique_ptr<State> _currentState;
	StateType _nextState;
	Poco::Logger& _logger;
	Poco::TaskManager _taskmanager;
	Poco::NotificationQueue& _queue;

protected:
	// state transition
	void transitState();

public:
	MachineState(Poco::NotificationQueue& queue);
	~MachineState();
	Poco::Logger& logger() const;
	Poco::TaskManager& taskmanager();

	// start looping and wait for events
	void start();
	void startServoMotion();
	void stopServoMotion();

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
