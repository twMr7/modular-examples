#include "AbstractPlugin.h"
#include "Poco/ClassLibrary.h"
#include <iostream>

class PluginC : public AbstractPlugin
{
public:
	std::string name() const
	{
		return "PluginC";
	}
};


POCO_BEGIN_MANIFEST(AbstractPlugin)
	POCO_EXPORT_CLASS(PluginC)
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
