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
	Poco::ClassLoader<AbstractPlugin> _pluginLoader;
	AbstractPlugin* _pPluginInstance{ nullptr };

public:
	AppLoadPlugin() : _helpRequested(false)
	{
	}

protected:
	void initialize(Application& self)
	{
		poco_information(logger(), std::string(name()) + " init");

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
				poco_debug(logger(), "invalid configuration file '" + filename + "', try loading default");
				loadConfiguration();
			}
		}
		catch (...)
		{
			poco_debug(logger(), "try loading default configuration file");
			loadConfiguration();
		}

		// ancestor initialization
		// registered subsystems, e.g. logger, are initialized here
		Application::initialize(self);

		loadPlugin();
	}

	void uninitialize()
	{
		poco_information(logger(), std::string(name()) + " uninit");

		// our own uninitialization code here
		unloadPlugin();

		// ancestor uninitialization
		Application::uninitialize();
	}

	void reinitialize(Application& self)
	{
		poco_information(logger(), std::string(name()) + " reinit");

		// our own uninitialization code here
		unloadPlugin();

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
			Option("plugin", "p", "load different plugin class, available plugins: { 'PluginA', 'PluginB', 'PluginC' }")
			.required(true)
			.repeatable(false)
			.argument("class")
			.callback(OptionCallback<AppLoadPlugin>(this, &AppLoadPlugin::handleArgumentPluginClass)));
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
		if (value == "PluginA" || value == "PluginB" || value == "PluginC")
			config().setString("plugin.class", value);
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

		// default to load PluginC class from PluginC.dll
		std::string pluginFile{ "PluginC.dll" };
		std::string pluginClass = config().getString("plugin.class", "PluginC");
		if (pluginClass == "PluginA" || pluginClass == "PluginB")
			pluginFile = "PluginAB.dll";

		// check the existence of the .dll file before loading it
		if (Poco::File(pluginFile).exists())
		{
			_pluginLoader.loadLibrary(pluginFile);

			// display the information about plugin class if needed
			Poco::ClassLoader<AbstractPlugin>::Iterator it(_pluginLoader.begin());
			Poco::ClassLoader<AbstractPlugin>::Iterator end(_pluginLoader.end());
			poco_trace(logger(), "Plugin loader info: " + it->first);
			for (; it != end; ++it)
			{
				poco_trace(logger(), "\tplugin path: " + it->first);
				Poco::Manifest<AbstractPlugin>::Iterator itMan(it->second->begin());
				Poco::Manifest<AbstractPlugin>::Iterator endMan(it->second->end());
				for (; itMan != endMan; ++itMan)
					poco_trace(logger(), "found class in plugin lib: " + std::string(itMan->name()));
			}

			// load the class
			_pPluginInstance = _pluginLoader.create(pluginClass);
			_pluginLoader.classFor(pluginClass).autoDelete(_pPluginInstance);
		}
	}

	void unloadPlugin()
	{
		if (_pPluginInstance == nullptr)
			return;

		// default to unload PluginC.dll
		std::string pluginFile{ "PluginC.dll" };
		std::string pluginClass = config().getString("plugin.class", "PluginC");
		if (pluginClass == "PluginA" || pluginClass == "PluginB")
			pluginFile = "PluginAB.dll";

		_pluginLoader.unloadLibrary(pluginFile);
	}

	int main(const ArgVec& args)
	{
		// Although the logger should log messages only in proper logging level, it is good practice to
		// check current level setting to avoid the overhead of string construction.
		// Two way to do the same thing, use one of the following methods wherever appropriate:
		// (1). check if logger().*LevelName*() then do logger().*LevelName*("message string")
		// (2). use Poco's predefined macro: poco_LevelName(logger(), "message string")
		if (!_helpRequested && logger().information())
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
		std::cout << "\nwhich plugin: " << _pPluginInstance->name() << " is loaded this time." << std::endl;

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
