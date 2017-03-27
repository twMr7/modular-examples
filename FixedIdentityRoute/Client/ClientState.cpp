#include <string>
#include <Poco/Util/Application.h>
#include <Poco/TaskManager.h>
#include <Poco/NotificationQueue.h>
#include <Poco/NObserver.h>
#include "ClientState.h"
#include "TaskHeartbeat.h"

using std::string;
using Poco::Util::Application;
using Poco::Logger;
using Poco::TaskManager;
using Poco::AutoPtr;
using Poco::Notification;
using Poco::NotificationQueue;
using Poco::NObserver;

ClientState::ClientState(string & id, TaskManager & taskmgr, NotificationQueue & queue)
	: _identity(id)
	, _currentState(new StartupState)
	, _nextStateInfo{ {"type", (int8_t)StateType::StayAsWere} }
	, _logger(Logger::get(id))
	, _taskManager(taskmgr)
	, _stateQueue(queue)
{
}

void ClientState::start()
{
	_taskManager.addObserver(NObserver<ClientState, Event_ServerLinkUp>(*this, &ClientState::onServerLinkUp));
	_taskManager.start(new TaskHeartbeat(_identity));

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

void ClientState::transitState()
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

Logger & ClientState::logger() const
{
	return _logger;
}

TaskManager & ClientState::taskManager() const
{
	return _taskManager;
}

/**********************************************************************************
 * Notification "Events" from TaskManager
 **********************************************************************************/
void ClientState::onServerLinkUp(const Poco::AutoPtr<Event_ServerLinkUp>& pNotify)
{
	if (_currentState->type() == StateType::Startup)
		_stateQueue.enqueueNotification(pNotify);
}

/**********************************************************************************
 * State Patterns for ClientState
 **********************************************************************************/
StateInfo StartupState::handleEvent(ClientState & machine, const Poco::AutoPtr<Poco::Notification>& pNotify)
{
	StateInfo stanfo;
	if (auto pevent = pNotify.cast<Event_ServerLinkUp>())
		stanfo["type"] = (int8_t)StateType::Online;
	else
		stanfo["type"] = (int8_t)StateType::StayAsWere;

	return stanfo;
}

void StartupState::enter(ClientState & machine)
{
	poco_information(machine.logger(), "enter startup state");
}

StateInfo OnlineState::handleEvent(ClientState & machine, const Poco::AutoPtr<Poco::Notification>& pNotify)
{
	StateInfo stanfo;
	stanfo["type"] = (int8_t)StateType::StayAsWere;
	return stanfo;
}

void OnlineState::enter(ClientState & machine)
{
	poco_information(machine.logger(), "enter online state");
}
