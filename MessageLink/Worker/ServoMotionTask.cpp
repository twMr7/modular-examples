#include "ServoMotionTask.h"
#include "MachineEvents.h"

ServoMotionTask::ServoMotionTask()
	: Task("ServoMotionTask")
{
}

void ServoMotionTask::runTask()
{
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


