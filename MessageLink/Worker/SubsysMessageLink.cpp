#include <sstream>
#include <zmq.hpp>
#include "Poco/Runnable.h"
#include "Poco/NotificationQueue.h"
#include "AppWorker.h"
#include "SubsysMessageLink.h"
#include "MachineEvents.h"

using Poco::NotificationQueue;

class MessageQueueIO: public Poco::Runnable
{
private:
	NotificationQueue& _eventQueue;

public:

	MessageQueueIO(NotificationQueue& queue)
		: _eventQueue(queue)
	{
	}

	virtual void run()
	{
		zmq::context_t context(1);
		zmq::socket_t subscriber(context, zmq::socket_type::sub);
		subscriber.connect("tcp://localhost:5566");

		const char* key = "DIOMO";
		subscriber.setsockopt(ZMQ_SUBSCRIBE, key, strlen(key));

		while (1)
		{
			zmq::message_t newmsg;
			subscriber.recv(&newmsg);

			std::string msgstr(static_cast<char*>(newmsg.data()));
			_eventQueue.enqueueNotification(new Event_IncomingMessage(msgstr));
		}
	}
};

SubsysMessageLink::SubsysMessageLink()
	: _mqtask("MessageQueueIO")
{
}

const char * SubsysMessageLink::name() const
{
	return "SubsysMessageLink";
}

SubsysMessageLink::~SubsysMessageLink()
{
}

void SubsysMessageLink::initialize(Poco::Util::Application & app)
{
	AppWorker& appMain = dynamic_cast<AppWorker&>(app);
	MessageQueueIO mqio(appMain.eventQueue());
	_mqtask.start(mqio);
}

void SubsysMessageLink::uninitialize()
{
	_mqtask.join();
}

