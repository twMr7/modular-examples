/*
	File:
		TaskPush.hpp
	Description:
		Task to send/receive messages over OMLP (Orisol Module Link Protocol)
		This is the implementation for worker modules, and the same file can
		be shared between projects of worker modules.
	Copyright:
		Orisol Asia Ltd.
*/
#pragma once
#include <string>
#include <sstream>
#include <Poco/Task.h>
#include <Poco/Logger.h>
#include <Poco/Format.h>
#include <Poco/NotificationQueue.h>
#include <zmq_addon.hpp>

using std::ostringstream;

#define PUSHER_ADDRESS "tcp://127.0.0.1:6866"

struct Point3d
{
	double x;
	double y;
	double z;
};

class TaskPush : public Poco::Task
{
private:
	Poco::Logger& _logger;

public:

	TaskPush()
		: Task("Pusher")
		, _logger(Poco::Logger::get("Pusher"))
	{
	}

	void runTask()
	{
		zmq::context_t context(1);
		zmq::socket_t pusher(context, zmq::socket_type::push);
		try
		{
			pusher.bind(PUSHER_ADDRESS);
		}
		catch (std::exception &e)
		{
			poco_debug(_logger, "Failed to connect to " + std::string(PUSHER_ADDRESS) + " - " + std::string(e.what()));
			return;
		}

		int job = 1;
		// try one receive and one send for every 10 msec.
		while (!sleep(1000))
		{
			// try send an outgoing message
			try
			{
				zmq::multipart_t msgOutgoing;
				int job_start = job;
				constexpr int numOfPoints = 4;
				Point3d p3data[numOfPoints];
				ostringstream strdata;
				for (int i = 0; i < numOfPoints; ++i)
				{
					p3data[i].x = std::sqrt(job++);
					p3data[i].y = std::sqrt(job++);
					p3data[i].z = std::sqrt(job++);
					strdata << "\t[ " << p3data[i].x << ", " << p3data[i].y << ", " << p3data[i].z << " ]\n";
				}
				int job_end = job - 1;
				msgOutgoing.addstr(strdata.str());
				msgOutgoing.addtyp<int>(numOfPoints);
				msgOutgoing.addmem(&p3data, sizeof p3data);
				msgOutgoing.send(pusher, ZMQ_DONTWAIT);
				poco_information(_logger, Poco::format("push job#%d-%d:\n%s", job_start, job_end, strdata.str()));
			}
			catch (std::exception &e)
			{
				poco_debug(_logger, "outgoing error: " + std::string(e.what()));
			}
		}

		pusher.disconnect(PUSHER_ADDRESS);
	}

};

