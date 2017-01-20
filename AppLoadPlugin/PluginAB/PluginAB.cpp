#include "AbstractPlugin.h"
#include "AbstractOtherPlugin.h"
#include "Poco/ClassLibrary.h"
#include "Poco/LocalDateTime.h"
#include <iostream>

class PluginA : public AbstractPlugin
{
public:
	std::string name() const
	{
		return "PluginA";
	}
};

class PluginB : public AbstractOtherPlugin
{
public:
	std::string name() const
	{
		return "PluginB";
	}

	std::string time() const
	{
		Poco::LocalDateTime now;
		std::string strNow =
			std::to_string(now.year()) + "/" +
			std::to_string(now.month()) + "/" +
			std::to_string(now.day()) + " " +
			std::to_string(now.hour()) + ":" +
			std::to_string(now.minute()) + ":" +
			std::to_string(now.second());
		return strNow;
	}
};

POCO_BEGIN_MANIFEST(AbstractPlugin)
	POCO_EXPORT_CLASS(PluginA)
POCO_END_MANIFEST

POCO_BEGIN_NAMED_MANIFEST(B, AbstractOtherPlugin)
	POCO_EXPORT_CLASS(PluginB)
POCO_END_MANIFEST

// optional set up and clean up functions
void pocoInitializeLibrary()
{
	std::cout << "PluginLibrary initializing" << std::endl;
}

void pocoUninitializeLibrary()
{
	std::cout << "PluginLibrary uninitializing" << std::endl;
}
