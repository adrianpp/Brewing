#ifndef WEB_COMPONENTS_H__
#define WEB_COMPONENTS_H__

#include "crow_integration.h"
#include "brewery_components.h"
#include <string>
#include <sstream>

std::string generateSelector(std::string name, std::vector<std::string> parent);
std::string generateEndpoint(std::string name, std::vector<std::string> parent);

/* Generate Layout */
template<class...Comps>
std::string generateLayout(ComponentTuple<Comps...>& ct)
{
	std::string ret = "<fieldset id=\"" + ct.getName() + "\">\n";
	ret += "<legend>" + ct.getName() + "</legend>\n";
	for_each_component(ct, [&](auto&& comp) {
			ret += generateLayout(std::forward<decltype(comp)>(comp));
		});
	ret += "</fieldset>\n";
	return ret;

}
std::string generateLayout(TempSensor&);
std::string generateLayout(Button& b);
template<class T>
std::string generateLayout(ReadableValue<T>& r);
template<class T>
std::string generateLayout(TargetValue<T>& t);

/* Register Endpoints */
template<class...Comps>
void registerEndpoints(ComponentTuple<Comps...>& ct, SimpleApp& app, std::string endpointPrefix)
{
	for_each_component(ct, [&](auto&& comp) {
			registerEndpoints(std::forward<decltype(comp)>(comp), app, endpointPrefix+"/"+ct.getName());
		});
	// could also register the tuple as an endpoint to get all status at once
}
void registerEndpoints(TempSensor&, SimpleApp& app, std::string endpointPrefix);
void registerEndpoints(Button& b, SimpleApp& app, std::string endpointPrefix);
template<class T>
void registerEndpoints(ReadableValue<T>& r, SimpleApp& app, std::string endpointPrefix);
template<class T>
void registerEndpoints(TargetValue<T>& t, SimpleApp& app, std::string endpointPrefix);

/* Generate Update JS */
template<class...Comps>
std::string generateUpdateJS(ComponentTuple<Comps...>& ct, std::vector<std::string> parent)
{
	parent.push_back(ct.getName());
	std::string ret;
	for_each_component(ct, [&](auto&& comp) {
			ret += generateUpdateJS(std::forward<decltype(comp)>(comp), parent);
		});
	return ret;
}
std::string generateUpdateJS(TempSensor&, std::vector<std::string> parent);
std::string generateUpdateJS(Button& b, std::vector<std::string> parent);
template<class T>
std::string generateUpdateJS(ReadableValue<T>& r, std::vector<std::string> parent);
template<class T>
std::string generateUpdateJS(TargetValue<T>& t, std::vector<std::string> parent);

#endif

