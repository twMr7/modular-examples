#include <string>
#include <vector>
#include <utility>
#include <Poco/Util/Application.h>
#include <Poco/TaskManager.h>
#include <Poco/NotificationQueue.h>
#include <Poco/NObserver.h>
#include "ServerState.h"
#include "TaskHeartbeat.h"

using std::string;
using std::vector;
using Poco::Util::Application;
using Poco::Logger;
using Poco::TaskManager;
using Poco::AutoPtr;
using Poco::Notification;
using Poco::NotificationQueue;
using Poco::NObserver;

ServerState::ServerState(TaskManager & taskmgr, NotificationQueue & queue, vector<string> list)
	: _currentState(new StartupState)
	, _nextStateInfo{ {"type", (int8_t)StateType::StayAsWere} }
	, _logger(Logger::get("ServerState"))
	, _taskManager(taskmgr)
	, _stateQueue(queue)
	, _clientList(std::move(list))
{
	for (const auto& id : _clientList)
	{
		poco_information(_logger, "add client: " + id);
		_clientLink[id] = false;
	}
}

void ServerState::start()
{
	_taskManager.addObserver(NObserver<ServerState, Event_ClientLinkUp>(*this, &ServerState::onClientLinkUp));
	_taskManager.addObserver(NObserver<ServerState, Event_ClientLinkDown>(*this, &ServerState::onClientLinkDown));
	_taskManager.start(new TaskHeartbeat(_clientList));

	for (;;)
	{
		Notification::Ptr pNotify(_stateQueue.waitDequeueNotification());
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

void ServerState::transitState()
{
	StateType nextState = (StateType)_nextStateInfo["type"].convert<int8_t>();
	if (nextState == StateType::StayAsWere)
		return;

	// initialze next state
	switch (nextState)
	{
	case StateType::Startup:
		_currentState = std::unique_ptr<StartupState>(new StartupState);
		break;

	case StateType::Online:
		_currentState = std::unique_ptr<OnlineState>(new OnlineState);
		break;

	default:
		poco_debug(_logger, "Invalid state: " + std::to_string((uint8_t)nextState));
		return;
	}

	_currentState->enter(*this);
}

Logger & ServerState::logger() const
{
	return _logger;
}

TaskManager & ServerState::taskManager() const
{
	return _taskManager;
}

/**********************************************************************************
 * Notification "Events" from TaskManager
 **********************************************************************************/
void ServerState::onClientLinkUp(const Poco::AutoPtr<Event_ClientLinkUp>& pNotify)
{
	// the valid identity should already be checked in protocol
	_clientLink[pNotify->identity()] = true;

	// check if all client are linked up
	bool allUp = std::all_of(_clientLink.cbegin(), _clientLink.cend(),
		[](const ClientLinkState::value_type & element) {
			return element.second;
		});

	if (_currentState->type() == StateType::Startup && allUp)
		_stateQueue.enqueueNotification(pNotify);
}

void ServerState::onClientLinkDown(const Poco::AutoPtr<Event_ClientLinkDown>& pNotify)
{
	// the valid identity should already be checked in protocol
	_clientLink[pNotify->identity()] = false;

	if (_currentState->type() == StateType::Online)
		_stateQueue.enqueueNotification(pNotify);
}

/**********************************************************************************
 * State Patterns for ServerState
 **********************************************************************************/
StateInfo StartupState::handleEvent(ServerState & machine, const Poco::AutoPtr<Poco::Notification>& pNotify)
{
	StateInfo stanfo;
	if (auto pevent = pNotify.cast<Event_ClientLinkUp>())
		stanfo["type"] = (int8_t)StateType::Online;
	else
		stanfo["type"] = (int8_t)StateType::StayAsWere;

	return stanfo;
}

void StartupState::enter(ServerState & machine)
{
	poco_information(machine.logger(), "enter startup state");
}

StateInfo OnlineState::handleEvent(ServerState & machine, const Poco::AutoPtr<Poco::Notification>& pNotify)
{
	StateInfo stanfo;
	if (auto pevent = pNotify.cast<Event_ClientLinkDown>())
		stanfo["type"] = (int8_t)StateType::Startup;
	else
		stanfo["type"] = (int8_t)StateType::StayAsWere;
	return stanfo;
}

void OnlineState::enter(ServerState & machine)
{
	poco_information(machine.logger(), "enter online state");
}
