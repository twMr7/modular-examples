#pragma once
#include <string>

class AbstractPlugin
{
public:
	AbstractPlugin() {};
	virtual ~AbstractPlugin() {};
	virtual std::string name() const = 0;
};

