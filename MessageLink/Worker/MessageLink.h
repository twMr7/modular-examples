#pragma once

#include "Poco/Util/Subsystem.h"
#include "zmq.hpp"

class MessageLink : public Poco::Util::Subsystem
{
private:
	zmq::context_t _mq;
public:
	MessageLink();
	const char* name() const;
protected:
	~MessageLink();
	void initialize(Poco::Util::Application& app);
	void uninitialize();
};
