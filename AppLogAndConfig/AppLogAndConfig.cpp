#include <string>
#include <iostream>
#include <sstream>
#include <Poco/Util/Option.h>
#include <Poco/Util/HelpFormatter.h>
#include <Poco/Util/AbstractConfiguration.h>
#include <Poco/File.h>
#include "AppLogAndConfig.h"

using std::string;
using std::ostringstream;
using Poco::Util::Application;
using Poco::Util::Option;
using Poco::Util::OptionSet;
using Poco::Util::OptionCallback;
using Poco::Util::HelpFormatter;
using Poco::Util::AbstractConfiguration;
using Poco::File;

void AppLogAndConfig::handleOptionHelp(const string & option, const string & argument)
{
	poco_trace(logger(), "handleOptionHelp: " + option + "=" + argument);
	_helpRequested = true;
	// display help
	HelpFormatter helpFormatter(options());
	helpFormatter.setCommand(commandName());
	helpFormatter.setUsage("OPTIONS");
	helpFormatter.setHeader("A sample application to demonstrate features of Poco::Util::Application, commandline options, configuration, and logging.");
	helpFormatter.format(std::cout);
	// stop further processing
	stopOptionsProcessing();
}

void AppLogAndConfig::handleOptionDefine(const string & option, const string & argument)
{
	poco_trace(logger(), "handleOptionDefine: " + option + "=" + argument);
	std::string name;
	std::string value;
	std::string::size_type pos = argument.find('=');
	if (pos != std::string::npos)
	{
		name.assign(argument, 0, pos);
		value.assign(argument, pos + 1, argument.length() - pos);
	}
	else
	{
		name = argument;
	}
	config().setString("custom." + name, value);
}

void AppLogAndConfig::printProperties(const std::string& base)
{
	AbstractConfiguration::Keys keys;
	config().keys(base, keys);
	if (keys.empty())
	{
		if (config().hasProperty(base))
			logger().information(base + " = " + config().getString(base));
	}
	else
	{
		for (const auto & key : keys)
		{
			std::string fullKey = base;
			if (!fullKey.empty())
				fullKey += '.';
			fullKey.append(key);
			printProperties(fullKey);
		}
	}
}

void AppLogAndConfig::initialize(Application & self)
{
	poco_information(logger(), config().getString("application.baseName", name()) + " initialize");
	// load default configuration file
	loadConfiguration();
	// all registered subsystems are initialized in ancestor's initialize procedure
	Application::initialize(self);
}

void AppLogAndConfig::uninitialize()
{
	poco_information(logger(), config().getString("application.baseName", name()) + " uninitialize");
	// ancestor uninitialization
	Application::uninitialize();
}

void AppLogAndConfig::defineOptions(Poco::Util::OptionSet & options)
{
	Application::defineOptions(options);

	options.addOption(
		Option("help", "h", "[help | h] display help information on command line arguments")
		.required(false)
		.repeatable(false)
		.callback(OptionCallback<AppLogAndConfig>(this, &AppLogAndConfig::handleOptionHelp)));

	options.addOption(
		Option("define", "d", "[define | d] define a custom configuration property")
		.required(false)
		.repeatable(true)
		.argument("name=value")
		.callback(OptionCallback<AppLogAndConfig>(this, &AppLogAndConfig::handleOptionDefine)));

	// This option assign argument directly to configuration "custom.configFile".
	// If argument validation is needed, we need to implement a FileValidator class derived from Validator
	options.addOption(
		Option("file", "f", "[file | f] assign a custom configuration file to load")
		.required(false)
		.repeatable(false)
		.argument("filename")
		.binding("custom.configFile"));
}

int AppLogAndConfig::main(const ArgVec & args)
{
	if (_helpRequested)
		return Application::EXIT_USAGE;

	// load custom configuration file
	string filename = config().getString("custom.configFile", "");
	if (!filename.empty() && File(filename).exists())
	{
		try
		{
			loadConfiguration(filename);
		}
		catch (Poco::Exception& e)
		{
			logger().log(e);
		}
	}

	// Although the logger should log messages only in proper logging level, it is good practice to
	// check current level setting to avoid the overhead of string construction.
	// Two way to do the same thing, use one of the following methods wherever appropriate:
	// (1). check if logger().*LevelName*() then do logger().*LevelName*("message string")
	// (2). use Poco's predefined macro: poco_LevelName(logger(), "message string")
	if (logger().information())
	{
		ostringstream ostrs;
		ostrs << "Command line:\n\t";
		for (const auto & arg : argv())
			ostrs << arg << " ";
		ostrs << std::endl;

		// unrecognized options are passed to as args of Application::main
		ostrs << "Arguments to Application::main():\n\t";
		for (const auto & arg : args)
			ostrs << arg << " ";
		ostrs << std::endl;

		logger().information(ostrs.str());

		logger().information("Application properties:\n");
		printProperties("");
	}

	/*AbstractConfiguration::Keys keys;
	config().keys("custom", keys);*/
	if (config().hasProperty("custom.configFile"))
	{
		ostringstream ostrs;
		ostrs << "has custom property:\n";
		int32_t propertyInt = config().getInt("custom.SignedInteger", 0);
		uint32_t propertyUint = config().getUInt("custom.UnsignedInteger", 0);
		double propertyDouble = config().getDouble("custom.Double", .0);
		ostrs << "\tcustom property SignedInteger = " << propertyInt << "\n";
		ostrs << "\tcustom property UnsignedInteger = " << propertyUint << "\n";
		ostrs << "\tcustom property Double = " << propertyDouble << "\n";
		logger().information(ostrs.str());
	}

	return Application::EXIT_OK;
}

bool AppLogAndConfig::helpRequested()
{
	return _helpRequested;
}
