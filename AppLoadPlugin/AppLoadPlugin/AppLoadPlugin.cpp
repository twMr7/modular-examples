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
#include "Poco/ConsoleChannel.h"
#include "Poco/Exception.h"
#include <string>
#include <iostream>
#include <sstream>

#include "AbstractPlugin.h"
#include "AbstractOtherPlugin.h"

using std::string;
using Poco::Util::Application;
using Poco::Util::Option;
using Poco::Util::OptionSet;
using Poco::Util::HelpFormatter;
using Poco::Util::AbstractConfiguration;
using Poco::Util::OptionCallback;
using Poco::ConsoleChannel;
using Poco::AutoPtr;
using Poco::Message;

class AppLoadPlugin : public Application
{
private:
	bool _helpRequested;
	Poco::ClassLoader<AbstractPlugin> _pluginLoader;
	Poco::ClassLoader<AbstractOtherPlugin> _pluginBLoader;
	AbstractPlugin* _pPluginInstance{ nullptr };
	AbstractOtherPlugin* _pPluginBInstance{ nullptr };
	string _checkKey;

public:
	AppLoadPlugin() : _helpRequested(false)
	{
	}

	bool helpRequested()
	{
		return _helpRequested;
	}

protected:
	void initialize(Application& self)
	{
		poco_information(logger(), config().getString("application.name", name()) + " initialize");

		// load specific configuration files, if present
		try
		{
			std::string filename = config().getString("application.configFile");
			if (Poco::Path(filename).isFile() && Poco::File(filename).exists())
			{
				poco_debug(logger(), "Load configuration file " + filename);
				loadConfiguration(filename);
			}
			else
			{
				throw Poco::NotFoundException();
			}
		}
		catch (...)
		{
			poco_debug(logger(), "try loading default configuration file");
			loadConfiguration();
		}

		// ancestor initialization
		// registered subsystems, e.g. logger, are initialized here
		try
		{
			Application::initialize(self);
		}
		catch (...)
		{
			// If Poco has problem with configuration file, try catch the exception to abort gracefully.
			std::clog << "Fail to initialize Application, it possibly caused by wrong configuration." << std::endl;
			std::exit(EXIT_FAILURE);
		}

		loadPlugin();
	}

	void uninitialize()
	{
		poco_information(logger(), config().getString("application.name", name()) + " uninitialize");

		// our own uninitialization code here
		unloadPlugin();
		// no matter what the current channel is, reset it back to default console channel
		// to avoid unpredictable result cause by other channel type, AsyncChannel especially.
		AutoPtr<ConsoleChannel> pCC = new ConsoleChannel;
		logger().setChannel(pCC);

		// ancestor uninitialization
		Application::uninitialize();
	}

	void reinitialize(Application& self)
	{
		poco_information(logger(), config().getString("application.name", name()) + " reinitialize");

		// our own uninitialization code here
		unloadPlugin();
		AutoPtr<ConsoleChannel> pCC = new ConsoleChannel;
		logger().setChannel(pCC);

		// reload configuration file
		try
		{
			std::string filename = config().getString("application.configFile");
			if (Poco::Path(filename).isFile() && Poco::File(filename).exists())
				loadConfiguration(filename);
			else
				throw Poco::NotFoundException();
		}
		catch (...)
		{
			loadConfiguration();
		}

		// ancestor uninitialization
		Application::reinitialize(self);

		// our own reinitialization code here
		loadPlugin();
	}

	void defineOptions(OptionSet& options)
	{
		Application::defineOptions(options);

		options.addOption(
			Option("help", "h", "display help information on command line arguments")
			.required(false)
			.repeatable(false)
			.callback(OptionCallback<AppLoadPlugin>(this, &AppLoadPlugin::handleArgumentHelp)));

		options.addOption(
			Option("define", "d", "define a configuration property")
			.required(false)
			.repeatable(true)
			.argument("name=value")
			.callback(OptionCallback<AppLoadPlugin>(this, &AppLoadPlugin::handleArgumentDefine)));

		// This option assign argument directly to configuration "application.configFile".
		// If argument validation is needed, we need to implement a FileValidator class derived from Validator
		options.addOption(
			Option("file", "f", "assign a specific configuration file name to load")
			.required(false)
			.repeatable(false)
			.argument("filename")
			.binding("application.configFile"));

		// This option also change the configuration "application.configFile" in callback handler.
		// The difference is that callback handler can do additional check on the arguments.
		options.addOption(
			Option("config", "c", "load configuration data from a file")
			.required(false)
			.repeatable(true)
			.argument("filename")
			.callback(OptionCallback<AppLoadPlugin>(this, &AppLoadPlugin::handleArgumentConfig)));

		options.addOption(
			Option("plugin", "p", "load different plugin class, available plugins: { 'PluginA', 'PluginC' }")
			.required(false)
			.repeatable(false)
			.argument("class")
			.callback(OptionCallback<AppLoadPlugin>(this, &AppLoadPlugin::handleArgumentPluginClass)));

		options.addOption(
			Option("key", "k", "check configuration key")
			.required(false)
			.repeatable(false)
			.argument("key2check")
			.callback(OptionCallback<AppLoadPlugin>(this, &AppLoadPlugin::handleArgumentCheckKey)));
	}

	void handleArgumentHelp(const std::string& name, const std::string& value)
	{
		_helpRequested = true;
		displayHelp();
		stopOptionsProcessing();
	}

	void handleArgumentDefine(const std::string& name, const std::string& value)
	{
		poco_debug(logger(), "handleArgumentDefine: " + name + "=" + value);
		defineProperty(value);
	}

	void handleArgumentConfig(const std::string& name, const std::string& value)
	{
		poco_debug(logger(), "handleArgumentConfig: " + value);
		// We can do additional check on the validity of argument here.
		// However, since the value of "application.configFile" may be set by various ways,
		// it is more proper to check the existence of file right before the application needs to load it in. 
		config().setString("application.configFile", value);
	}

	void handleArgumentPluginClass(const std::string& name, const std::string& value)
	{
		poco_debug(logger(), "handleArgumentPluginClass: " + value);
		// validate the argument for the availability of plugin classes
		if (value == "PluginA" || value == "PluginC")
			config().setString("plugin.class", value);
	}

	void handleArgumentCheckKey(const std::string& name, const std::string& value)
	{
		poco_debug(logger(), "handleArgumentCheckKey: " + value);
		_checkKey = value;
	}

	void displayHelp()
	{
		HelpFormatter helpFormatter(options());
		helpFormatter.setCommand(commandName());
		helpFormatter.setUsage("OPTIONS");
		helpFormatter.setHeader("A sample application that demonstrates features of Application, commandline options, configuration, logging, and loading plugins.");
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

	void loadPlugin()
	{
		if (_pPluginInstance != nullptr)
			return;

		try
		{
			std::string pluginClass = config().getString("plugin.class", "PluginC");
			std::string pluginFile;
			if (pluginClass == "PluginA")
			{
				pluginFile = "PluginAB.dll";
				if (Poco::File(pluginFile).exists())
				{
					// load additional class B
					_pluginBLoader.loadLibrary(pluginFile, "B");
					// display the information about plugin class if needed
					for (auto it = _pluginBLoader.begin(), end = _pluginBLoader.end(); it != end; ++it)
					{
						poco_trace(logger(), "plugin path: " + it->first);
						for (auto itMan = it->second->begin(), endMan = it->second->end(); itMan != endMan; ++itMan)
							poco_trace(logger(), "found class in plugin lib: " + std::string(itMan->name()));
					}
					// load the class B
					_pPluginBInstance = _pluginBLoader.create("PluginB");
					_pluginBLoader.classFor("PluginB").autoDelete(_pPluginBInstance);
				}
			}
			else
			{
				pluginFile = "PluginC.dll";
			}

			// check the existence of the .dll file before loading it
			if (Poco::File(pluginFile).exists())
			{
				_pluginLoader.loadLibrary(pluginFile);

				// display the information about plugin class if needed
				for (auto it = _pluginLoader.begin(), end = _pluginLoader.end(); it != end; ++it)
				{
					poco_trace(logger(), "plugin path: " + it->first);
					for (auto itMan = it->second->begin(), endMan = it->second->end(); itMan != endMan; ++itMan)
						poco_trace(logger(), "found class in plugin lib: " + std::string(itMan->name()));
				}

				// load the class
				_pPluginInstance = _pluginLoader.create(pluginClass);
				_pluginLoader.classFor(pluginClass).autoDelete(_pPluginInstance);
			}
		}
		catch (Poco::Exception exp)
		{
			logger().log(exp);
		}
	}

	void unloadPlugin()
	{
		if (_pPluginInstance == nullptr)
			return;

		// default to unload PluginC.dll
		std::string pluginClass = config().getString("plugin.class", "PluginC");

		if (pluginClass == "PluginC")
		{
			_pluginLoader.unloadLibrary("PluginC.dll");
		}
		else
		{
			_pluginLoader.unloadLibrary("PluginAB.dll");
			_pluginBLoader.unloadLibrary("PluginAB.dll");
		}
	}

	int main(const ArgVec& args)
	{
		if (_helpRequested)
			return Application::EXIT_USAGE;

		// Although the logger should log messages only in proper logging level, it is good practice to
		// check current level setting to avoid the overhead of string construction.
		// Two way to do the same thing, use one of the following methods wherever appropriate:
		// (1). check if logger().*LevelName*() then do logger().*LevelName*("message string")
		// (2). use Poco's predefined macro: poco_LevelName(logger(), "message string")
		if (logger().information())
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
		std::cout << "\nplugin: " << _pPluginInstance->name() << " is loaded as you wish." << std::endl;
		if (_pPluginBInstance != nullptr)
		{
			std::cout << "you may want to know that additional class '"
				<< _pPluginBInstance->name() << "' is also loaded at "
				<< _pPluginBInstance->time() << std::endl;
		}

		if (!_checkKey.empty())
		{
			std::cout << "check out key: " << _checkKey << std::endl;
			AbstractConfiguration::Keys keys;
			config().keys(_checkKey, keys);
			if (!keys.empty())
			{
				std::cout << "has sub-key: ";
				for (auto value : keys)
					std::cout << value << " ";
				std::cout << std::endl;
			}
			else if (config().hasProperty(_checkKey))
				std::cout << _checkKey << " is full key." << std::endl;
			else
				std::cout << "no such configuration" << std::endl;

		}
		// wait for any key then exit
		std::system("pause");

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


// We can also use predefined macro with less control over how Application runs
//POCO_APP_MAIN(AppLoadPlugin)

int wmain(int argc, wchar_t** argv)
{
	Poco::AutoPtr<AppLoadPlugin> pApp = new AppLoadPlugin;
	try
	{
	    // init() process command line and set properties
		pApp->init(argc, argv);
	}
	catch (Poco::Exception& exp)
	{
		pApp->logger().log(exp);
		return Application::EXIT_CONFIG;
	}

	// user requests for help, no need to run the whole procedure
	if (pApp->helpRequested())
		return Application::EXIT_USAGE;

	// initialize(), main(), and then uninitialize()
	return pApp->run();
}
