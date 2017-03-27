#include <iostream>
#include <Poco/Logger.h>
#include "AppPullWorker.h"

using Poco::Util::Application;
using Poco::Logger;

int wmain(int argc, wchar_t** argv)
{
	AppPullWorker appMain;
	try
	{
		// init() process command line and set properties
		appMain.init(argc, argv);
	}
	catch (Poco::Exception& exp)
	{
		appMain.logger().log(exp);
		return Application::EXIT_CONFIG;
	}

	// user requests for help, no need to run the whole procedure
	if (appMain.helpRequested())
		return Application::EXIT_USAGE;

	try
	{
		// initialize(), main(), and then uninitialize()
		return appMain.run();
	}
	catch (Poco::Exception& e)
	{
		std::cerr << "Application.run() failed." << std::endl;
		appMain.logger().log(e);
		return Application::EXIT_SOFTWARE;
	}
}
