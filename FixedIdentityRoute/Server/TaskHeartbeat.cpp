#include <algorithm>
#include <zmq_addon.hpp>
#include "TaskHeartbeat.h"
#include "ServerEvents.h"

#define HEARTBEAT_PING 0x55
#define HEARTBEAT_PONG 0xAA

#define HB_ALIVE 1
#define HB_WAITPONG 0
#define HB_MISSINGPONG2 -1
#define HB_MISSINGPONG3 -2
#define HB_MISSINGPONG4 -3
#define HB_AWAY -4

using Poco::Logger;
using std::vector;
using std::string;

TaskHeartbeat::TaskHeartbeat(vector<string>& clientlist)
	: Task("TaskHeartbeat")
	, _logger(Logger::get("Heartbeat"))
	, _clientid(clientlist)
{
	for (const auto& id : _clientid)
		_clientHeart[id] = HB_AWAY;
}

void TaskHeartbeat::runTask()
{
	zmq::context_t context(1);
	zmq::socket_t socketRouter(context, zmq::socket_type::router);
	int raiseIfUnroutable = 1;
	socketRouter.setsockopt(ZMQ_ROUTER_MANDATORY, &raiseIfUnroutable, sizeof(raiseIfUnroutable));
	socketRouter.bind("tcp://127.0.0.1:6801");

	zmq::pollitem_t items[] = { { socketRouter, 0, ZMQ_POLLIN, 0 } };
	// for simplicity, use a counter to send heartbeat between desired time interval
	uint16_t interval = 0;

	while (!sleep(10))
	{
		++interval;
		try
		{
			// heartbeat shall ping about every 2000 msec
			if (interval >= 200)
			{
				// reset interval
				interval = 0;
				// send heartbeat ping to every expected client
				for (const auto& id : _clientid)
				{
					// client is considered away if 5 pongs are missing
					if (_clientHeart[id] == HB_MISSINGPONG4)
					{
						_clientHeart[id] = HB_AWAY;
						poco_trace(_logger, id + " is gone");
						postNotification(new Event_ClientLinkDown(id));
					}
					else if (_clientHeart[id] > HB_MISSINGPONG4)
					{
						_clientHeart[id] = _clientHeart[id] - 1;
					}

					// dead or alive, send out heartbeat ping
					zmq::multipart_t msgOutgoing;
					msgOutgoing.addstr(id);
					msgOutgoing.addtyp<uint8_t>(HEARTBEAT_PING);
					try
					{
						msgOutgoing.send(socketRouter);
					}
					catch (std::exception &e)
					{
						// unroutable message should raise exception EHOSTUNREACH
						poco_trace(_logger, e.what());
						continue;
					}
				}
			}

			// polling the incoming message
			zmq::poll(items, 1, 0);
			if (items[0].revents & ZMQ_POLLIN)
			{
				zmq::multipart_t msgIncoming;
				if (msgIncoming.recv(socketRouter, ZMQ_DONTWAIT))
				{
					// the first frame is client id appended by router socket
					std::string id = msgIncoming.popstr();
					auto itclient = std::find(_clientid.cbegin(), _clientid.cend(), id);
					poco_trace(_logger, "Incoming message from " + *itclient);
					// is it one of the expected clients? 
					if (itclient != _clientid.cend())
					{
						if (msgIncoming.empty())
							poco_debug(_logger, "Invalid message: empty payload");
						if (HEARTBEAT_PONG == msgIncoming.poptyp<uint8_t>())
						{
							switch (_clientHeart[id])
							{
							case HB_MISSINGPONG2:
							case HB_MISSINGPONG3:
							case HB_MISSINGPONG4:
							case HB_AWAY:
								_clientHeart[id] = HB_ALIVE;
								poco_trace(_logger, "<-- Heartbeat_Pong from " + *itclient);
								postNotification(new Event_ClientLinkUp(*itclient));
								break;

							case HB_WAITPONG:
								_clientHeart[id] += 1;
								break;

							case HB_ALIVE:
							default:
								break;
							}
						}
					}
				}
			}
		}
		catch (std::exception &e)
		{
			poco_debug(_logger, e.what());
		}
	}
}

