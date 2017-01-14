#pragma once

#include "Poco/Util/Subsystem.h"
#include "Poco/Thread.h"

class SubsysMessageLink : public Poco::Util::Subsystem
{
private:
	Poco::Thread _mqtask;
public:
	SubsysMessageLink();
	const char* name() const;
protected:
	~SubsysMessageLink();
	void initialize(Poco::Util::Application& app);
	void uninitialize();
};
