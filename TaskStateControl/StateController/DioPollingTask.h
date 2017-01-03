#pragma once

#include "Poco/Task.h"
#include "Poco/Logger.h"
#include "Poco/Notification.h"

class DioPollingTask : public Poco::Task
{
private:
	uint16_t _stateDin;
	uint16_t _stateDout;
	Poco::Logger& _logger;

public:
	DioPollingTask();
	void runTask();
};

enum class Din : uint16_t
{
	Device1 = 0x001,
	Device2 = 0x002,
	NotUsed1 = 0x004,
	Button1 = 0x008,
	Sensor1 = 0x010,
	NotUsed2 = 0x020,
	Device3 = 0x040,
	Device4 = 0x080,
	NotUsed3 = 0x100,
	Device5 = 0x200,
	Button2 = 0x400,
	Sensor2 = 0x800,
	Mask = 0x0FFF
};

class Sensor1Changed : public Poco::Notification
{
public:
	Sensor1Changed(bool value) : _value(value) {}
	bool isON() const { return _value; }

private:
	bool _value;
};

class Sensor2Changed: public Poco::Notification
{
public:
	Sensor2Changed(bool value) : _value(value) {}
	bool isON() const { return _value; }

private:
	bool _value;
};
