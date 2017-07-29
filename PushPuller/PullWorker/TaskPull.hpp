#pragma once
#include <string>
#include <memory>
#include <sstream>
#include <vector>
#include <Poco/Task.h>
#include <Poco/Logger.h>
#include <Poco/Format.h>
#include <Poco/NotificationQueue.h>
#include <zmq_addon.hpp>
//#include <Eigen/Core>

using std::string;
using std::ostringstream;
using std::vector;

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
	const string _pullfrom;

public:

	TaskPull(string pullfrom)
		: Task("Puller")
		, _logger(Poco::Logger::get("Puller"))
		, _pullfrom(pullfrom)
	{
	}

	void runTask()
	{
		zmq::context_t context(1);
		zmq::socket_t puller(context, zmq::socket_type::pull);
		try
		{
			puller.connect(_pullfrom);
		}
		catch (std::exception &e)
		{
			poco_debug(_logger, "Failed to connect to " + _pullfrom + " - " + std::string(e.what()));
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
					//std::string strdata = msgIncoming.popstr();
					poco_information(_logger, "### New job:");
					// 1st frame is an integer to indicate the number of point
					int numOfPoints = msgIncoming.poptyp<int>();

					// 2nd frame is 3D points array
					zmq::message_t framePoint3d = msgIncoming.pop();
					std::unique_ptr<Point3d[]> p3data{ new Point3d[numOfPoints] };
					// memory copy to sequential buffer and mapping back to Point3d array is known to work
					std::memcpy(p3data.get(), framePoint3d.data(), framePoint3d.size());
					
					// 3rd frame is the size of double array
					uint32_t sizeOfDoubleArray = msgIncoming.poptyp<uint32_t>();

					// 4th frame is double array
					zmq::message_t frameDoubleArray = msgIncoming.pop();
					double* pdouble = (double *)frameDoubleArray.data();
					vector<double> dvector(pdouble, pdouble + sizeOfDoubleArray);

					// 5th frame is a double scalar
					double dscalar = msgIncoming.poptyp<double>();

					// dump the Point3d array
					ostringstream p3datastr;
					for (int i = 0; i < numOfPoints; ++i)
						p3datastr << "\t[ " << p3data[i].x << ", " << p3data[i].y << ", " << p3data[i].z << " ]\n";
					poco_information(_logger, Poco::format(">>> Point3d array:\n%s", p3datastr.str()));

					// dump the double array
					ostringstream dvecstr;
					dvecstr << "\t[ ";
					for (const auto & d : dvector)
						dvecstr << d << ", ";
					dvecstr << " ]\n";

					poco_information(_logger, Poco::format(">>> double array:\n%s", dvecstr.str()));
					poco_information(_logger, Poco::format(">>> double scalar: %f\n", dscalar));
/*
					// mapping to Eigen::MatrixX3d is also work
					Eigen::Map<Eigen::Matrix<double, -1, 3, Eigen::RowMajor>> p3d2matrix((double *)framePoint3d.data(), numOfPoints, 3);
					Eigen::MatrixX3d matrixdata = p3d2matrix;
					// dump the matrix
					p3datastr.str("");
					p3datastr << matrixdata << std::endl;
					poco_information(_logger, Poco::format(">>> matrix dump:\n%s", p3datastr.str()));
*/
				}
			}
			catch (std::exception &e)
			{
				poco_debug(_logger, "incoming error: " + std::string(e.what()));
			}
		}

		puller.disconnect(_pullfrom);
	}

};

