#include <string>
#include <algorithm>
#include <zmq_addon.hpp>
#include "TaskHeartbeat.h"
#include "ClientEvents.h"

#define HEARTBEAT_PING 0x55
#define HEARTBEAT_PONG 0xAA

using std::string;
using Poco::Logger;

TaskHeartbeat::TaskHeartbeat(string & id)
	: Task("TaskHeartbeat")
	, _logger(Logger::get("Heartbeat"))
	, _identity(id)
{
}

void TaskHeartbeat::runTask()
{
	zmq::context_t context(1);
	zmq::socket_t socketDealer(context, zmq::socket_type::dealer);
	socketDealer.setsockopt(ZMQ_IDENTITY, _identity.c_str(), _identity.size());
	socketDealer.connect("tcp://127.0.0.1:6801");

	while (!sleep(10))
	{
		try
		{
			zmq::multipart_t msgIncoming;
			if (msgIncoming.recv(socketDealer, ZMQ_DONTWAIT))
			{
				if (msgIncoming.empty())
					poco_debug(_logger, "Invalid message: empty payload");

				if (HEARTBEAT_PING == msgIncoming.poptyp<uint8_t>())
				{
					poco_trace(_logger, "<-- heartbeat ping received, send back a pong.");
					postNotification(new Event_ServerLinkUp);
					zmq::multipart_t msgOutgoing;
					msgOutgoing.addtyp<uint8_t>(HEARTBEAT_PONG);
					msgOutgoing.send(socketDealer);
				}
			}
		}
		catch (std::exception &e)
		{
			poco_debug(_logger, e.what());
		}
	}

	socketDealer.disconnect("tcp://127.0.0.1:6866");
}

