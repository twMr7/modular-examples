#pragma once
#include <string>
#include <Poco/Util/Application.h>
#include <Poco/Util/OptionSet.h>

class AppLogAndConfig : public Poco::Util::Application
{
private:
	// for the help request by user
	bool _helpRequested{ false };
	void handleOptionHelp(const std::string& name, const std::string& value);
	void handleOptionDefine(const std::string& name, const std::string& value);
	void printProperties(const std::string& base);

protected:
	void initialize(Poco::Util::Application& self);
	void uninitialize();
	void defineOptions(Poco::Util::OptionSet& options);
	int main(const ArgVec& args);

public:
	AppLogAndConfig() {};
	bool helpRequested();
};

