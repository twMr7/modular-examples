/*
	File:
		AppPushWorker.h
	Description:
		Implementation of the application life cycle and facilities
	Copyright:
		Orisol Asia Ltd.
*/
#pragma once
#include <string>
#include <Poco/Util/Application.h>
#include <Poco/Util/OptionSet.h>
#include <Poco/Event.h>
#include <Poco/Notification.h>
#include <Poco/NotificationQueue.h>

class AppPushWorker: public Poco::Util::Application
{
private:
	// for handling Ctrl+C and terminate request
	static Poco::Event _eventTerminated;
	static BOOL __stdcall ConsoleCtrlHandler(DWORD ctrlType);
	// for the help request by user
	bool _helpRequested{ false };
	void handleHelp(const std::string& name, const std::string& value);
	// for events handle by state machine
	static Poco::NotificationQueue _stateQueue;

protected:
	void initialize(Poco::Util::Application& self);
	void uninitialize();
	void defineOptions(Poco::Util::OptionSet& options);
	void printProperties(const std::string& base);
	int main(const ArgVec& args);

public:
	AppPushWorker() {};
	bool helpRequested();
	static void terminate();
};

class Event_TerminateRequest : public Poco::Notification
{
public:
	Event_TerminateRequest() {}
};
