#include "Poco/Util/Application.h"
#include "Poco/Util/Option.h"
#include "Poco/Util/OptionSet.h"
#include "Poco/Util/HelpFormatter.h"
#include "Poco/Util/AbstractConfiguration.h"
#include "Poco/AutoPtr.h"
#include "Poco/File.h"
#include "Poco/Message.h"
#include "Poco/NumberParser.h"
#include "Poco/ClassLoader.h"
#include "Poco/Manifest.h"
#include "Poco/String.h"
#include <string>
#include <iostream>
#include <sstream>

#include "AbstractPlugin.h"

using Poco::Util::Application;
using Poco::Util::Option;
using Poco::Util::OptionSet;
using Poco::Util::HelpFormatter;
using Poco::Util::AbstractConfiguration;
using Poco::Util::OptionCallback;
using Poco::AutoPtr;
using Poco::Message;

class AppLoadPlugin : public Application
{
private:
	bool _helpRequested;
	std::string _appConfigFile { "" };
	std::string _pluginFile { "PluginC.dll" };
	std::string _pluginClass { "PluginC" };
	Poco::ClassLoader<AbstractPlugin> _pluginLoader;
	AbstractPlugin* pPluginInstance;

public:
	AppLoadPlugin() : _helpRequested(false)
	{
	}

protected:
	void initialize(Application& self)
	{
		// First, change to desired logging level,
		// so that all the initialization can be logged
		try
		{
			int level = config().getInt("logging.level");
			logger().root().setLevel(level);
		}
		catch (...)
		{
			poco_information(logger(), "Use default logging level");
		}
		poco_debug(logger(), std::string(name()) + " init");

		// load default configuration files, if present
		if (_appConfigFile.empty())
			loadConfiguration();
		else
			loadConfiguration(_appConfigFile);

		Application::initialize(self);

		try
		{
			std::string pluginClass  = config().getString("module.plugin");
			if (0 == Poco::icompare(std::string{ "A" }, pluginClass))
			{
				_pluginFile = "PluginAB.dll";
				_pluginClass = "PluginA";
			}
			else if (0 == Poco::icompare(std::string{ "B" }, pluginClass))
			{

				_pluginFile = "PluginAB.dll";
				_pluginClass = "PluginB";
			}
			// else use default
		}
		catch (...)
		{
			poco_information(logger(), "Load default plugin C");
		}
		_pluginLoader.loadLibrary(_pluginFile);
		Poco::ClassLoader<AbstractPlugin>::Iterator it(_pluginLoader.begin());
		Poco::ClassLoader<AbstractPlugin>::Iterator end(_pluginLoader.end());
		for (; it != end; ++it)
		{
			std::cout << "lib path: " << it->first << std::endl;
			Poco::Manifest<AbstractPlugin>::Iterator itMan(it->second->begin());
			Poco::Manifest<AbstractPlugin>::Iterator endMan(it->second->end());
			for (; itMan != endMan; ++itMan)
				std::cout << "Found plugin class in lib: " << itMan->name() << std::endl;
		}

		// load the class
		pPluginInstance = _pluginLoader.create(_pluginClass);
		_pluginLoader.classFor(_pluginClass).autoDelete(pPluginInstance);
	}

	void uninitialize()
	{
		poco_debug(logger(), std::string(name()) + " uninit");
		// our own uninitialization code here
		_pluginLoader.unloadLibrary(_pluginFile);

		Application::uninitialize();
	}

	void reinitialize(Application& self)
	{
		poco_debug(logger(), std::string(name()) + " reinit");

		// our own uninitialization code here
		_pluginLoader.unloadLibrary(_pluginFile);

		Application::reinitialize(self);

		// our own reinitialization code here
		_pluginLoader.loadLibrary(_pluginFile);
		pPluginInstance = _pluginLoader.create(_pluginClass);
		_pluginLoader.classFor(_pluginClass).autoDelete(pPluginInstance);
	}

	void defineOptions(OptionSet& options)
	{
		Application::defineOptions(options);

		options.addOption(
			Option("help", "h", "display help information on command line arguments")
			.required(false)
			.repeatable(false)
			.callback(OptionCallback<AppLoadPlugin>(this, &AppLoadPlugin::handleHelp)));

		options.addOption(
			Option("config", "c", "load configuration data from a file")
			.required(false)
			.repeatable(true)
			.argument("file")
			.callback(OptionCallback<AppLoadPlugin>(this, &AppLoadPlugin::handleConfig)));

		options.addOption(
			Option("define", "d", "define a configuration property")
			.required(false)
			.repeatable(true)
			.argument("name=value")
			.callback(OptionCallback<AppLoadPlugin>(this, &AppLoadPlugin::handleDefine)));

		options.addOption(
			Option("bind", "b", "bind option value to test.property")
			.required(false)
			.repeatable(false)
			.argument("value")
			.binding("test.property"));

		options.addOption(
			Option("loglevel", "l", "[1-8] or level name to assign logging verbose level")
			.required(false)
			.repeatable(false)
			.argument("level")
			.callback(OptionCallback<AppLoadPlugin>(this, &AppLoadPlugin::handleLogLevel)));

		options.addOption(
			Option("module", "m", "{'A', 'B', 'C'} to load different plugin class")
			.required(true)
			.repeatable(false)
			.argument("name")
			.callback(OptionCallback<AppLoadPlugin>(this, &AppLoadPlugin::handleModuleName)));

	}

	void handleHelp(const std::string& name, const std::string& value)
	{
		_helpRequested = true;
		displayHelp();
		stopOptionsProcessing();
	}

	void handleDefine(const std::string& name, const std::string& value)
	{
		poco_debug(logger(), "handleDefine: " + name + "=" + value);
		defineProperty(value);
	}

	void handleConfig(const std::string& name, const std::string& value)
	{
		poco_debug(logger(), "handleConfig: " + value);
		if (Poco::Path(value).isFile())
		{
			poco_trace(logger(), value + " is file.");
			if (Poco::File(value).exists())
				_appConfigFile = value;
			else
				poco_trace(logger(), value + " is not exist.");
		}
	}

	void handleLogLevel(const std::string& name, const std::string& value)
	{
		poco_debug(logger(), "handleLogLevel: " + value);
		try
		{
			// fatal       = 1 (highest priority)
			// critical    = 2
			// error       = 3
			// warning     = 4
			// notice      = 5
			// information = 6
			// debug       = 7
			// trace       = 8 (lowest priority)
			int level = logger().parseLevel(value);
			config().setInt("logging.level", level);
		}
		catch (Poco::InvalidArgumentException(exp))
		{
			poco_error(logger(), "argument " + value + " " + exp.message());
		}
	}

	void handleModuleName(const std::string& name, const std::string& value)
	{
		poco_debug(logger(), "handleModuleName: " + value);
		if (0 == Poco::icompare(std::string{ "A" }, value) ||
			0 == Poco::icompare(std::string{ "B" }, value) ||
			0 == Poco::icompare(std::string{ "C" }, value))
			config().setString("module.plugin", value);
	}

	void displayHelp()
	{
		HelpFormatter helpFormatter(options());
		helpFormatter.setCommand(commandName());
		helpFormatter.setUsage("OPTIONS");
		helpFormatter.setHeader("A sample application that demonstrates features of Application, commandline options, configuration and loading plugins.");
		helpFormatter.format(std::cout);
	}

	void defineProperty(const std::string& def)
	{
		std::string name;
		std::string value;
		std::string::size_type pos = def.find('=');
		if (pos != std::string::npos)
		{
			name.assign(def, 0, pos);
			value.assign(def, pos + 1, def.length() - pos);
		}
		else name = def;
		config().setString(name, value);
	}

	int main(const ArgVec& args)
	{
		if (!_helpRequested)
		{
			logger().information("Command line:");
			std::ostringstream ostr;
			for (ArgVec::const_iterator it = argv().begin(); it != argv().end(); ++it)
			{
				ostr << *it << ' ';
			}
			logger().information(ostr.str());
			logger().information("Arguments to main():");
			for (ArgVec::const_iterator it = args.begin(); it != args.end(); ++it)
			{
				logger().information(*it);
			}
			logger().information("Application properties:");
			printProperties("");
		}

		// show loaded plugin
		std::cout << "Module: " << pPluginInstance->name() << " is used this time." << std::endl;

		return Application::EXIT_OK;
	}

	void printProperties(const std::string& base)
	{
		AbstractConfiguration::Keys keys;
		config().keys(base, keys);
		if (keys.empty())
		{
			if (config().hasProperty(base))
			{
				std::string msg;
				msg.append(base);
				msg.append(" = ");
				msg.append(config().getString(base));
				logger().information(msg);
			}
		}
		else
		{
			for (AbstractConfiguration::Keys::const_iterator it = keys.begin(); it != keys.end(); ++it)
			{
				std::string fullKey = base;
				if (!fullKey.empty()) fullKey += '.';
				fullKey.append(*it);
				printProperties(fullKey);
			}
		}
	}

};


POCO_APP_MAIN(AppLoadPlugin)
/*
// The macro POCO_APP_MAIN(App) expand to
int wmain(int argc, wchar_t** argv)
{
	Poco::AutoPtr<App> pApp = new App;
	try
	{
	    // init() process command line and set properties
		pApp->init(argc, argv);
	}
	catch (Poco::Exception& exc)
	{
		pApp->logger().log(exc);
		return Poco::Util::Application::EXIT_CONFIG;
	}

	// initialize(), main(), and then uninitialize()
	return pApp->run();
}
*/
