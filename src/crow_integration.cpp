#include "crow_integration.h"

void ComponentBase::registerEndpoints(crow::SimpleApp& app, std::string endpointPrefix)
{
	app.route_dynamic(endpointPrefix+"/"+this->getName()+"/status")
	([&](){
		return this->getStatus();
	});
}

std::string ComponentBase::generateUpdateJS(std::vector<std::string>)
{
	return "";
}

