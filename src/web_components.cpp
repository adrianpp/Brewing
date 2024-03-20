#include "web_components.h"

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

std::string generateLayout(TempSensor& t)
{
	return "<div id=\"" + t.getName() + "\"></div>\n"
		"<canvas id=\"" + t.getName() + "_graph\" style=\"width:100%;max-width:700px\"></canvas>\n";
}
void registerEndpoints(TempSensor& t, SimpleApp& app, std::string endpointPrefix)
{
	app.route_dynamic(endpointPrefix+"/"+t.getName()+"/status/<int>",
			[&](int last){
			JSONWrapper ret;
			auto hist = t.getHistory();
			for(unsigned int i = last; i < hist.size(); ++i)
			{
				JSONWrapper v;
				v.set("x", std::to_string(i*2));
				v.set("y", std::to_string(hist[i]));
				ret.set(i, v);
			}
			return ret.dump();
		});
}
std::string generateUpdateJS(TempSensor& t, std::vector<std::string> parent)
{
	std::string selector = generateSelector(t.getName(), parent);
	std::string endpoint = generateEndpoint(t.getName(), parent);
	return "registerGraph('" + endpoint + "', '" + selector + "', '" + selector + "_graph');\n";
}

std::string generateLayout(Button& b)
{
	return "<button id=\"" + b.getName() + "\">" + b.getName() + "</button>\n";
}

void registerEndpoints(Button& b, SimpleApp& app, std::string endpointPrefix)
{
	registerEndpoints(static_cast<ReadableValue<int>&>(b), app, endpointPrefix);
	app.route_dynamic(endpointPrefix+"/"+b.getName()+"/toggle",
			[&](){
				b.set(!b.get());
				return std::string{};
			});
}

std::string generateUpdateJS(Button& b, std::vector<std::string> parent)
{
	std::string selector = generateSelector(b.getName(), parent);
	std::string endpoint = generateEndpoint(b.getName(), parent);
	return "registerButton('" + endpoint + "', '" + selector + "');\n";
}
