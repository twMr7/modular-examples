#pragma once
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <Poco/TaskManager.h>
#include <Poco/Logger.h>
#include <Poco/AutoPtr.h>
#include <Poco/NotificationQueue.h>
#include <Poco/DynamicAny.h>

#include "ClientEvents.h"

// available module states
enum class StateType : uint8_t
{
	Startup,
	Online,	
	// special type of state, no class is defined for this
	StayAsWere
};

// forward declaration for State class
class State;
typedef std::unordered_map<std::string, Poco::DynamicAny> StateInfo;
typedef std::unordered_map<std::string, bool> ClientLinkState;

// the state context of the module
// ClientState manages primary state andd event flow of this module
class ClientState
{
private:
	std::string _identity;
	std::unique_ptr<State> _currentState;
	StateInfo _nextStateInfo;
	Poco::Logger& _logger;
	Poco::TaskManager& _taskManager;
	Poco::NotificationQueue& _stateQueue;

protected:
	void transitState();

public:
	ClientState(std::string& id, Poco::TaskManager& taskmgr, Poco::NotificationQueue& queue);

	// start looping and wait for events
	void start();
	// event handler and state transition
	Poco::Logger& logger() const;
	Poco::TaskManager& taskManager() const;

	// event observers
	void onServerLinkUp(const Poco::AutoPtr<Event_ServerLinkUp>& pNotify);
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
	virtual StateInfo handleEvent(ClientState& machine, const Poco::AutoPtr<Poco::Notification>& pNotify) = 0;
	virtual void enter(ClientState& machine) = 0;
};

/// concrete state classes corresponding to StateType
class StartupState : public State
{
public:
	StartupState() : State(StateType::Startup) {}
	StateInfo handleEvent(ClientState& machine, const Poco::AutoPtr<Poco::Notification>& pNotify);
	void enter(ClientState& machine);
};

class OnlineState : public State
{
public:
	OnlineState() : State(StateType::Online) {}
	StateInfo handleEvent(ClientState& machine, const Poco::AutoPtr<Poco::Notification>& pNotify);
	void enter(ClientState& machine);
};
