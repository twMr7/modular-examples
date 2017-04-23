/*
	File:
		TaskPull.hpp
	Description:
		Task to send/receive messages over OMLP (Orisol Module Link Protocol)
		This is the implementation for worker modules, and the same file can
		be shared between projects of worker modules.
	Copyright:
		Orisol Asia Ltd.
*/
#pragma once
#include <string>
#include <memory>
#include <sstream>
#include <Poco/Task.h>
#include <Poco/Logger.h>
#include <Poco/Format.h>
#include <Poco/NotificationQueue.h>
#include <zmq_addon.hpp>
#include <Eigen/Core>

using std::ostringstream;

#define PUSHER_ADDRESS "tcp://127.0.0.1:6866"

struct Point3d
{
	double x;
	double y;
	double z;
};

class TaskPull : public Poco::Task
{
private:
	Poco::Logger& _logger;

public:

	TaskPull()
		: Task("Puller")
		, _logger(Poco::Logger::get("Puller"))
	{
	}

	void runTask()
	{
		zmq::context_t context(1);
		zmq::socket_t puller(context, zmq::socket_type::pull);
		try
		{
			puller.connect(PUSHER_ADDRESS);
		}
		catch (std::exception &e)
		{
			poco_debug(_logger, "Failed to connect to " + std::string(PUSHER_ADDRESS) + " - " + std::string(e.what()));
			return;
		}

		// try one receive and one send for every 10 msec.
		while (!sleep(1000))
		{
			// try receive an incoming message
			try
			{
				zmq::multipart_t msgIncoming;
				if (msgIncoming.recv(puller, ZMQ_DONTWAIT))
				{
					// 1st frame is string data
					std::string strdata = msgIncoming.popstr();
					poco_information(_logger, Poco::format("### New job:\n%s", strdata));
					// 2nd frame is an integer to indicate the number of point
					int numOfPoints = msgIncoming.poptyp<int>();

					// 3rd frame is 3D points array
					zmq::message_t memFrame = msgIncoming.pop();
					std::unique_ptr<Point3d[]> p3data{ new Point3d[numOfPoints] };
					// memory copy to sequential buffer and mapping back to Point3d array is known to work
					std::memcpy(p3data.get(), memFrame.data(), memFrame.size());
					// dump the array
					ostringstream p3datastr;
					for (int i = 0; i < numOfPoints; ++i)
					{
						p3datastr << "\t[ " << p3data[i].x << ", " << p3data[i].y << ", " << p3data[i].z << " ]\n";
					}
					poco_information(_logger, Poco::format(">>> p3data array:\n%s", p3datastr.str()));

					// mapping to Eigen::MatrixX3d is also work
					Eigen::Map<Eigen::Matrix<double, -1, 3, Eigen::RowMajor>> p3d2matrix((double *)memFrame.data(), numOfPoints, 3);
					Eigen::MatrixX3d matrixdata = p3d2matrix;
					// dump the matrix
					p3datastr.str("");
					p3datastr << matrixdata << std::endl;
					poco_information(_logger, Poco::format(">>> matrix dump:\n%s", p3datastr.str()));
				}
			}
			catch (std::exception &e)
			{
				poco_debug(_logger, "incoming error: " + std::string(e.what()));
			}
		}

		puller.disconnect(PUSHER_ADDRESS);
	}

};

