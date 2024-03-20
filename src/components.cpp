#include "components.h"

std::string generateSelector(std::string name, std::vector<std::string> parent)
{
	std::string selector;
	for(auto p : parent)
		selector += "#" + p + " > ";
	selector += "#" + name;
	return selector;
}

std::string generateEndpoint(std::string name, std::vector<std::string> parent)
{
	std::string endpoint;
	for(auto p : parent)
		endpoint += "/" + p;
	endpoint += "/" + name;
	return endpoint;
}

std::string ComponentBase::generateUpdateJS(std::vector<std::string>)
{
       return "";
}
