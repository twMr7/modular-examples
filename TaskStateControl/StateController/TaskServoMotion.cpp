#include <Poco/Thread.h>
#include "TaskServoMotion.h"
#include "MachineState.h"

using Poco::Thread;
using Poco::Logger;

TaskServoMotion::TaskServoMotion()
	: Task("TaskServoMotion")
	, _logger(Logger::get("TaskServoMotion"))
{
}

void TaskServoMotion::runTask()
{
	poco_information(_logger, Poco::format("task started Tid=%lu", Thread::currentTid()));
	// (1). setting moving speed and distance for servo motor
	// (2). command motor to move
	// (3). option to retrieve current position
	int position = 0;

	// report back incremental value to pretend it is the step position
	while (!sleep(500))
	{
		position += 123;
		postNotification(new Event_MotorFeedback(position));
	}
}


