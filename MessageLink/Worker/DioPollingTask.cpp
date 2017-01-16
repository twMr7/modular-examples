#include "Poco/Util/Application.h"
#include "Poco/NumberFormatter.h"

#include "DioPollingTask.h"
#include "MachineEvents.h"

using Poco::Task;
using Poco::Logger;
using Poco::NumberFormatter;

DioPollingTask::DioPollingTask()
	: Task("DioPollingTask")
	, _stateDin(0)
	, _stateDout(0)
	, _logger(Logger::get("Dio"))
{
}

void DioPollingTask::runTask()
{
	// (1). real DIO should perform a initial reading here

	// Task.sleep() will return true immediately if the task is cancelled while sleeping.
	while (!sleep(1000))
	{
		// (2). read the DIO value for every N msec.
		// simulating the ports reading by assigning values in series order 
		// NOTE: If states can be R/W by mutiple threads, the following read/write state must be protected by mutex
		if (_stateDin == 0x000 || _stateDin == 0x800)
		{
			_stateDin = 0x001;
		}
		else
		{
			_stateDin <<= 1;
		}

		// (3). compare to the old state to see if there is any bit changed
		// skip this part for simulation

		if (!isCancelled())
		{
			// take corresonding actions for the changes
			/*if (_stateDin == (uint16_t)Din::Sensor1)
			{
				poco_trace(_logger, "Sensor1 ON");
				postNotification(new Event_Sensor1Changed(true));
			}
			else if (_stateDin == (uint16_t)Din::Sensor2)
			{
				poco_trace(_logger, "Sensor2 ON");
				postNotification(new Event_Sensor2Changed(true));
			}
			else*/
			{
				poco_trace(_logger, "DIO changes " + NumberFormatter::formatHex(_stateDin, true));
			}
		}
	}
}
