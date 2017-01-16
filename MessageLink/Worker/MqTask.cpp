#include <zmq.hpp>
#include <zmq_addon.hpp>
#include "MqTask.h"
#include "MachineEvents.h"

using Poco::Logger;

enum class CommandType : uint8_t
{
	KeepAlive = 0x00,
	StartMotor = 0x01,
	StopMotor = 0x02,
	// special type to mask out invalid 
	Mask = 0x03
};

MqTask::MqTask()
	: Task("MqTask")
	, _logger(Logger::get("MQ"))
{
}

void MqTask::runTask()
{
	zmq::context_t context(1);
	zmq::socket_t commandSubscriber(context, zmq::socket_type::sub);
	commandSubscriber.connect("tcp://127.0.0.1:7889");

	const char* key= "To Worker";
	commandSubscriber.setsockopt(ZMQ_SUBSCRIBE, key, strlen(key));

	while (!sleep(10))
	{
		zmq::multipart_t messageIncoming;
		if (messageIncoming.recv(commandSubscriber, ZMQ_DONTWAIT))
		{
			std::string address = messageIncoming.popstr();
			assert(address == "To Worker");

			CommandType cmdtype = (CommandType)messageIncoming.poptyp<uint8_t>();
			switch (cmdtype)
			{
			case CommandType::StartMotor:
				poco_trace(_logger, "command StartMotor");
				postNotification(new Event_StartMotor(messageIncoming.poptyp<int32_t>()));
				break;

			case CommandType::StopMotor:
				poco_trace(_logger, "command StopMotor");
				postNotification(new Event_StopMotor);
				break;

			case CommandType::KeepAlive:
				poco_trace(_logger, "command KeepAlive");
				break;

			default:
				poco_debug(_logger, "Unknow command type: " + std::to_string((uint8_t)cmdtype));
				break;
			}
		}
	}
}

