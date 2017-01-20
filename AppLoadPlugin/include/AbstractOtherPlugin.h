#pragma once
#include <string>

class AbstractOtherPlugin
{
public:
	AbstractOtherPlugin() {};
	virtual ~AbstractOtherPlugin() {};
	virtual std::string name() const = 0;
	virtual std::string time() const = 0;
};

