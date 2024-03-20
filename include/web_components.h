#ifndef WEB_COMPONENTS_H__
#define WEB_COMPONENTS_H__

#include "crow_integration.h"
#include "brewery_components.h"
#include <string>
#include <sstream>

std::string generateSelector(std::string name, std::vector<std::string> parent);
std::string generateEndpoint(std::string name, std::vector<std::string> parent);

/* Generate Layout */
std::string generateLayout(TempSensor&);

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

template<class T>
std::string generateLayout(ReadableValue<T>& r)
{
	return "<div id=\"" + r.getName() + "\"></div>\n";
}

std::string generateLayout(Button& b);

template<class T>
std::string generateLayout(TargetValue<T>& t)
{
	std::string ret;
	ret += "<div id=\"" + t.getName() + "_label\"></div>\n";
	ret += "<input id=\"" + t.getName() + "\" type='range'/>\n";
	return ret;
}

/* Register Endpoints */
void registerEndpoints(TempSensor&, SimpleApp& app, std::string endpointPrefix);

template<class...Comps>
void registerEndpoints(ComponentTuple<Comps...>& ct, SimpleApp& app, std::string endpointPrefix)
{
	for_each_component(ct, [&](auto&& comp) {
			registerEndpoints(std::forward<decltype(comp)>(comp), app, endpointPrefix+"/"+ct.getName());
		});
	// could also register the tuple as an endpoint to get all status at once
}

template<class T>
void registerEndpoints(ReadableValue<T>& r, SimpleApp& app, std::string endpointPrefix)
{
	app.route_dynamic(endpointPrefix+"/"+r.getName()+"/status",
			[&](){
				return std::to_string(r.get());
			});
}

void registerEndpoints(Button& b, SimpleApp& app, std::string endpointPrefix);

template<class T>
void registerEndpoints(TargetValue<T>& t, SimpleApp& app, std::string endpointPrefix)
{
	registerEndpoints(static_cast<WriteableValue<T>&>(t), app, endpointPrefix);
	app.route_dynamic(endpointPrefix+"/"+t.getName()+"/set_target",
			[&](const CrowRequest& req){
				std::stringstream ss;
				ss << req.url_params_get("value");
				T v;
				ss >> v;
				t.set(v);
				return std::string{};
			});
}

/* Generate Update JS */
std::string generateUpdateJS(TempSensor&, std::vector<std::string> parent);

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

template<class T>
std::string generateUpdateJS(ReadableValue<T>& r, std::vector<std::string> parent)
{
	std::string selector = generateSelector(r.getName(), parent);
	std::string endpoint = generateEndpoint(r.getName(), parent);
	return "registerText('" + endpoint + "', '" + selector + "');\n";
}

std::string generateUpdateJS(Button& b, std::vector<std::string> parent);

template<class T>
std::string generateUpdateJS(TargetValue<T>& t, std::vector<std::string> parent)
{
	std::string selector = generateSelector(t.getName(), parent);
	std::string endpoint = generateEndpoint(t.getName(), parent);
	return "registerTargetValue(\'" + endpoint + "\', \'" + selector + "\'," + std::to_string(t.getMin()) + ", " + std::to_string(t.getMax()) + ");\n";
}

#endif

