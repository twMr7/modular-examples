#include "ServoMotionTask.h"
#include "MachineEvents.h"

ServoMotionTask::ServoMotionTask(int32_t speed)
	: Task("ServoMotionTask")
	, _speed(speed)
{
}

void ServoMotionTask::runTask()
{
	// (1). setting moving speed and distance for servo motor
	// (2). command motor to move
	// (3). option to retrieve current position
	int32_t position = 0;

	// report back incremental value to pretend it is the step position
	while (!sleep(std::abs(_speed)))
	{
		position += _speed;
		postNotification(new Event_MotorFeedback(position));
	}
}


