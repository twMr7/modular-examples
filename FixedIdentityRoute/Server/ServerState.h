#pragma once
#include <memory>
#include <vector>
#include <unordered_map>
#include <Poco/TaskManager.h>
#include <Poco/Logger.h>
#include <Poco/AutoPtr.h>
#include <Poco/NotificationQueue.h>
#include <Poco/DynamicAny.h>

#include "ServerEvents.h"

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
// ServerState manages primary state andd event flow of this module
class ServerState
{
private:
	std::unique_ptr<State> _currentState;
	StateInfo _nextStateInfo;
	Poco::Logger& _logger;
	Poco::TaskManager& _taskManager;
	Poco::NotificationQueue& _stateQueue;
	std::vector<std::string> _clientList;
	ClientLinkState _clientLink;

protected:
	void transitState();

public:
	ServerState(Poco::TaskManager& taskmgr, Poco::NotificationQueue& queue, std::vector<std::string> list);

	// start looping and wait for events
	void start();
	// event handler and state transition
	Poco::Logger& logger() const;
	Poco::TaskManager& taskManager() const;

	// event observers
	void onClientLinkUp(const Poco::AutoPtr<Event_ClientLinkUp>& pNotify);
	void onClientLinkDown(const Poco::AutoPtr<Event_ClientLinkDown>& pNotify);
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
	virtual StateInfo handleEvent(ServerState& machine, const Poco::AutoPtr<Poco::Notification>& pNotify) = 0;
	virtual void enter(ServerState& machine) = 0;
};

/// concrete state classes corresponding to StateType
class StartupState : public State
{
public:
	StartupState() : State(StateType::Startup) {}
	StateInfo handleEvent(ServerState& machine, const Poco::AutoPtr<Poco::Notification>& pNotify);
	void enter(ServerState& machine);
};

class OnlineState : public State
{
public:
	OnlineState() : State(StateType::Online) {}
	StateInfo handleEvent(ServerState& machine, const Poco::AutoPtr<Poco::Notification>& pNotify);
	void enter(ServerState& machine);
};
