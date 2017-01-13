#include "MessageLink.h"

MessageLink::MessageLink()
	: _mq{ 1 }
{
}

const char * MessageLink::name() const
{
	return "MessageLink";
}

MessageLink::~MessageLink()
{
	_mq.close();
}

void MessageLink::initialize(Poco::Util::Application & app)
{
}

void MessageLink::uninitialize()
{
}

