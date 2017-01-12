#pragma once

#include "Poco/Util/Subsystem.h"

class MessageLink : public Poco::Util::Subsystem
{
public:
	MessageLink();
	const char* name() const;
protected:
	~MessageLink();
	void initialize(Poco::Util::Application& app);
	void uninitialize();
};
